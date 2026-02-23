
#include "DowngraderModule.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"

#include "LevelEditor.h"
#include "UObject/CoreRedirects.h"

void PrintAllRedirects()
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5
	TArray<FString> Filenames = {
		TEXT( "../../../Engine/Config/BaseEngine.ini" ),
		TEXT( "../../../Engine/Plugins/Interchange/Runtime/Config/BaseInterchange.ini" ),
		TEXT( "../../../Engine/Plugins/Interchange/Assets/Config/BaseInterchangeAssets.ini" ),
		TEXT( "../../../Engine/Plugins/Animation/ControlRig/Config/BaseControlRig.ini" ),
		TEXT( "../../../Engine/Plugins/EnhancedInput/Config/BaseEnhancedInput.ini" ),
		TEXT( "../../../Engine/Plugins/FX/Niagara/Config/BaseNiagara.ini" )
	};// GConfig->GetFilenames();
	for (int i = 0; i < Filenames.Num();i++)
	{
		const FString& Filename = Filenames[i];
		const TCHAR* RedirectSectionName = TEXT( "CoreRedirects" );
		const FConfigSection* RedirectSection = GConfig->GetSection( RedirectSectionName, false, Filename );
		if (RedirectSection)
		{
			TArray<FCoreRedirect> NewRedirects;

			for (FConfigSection::TConstIterator It( *RedirectSection ); It; ++It)
			{
				FString OldName, NewName, OverrideClassName;

				bool bInstanceOnly = false;
				bool bRemoved = false;
				bool bMatchSubstring = false;
				bool bMatchWildcard = false;

				const FString& KeyString = It.Key().ToString();
				const FString& ValueString = It.Value().GetValue();

				FParse::Bool( *ValueString, TEXT( "InstanceOnly=" ), bInstanceOnly );
				FParse::Bool( *ValueString, TEXT( "Removed=" ), bRemoved );
				FParse::Bool( *ValueString, TEXT( "MatchSubstring=" ), bMatchSubstring );
				FParse::Bool( *ValueString, TEXT( "MatchWildcard=" ), bMatchWildcard );

				FParse::Value( *ValueString, TEXT( "OldName=" ), OldName );
				FParse::Value( *ValueString, TEXT( "NewName=" ), NewName );

				FParse::Value( *ValueString, TEXT( "OverrideClassName=" ), OverrideClassName );

				FString RedirectType;
				if (KeyString.Compare( TEXT( "+ClassRedirects" ) ) == 0)
				{
					RedirectType = TEXT( "Type_Class" );
				}
				if (KeyString.Compare( TEXT( "+StructRedirects" ) ) == 0)
				{
					RedirectType = TEXT( "Type_Struct" );
				}
				if (KeyString.Compare( TEXT( "+FunctionRedirects" ) ) == 0)
				{
					RedirectType = TEXT( "Type_Function" );
				}
				if (KeyString.Compare( TEXT( "+EnumRedirects" ) ) == 0)
				{
					RedirectType = TEXT( "Type_Enum" );
				}
				if (KeyString.Compare( TEXT( "+PropertyRedirects" ) ) == 0)
				{
					RedirectType = TEXT( "Type_Property" );
				}
				if (KeyString.Compare( TEXT( "+PackageRedirects" ) ) == 0)
				{
					RedirectType = TEXT( "Type_Package" );
				}

				if (RedirectType.Len() > 0)
				{
					UE_LOG( LogTemp, Warning,
						TEXT( "Redirects.Add( FCoreRedirect( ECoreRedirectFlags::%s, TEXT( \"%s\" ), TEXT( \"%s\" ) ) );" ),
						*RedirectType, *NewName, *OldName );
				}

			}
		}
	}
#endif
}
bool RedirectExists( FString OldName, const TArray<FCoreRedirect>& Redirects )
{
	for (int i = 0; i < Redirects.Num(); i++)
	{
		const FCoreRedirect& Redirect = Redirects[i];
		if (Redirect.OldName.ToString().Compare( OldName ) == 0)
		{
			return true;
		}
	}

	return false;
}
void AddRedirects()
{
	bool DoPrintAllRedirects = true;
	if (DoPrintAllRedirects)
	{
		PrintAllRedirects();
	}
	TArray<FCoreRedirect> Redirects;
#if (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION <= 27)	
	//5.0
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/ControlRig.RigUnit_SpringInterpQuaternionV2" ), TEXT( "/Script/ControlRig.RigUnit_SpringInterpQuaternion" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/RigVMDeveloper.RigVMUnitNode" ), TEXT( "RigVMStructNode" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Enum, TEXT( "/Script/SlateCore.ETextTransformPolicy" ), TEXT( "/Script/Slate.ETextTransformPolicy" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "Mode" ), TEXT( "BlendProfile.BlendProfileMode" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "UnitNode" ), TEXT( "RigVMInjectionInfo.StructNode" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/DeveloperToolSettings.ProjectPackagingSettings" ), TEXT( "/Script/UnrealEd.ProjectPackagingSettings" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/DeveloperToolSettings.CookerSettings" ), TEXT( "/Script/UnrealEd.CookerSettings" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/Engine.StaticMeshDescriptionBulkData" ), TEXT( "/Script/MeshDescription.MeshDescriptionBulkDataWrapper" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/CoreUObject.FilePath" ), TEXT( "/Script/Engine.FilePath" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/CoreUObject.DirectoryPath" ), TEXT( "/Script/Engine.DirectoryPath" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "AnimationSequenceBase" ), TEXT( "AnimationBlueprintLibrary.GetNumFrames.AnimationSequence" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "AnimationSequenceBase" ), TEXT( "AnimationBlueprintLibrary.GetNumKeys.AnimationSequence" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "AnimationSequenceBase" ), TEXT( "AnimationBlueprintLibrary.GetAnimationTrackNames.AnimationSequence" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "AnimationSequenceBase" ), TEXT( "AnimationBlueprintLibrary.GetRawTrackPositionData.AnimationSequence" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "AnimationSequenceBase" ), TEXT( "AnimationBlueprintLibrary.GetRawTrackRotationData.AnimationSequence" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "AnimationSequenceBase" ), TEXT( "AnimationBlueprintLibrary.GetRawTrackScaleData.AnimationSequence" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "AnimationSequenceBase" ), TEXT( "AnimationBlueprintLibrary.GetRawTrackData.AnimationSequence" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "AnimationSequenceBase" ), TEXT( "AnimationBlueprintLibrary.IsValidRawAnimationTrackName.AnimationSequence" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "AnimationSequenceBase" ), TEXT( "AnimationBlueprintLibrary.GetAnimationNotifyEvents.AnimationSequence" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "AnimationSequenceBase" ), TEXT( "AnimationBlueprintLibrary.GetAnimationNotifyEventNames.AnimationSequence" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "AnimationSequenceBase" ), TEXT( "AnimationBlueprintLibrary.AddAnimationNotifyEvent.AnimationSequence" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "AnimationSequenceBase" ), TEXT( "AnimationBlueprintLibrary.AddAnimationNotifyStateEvent.AnimationSequence" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "AnimationSequenceBase" ), TEXT( "AnimationBlueprintLibrary.AddAnimationNotifyEventObject.AnimationSequence" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "AnimationSequenceBase" ), TEXT( "AnimationBlueprintLibrary.AddAnimationNotifyStateEventObject.AnimationSequence" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "AnimationSequenceBase" ), TEXT( "AnimationBlueprintLibrary.RemoveAnimationNotifyEventsByName.AnimationSequence" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "AnimationSequenceBase" ), TEXT( "AnimationBlueprintLibrary.RemoveAnimationNotifyEventsByTrack.AnimationSequence" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "AnimationSequenceBase" ), TEXT( "AnimationBlueprintLibrary.ReplaceAnimNotifyStates.AnimationSequence" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "AnimationSequenceBase" ), TEXT( "AnimationBlueprintLibrary.ReplaceAnimNotifies.AnimationSequence" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "SourceAnimationSequenceBase" ), TEXT( "AnimationBlueprintLibrary.CopyAnimNotifiesFromSequence.SrcAnimSequence" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "DestinationAnimationSequenceBase" ), TEXT( "AnimationBlueprintLibrary.CopyAnimNotifiesFromSequence.DestAnimSequence" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "AnimationSequenceBase" ), TEXT( "AnimationBlueprintLibrary.GetAnimationNotifyTrackNames.AnimationSequence" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "AnimationSequenceBase" ), TEXT( "AnimationBlueprintLibrary.AddAnimationNotifyTrack.AnimationSequence" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "AnimationSequenceBase" ), TEXT( "AnimationBlueprintLibrary.RemoveAnimationNotifyTrack.AnimationSequence" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "AnimationSequenceBase" ), TEXT( "AnimationBlueprintLibrary.RemoveAllAnimationNotifyTracks.AnimationSequence" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "AnimationSequenceBase" ), TEXT( "AnimationBlueprintLibrary.IsValidAnimNotifyTrackName.AnimationSequence" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "AnimationSequenceBase" ), TEXT( "AnimationBlueprintLibrary.GetAnimationNotifyEventsForTrack.AnimationSequence" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "AnimationAsset" ), TEXT( "AnimationBlueprintLibrary.AddMetaData.AnimationSequence" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "AnimationAsset" ), TEXT( "AnimationBlueprintLibrary.AddMetaDataObject.AnimationSequence" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "AnimationAsset" ), TEXT( "AnimationBlueprintLibrary.RemoveAllMetaData.AnimationSequence" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "AnimationAsset" ), TEXT( "AnimationBlueprintLibrary.RemoveMetaData.AnimationSequence" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "AnimationAsset" ), TEXT( "AnimationBlueprintLibrary.RemoveMetaDataOfClass.AnimationSequence" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "AnimationAsset" ), TEXT( "AnimationBlueprintLibrary.GetMetaData.AnimationSequence" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "AnimationAsset" ), TEXT( "AnimationBlueprintLibrary.GetMetaDataOfClass.AnimationSequence" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "AnimationAsset" ), TEXT( "AnimationBlueprintLibrary.ContainsMetaDataOfClass.AnimationSequence" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "AnimationSequenceBase" ), TEXT( "AnimationBlueprintLibrary.GetBonePoseForTime.AnimationSequence" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "AnimationSequenceBase" ), TEXT( "AnimationBlueprintLibrary.GetBonePoseForFrame.AnimationSequence" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "AnimationSequenceBase" ), TEXT( "AnimationBlueprintLibrary.GetBonePosesForTime.AnimationSequence" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "AnimationSequenceBase" ), TEXT( "AnimationBlueprintLibrary.GetBonePosesForFrame.AnimationSequence" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "AnimationSequenceBase" ), TEXT( "AnimationBlueprintLibrary.GetSequenceLength.AnimationSequence" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "AnimationSequenceBase" ), TEXT( "AnimationBlueprintLibrary.GetRateScale.AnimationSequence" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "AnimationSequenceBase" ), TEXT( "AnimationBlueprintLibrary.SetRateScale.AnimationSequence" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "AnimationSequenceBase" ), TEXT( "AnimationBlueprintLibrary.GetFrameAtTime.AnimationSequence" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "AnimationSequenceBase" ), TEXT( "AnimationBlueprintLibrary.GetTimeAtFrame.AnimationSequence" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "AnimationSequenceBase" ), TEXT( "AnimationBlueprintLibrary.IsValidTime.AnimationSequence" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "AnimationSequenceBase" ), TEXT( "AnimationBlueprintLibrary.FindBonePathToRoot.AnimationSequence" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "bIgnoreDenyListScanFilters" ), TEXT( "AssetRegistry.ScanPathsSynchronous.bIgnoreBlackListScanFilters" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "LayerAllowList" ), TEXT( "LandscapeComponent.LayerWhitelist" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Enum, TEXT( "" ), TEXT( "ELandscapeLayerPaintingRestriction" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "DataLayer.bIsRuntime" ), TEXT( "DataLayer.bIsDynamicallyLoaded" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "DataLayer.InitialRuntimeState" ), TEXT( "DataLayer.InitialState" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "DataLayerSubsystem.OnDataLayerRuntimeStateChanged" ), TEXT( "DataLayerSubsystem.OnDataLayerStateChanged" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Function, TEXT( "WorldDataLayers.OnDataLayerRuntimeStateChanged" ), TEXT( "WorldDataLayers.OnDataLayerStateChanged" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/Engine.BlendSpace" ), TEXT( "/Script/Engine.BlendSpaceBase" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Function, TEXT( "SetDesiredSizeOverride" ), TEXT( "Image.SetBrushSize" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/Engine.AnimBlueprintClassSubsystem_PropertyAccess" ), TEXT( "/Script/PropertyAccess.AnimBlueprintClassSubsystem_PropertyAccess" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "" ), TEXT( "/Script/PropertyAccess" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/Engine.AnimSubsystemInstance" ), TEXT( "AnimInstanceSubsystemData" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/AnimationBlueprintLibrary.AnimationBlueprintLibrary" ), TEXT( "/Script/AnimationModifiers.AnimationBlueprintLibrary" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "SimulatePhysics" ), TEXT( "GeometryCollectionComponent.Simulating" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "GeometryCollectionComponent.ApplyExternalStrain.ItemIndex" ), TEXT( "GeometryCollectionComponent.ApplyExternalStrain.Index" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "GeometryCollectionComponent.CrumbleCluster.ItemIndex" ), TEXT( "GeometryCollectionComponent.CrumbleCluster.Index" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "OnPreForwardsSolveDelegate" ), TEXT( "ControlRigComponent.OnPreUpdateDelegate" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "OnPostForwardsSolveDelegate" ), TEXT( "ControlRigComponent.OnPostUpdateDelegate" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "AnimNode_Mirror.BlendTime" ), TEXT( "AnimNode_Mirror.BlendTimeOnMirrorStateChange" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "AnimNode_Mirror.bResetChild" ), TEXT( "AnimNode_Mirror.bResetChildOnMirrorStateChange" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "CompressedPing" ), TEXT( "PlayerState.Ping" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/MovieSceneTracks.MovieSceneFloatVectorSection" ), TEXT( "/Script/MovieSceneTracks.MovieSceneVectorSection" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/MovieSceneTracks.MovieSceneFloatVectorTrack" ), TEXT( "/Script/MovieSceneTracks.MovieSceneVectorTrack" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/SequencerScripting.MovieSceneFloatVectorTrackExtensions" ), TEXT( "/Script/MovieSceneTracks.MovieSceneVectorTrackExtensions" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/MovieSceneTracks.MovieSceneFloatVectorKeyStructBase" ), TEXT( "/Script/MovieSceneTracks.MovieSceneVectorKeyStructBase" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/MovieSceneTracks.MovieSceneVector3fKeyStruct" ), TEXT( "/Script/MovieSceneTracks.MovieSceneVectorKeyStruct" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/AudioExtensions.AudioParameter" ), TEXT( "AudioComponentParam" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "AudioParameter.ObjectParam" ), TEXT( "AudioParameter.SoundWaveParam" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Function, TEXT( "AudioParameterControllerInterface.SetBoolParameter" ), TEXT( "AudioComponent.SetBoolParameter" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Function, TEXT( "AudioParameterControllerInterface.SetFloatParameter" ), TEXT( "AudioComponent.SetFloatParameter" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Function, TEXT( "AudioParameterControllerInterface.SetIntParameter" ), TEXT( "AudioComponent.SetIntParameter" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "AudioComponent.DefaultParameters" ), TEXT( "AudioComponent.InstanceParameters" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Function, TEXT( "AudioParameterControllerInterface.SetBoolParameter" ), TEXT( "SoundGeneratorParameterInterface.SetBoolParameter" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Function, TEXT( "AudioParameterControllerInterface.SetFloatParameter" ), TEXT( "SoundGeneratorParameterInterface.SetFloatParameter" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Function, TEXT( "AudioParameterControllerInterface.SetIntParameter" ), TEXT( "SoundGeneratorParameterInterface.SetIntParameter" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/AudioExtensions.AudioParameterControllerInterface" ), TEXT( "/Script/AudioExtensions.AudioParameterInterface" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Enum, TEXT( "" ), TEXT( "ESoundwaveSampleRateSettings" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "GameplayCueNotify_SoundInfo.Sound" ), TEXT( "GameplayCueNotify_SoundInfo.SoundCue" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/LiveLinkAnimationCore.LiveLinkRetargetAsset" ), TEXT( "/Script/LiveLink.LiveLinkRetargetAsset" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/LiveLinkAnimationCore.LiveLinkRemapAsset" ), TEXT( "/Script/LiveLink.LiveLinkRemapAsset" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/LiveLinkAnimationCore.LiveLinkInstance" ), TEXT( "/Script/LiveLink.LiveLinkInstance" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/LiveLinkAnimationCore.AnimNode_LiveLinkPose" ), TEXT( "/Script/LiveLink.AnimNode_LiveLinkPose" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/LiveLinkAnimationCore.LiveLinkInstanceProxy" ), TEXT( "/Script/LiveLink.LiveLinkInstanceProxy" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Enum, TEXT( "" ), TEXT( "ELevelInstanceRuntimeBehavior" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "" ), TEXT( "/Engine/VT/LightmapVirtualTextureSpace_0_Compressed" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/ComputeFramework.ComputeGraph" ), TEXT( "ComputeGraph" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/ComputeFramework.ComputeGraphComponent" ), TEXT( "ComputeGraphComponent" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/ComputeFramework.ComputeKernel" ), TEXT( "ComputeKernel" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/ComputeFramework.ComputeKernelFromText" ), TEXT( "ComputeKernelFromText" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/ComputeFramework.ComputeKernelSource" ), TEXT( "ComputeKernelSource" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/ComputeFramework.ComputeKernel" ), TEXT( "ComputeKernel" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/Engine.PackedLevelActor" ), TEXT( "PackedLevelInstance" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/UnrealEd.EditorStyleSettings" ), TEXT( "EditorStyleSettings" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Enum, TEXT( "/Script/UnrealEd.EAssetEditorOpenLocation" ), TEXT( "EAssetEditorOpenLocation" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Enum, TEXT( "/Script/OutputLog.ELogCategoryColorizationMode" ), TEXT( "ELogCategoryColorizationMode" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Enum, TEXT( "/Script/OutputLog.ELogCategoryColorizationMode" ), TEXT( "/Script/UnrealEd.ELogCategoryColorizationMode" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "PostProcessSettings.LocalExposureContrastScale" ), TEXT( "PostProcessSettings.LocalExposureContrastReduction" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "PostProcessSettings.bOverride_LocalExposureContrastScale" ), TEXT( "PostProcessSettings.bOverride_LocalExposureContrastReduction" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Function, TEXT( "Conv_DoubleToString" ), TEXT( "KismetStringLibrary.Conv_FloatToString" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "Conv_DoubleToString.InDouble" ), TEXT( "KismetStringLibrary.Conv_DoubleToString.InFloat" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Function, TEXT( "Conv_StringToDouble" ), TEXT( "KismetStringLibrary.Conv_StringToFloat" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Function, TEXT( "BuildString_Double" ), TEXT( "KismetStringLibrary.BuildString_Float" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "BuildString_Double.InDouble" ), TEXT( "KismetStringLibrary.BuildString_Double.InFloat" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Function, TEXT( "MakeVector" ), TEXT( "KismetMathLibrary.MakeVector_NetQuantize" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Function, TEXT( "MakeVector" ), TEXT( "KismetMathLibrary.MakeVector_NetQuantize10" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Function, TEXT( "MakeVector" ), TEXT( "KismetMathLibrary.MakeVector_NetQuantize100" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Function, TEXT( "MakeVector" ), TEXT( "KismetMathLibrary.MakeVector_NetQuantizeNormal" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Function, TEXT( "BreakVector" ), TEXT( "KismetMathLibrary.BreakVector_NetQuantize" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Function, TEXT( "BreakVector" ), TEXT( "KismetMathLibrary.BreakVector_NetQuantize10" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Function, TEXT( "BreakVector" ), TEXT( "KismetMathLibrary.BreakVector_NetQuantize100" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Function, TEXT( "BreakVector" ), TEXT( "KismetMathLibrary.BreakVector_NetQuantizeNormal" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Function, TEXT( "Multiply_DoubleDouble" ), TEXT( "KismetMathLibrary.Multiply_FloatFloat" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Function, TEXT( "Divide_DoubleDouble" ), TEXT( "KismetMathLibrary.Divide_FloatFloat" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Function, TEXT( "Add_DoubleDouble" ), TEXT( "KismetMathLibrary.Add_FloatFloat" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Function, TEXT( "Subtract_DoubleDouble" ), TEXT( "KismetMathLibrary.Subtract_FloatFloat" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Function, TEXT( "Less_DoubleDouble" ), TEXT( "KismetMathLibrary.Less_FloatFloat" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Function, TEXT( "Greater_DoubleDouble" ), TEXT( "KismetMathLibrary.Greater_FloatFloat" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Function, TEXT( "LessEqual_DoubleDouble" ), TEXT( "KismetMathLibrary.LessEqual_FloatFloat" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Function, TEXT( "GreaterEqual_DoubleDouble" ), TEXT( "KismetMathLibrary.GreaterEqual_FloatFloat" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Function, TEXT( "EqualEqual_DoubleDouble" ), TEXT( "KismetMathLibrary.EqualEqual_FloatFloat" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Function, TEXT( "NotEqual_DoubleDouble" ), TEXT( "KismetMathLibrary.NotEqual_FloatFloat" ) ) );

	//AnimBlueprintExtensions
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/AnimGraph.AnimBlueprintExtension_Base" ), TEXT( "/Script/Engine.BlueprintExtension" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/AnimGraph.AnimBlueprintExtension_Attributes" ), TEXT( "/Script/Engine.BlueprintExtension" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/AnimGraph.AnimBlueprintExtension_BlendSpaceGraph" ), TEXT( "/Script/Engine.BlueprintExtension" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/AnimGraph.AnimBlueprintExtension_CachedPose" ), TEXT( "/Script/Engine.BlueprintExtension" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/AnimGraph.AnimBlueprintExtension_CallFunction" ), TEXT( "/Script/Engine.BlueprintExtension" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/AnimGraph.AnimBlueprintExtension_PropertyAccess" ), TEXT( "/Script/Engine.BlueprintExtension" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/AnimGraph.AnimBlueprintExtension_LinkedAnimGraph" ), TEXT( "/Script/Engine.BlueprintExtension" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/AnimGraph.AnimBlueprintExtension_StateMachine" ), TEXT( "/Script/Engine.BlueprintExtension" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/AnimGraph.AnimBlueprintExtension_NodeRelevancy" ), TEXT( "/Script/Engine.BlueprintExtension" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/AnimGraph.AnimBlueprintExtension_Tag" ), TEXT( "/Script/Engine.BlueprintExtension" ) ) );

	//EnhancedInput
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Function, TEXT( "/Script/InputMappingContext.UnmapAllKeysFromAction" ), TEXT( "/Script/InputMappingContext.UnmapAction" ) ) );
	
	//Downgrader function dependencies
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Function, TEXT( "BlueprintMapLibrary.Map_IsNotEmpty" ), TEXT( "DowngraderFunctionDependencies.Map_IsNotEmpty" ) ) );

	if (!RedirectExists( TEXT( "RigVMAggregateNode" ), Redirects ))
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "RigVMAggregateNode" ), TEXT( "RigVMCollapseNode" ) ) );
#endif
#if (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION <= 27) || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION <= 0)
	//5.1
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/Engine.DataCacheDuplicatedObjectData" ), TEXT( "/Script/Engine.ActorComponentDuplicatedObjectData" ) ) );
	//if (!RedirectExists( TEXT( "/Script/RigVMDeveloper.RigVMUnitNode" ), Redirects ))
		//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/RigVMDeveloper.RigVMUnitNode" ), TEXT( "RigVMTemplateNode" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/MovieScene.MovieSceneBindingProxy" ), TEXT( "/Script/SequencerScripting.SequencerBindingProxy" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Function, TEXT( "MakeLiteralDouble" ), TEXT( "KismetSystemLibrary.MakeLiteralFloat" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Function, TEXT( "Conv_DoubleToText" ), TEXT( "KismetTextLibrary.Conv_FloatToText" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "Conv_DoubleToText.Value" ), TEXT( "KismetTextLibrary.Conv_DoubleToText.InDouble" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "AITask_MoveTo.AIMoveTo.bUseContinuousGoalTracking" ), TEXT( "AITask_MoveTo.AIMoveTo.bUseContinuosGoalTracking" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "NewValue" ), TEXT( "RectLightComponent.SetSourceTexture.bNewValue" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "NewValue" ), TEXT( "RectLightComponent.SetSourceWidth.bNewValue" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "bHeightfieldSolidPostInclusionBoundsFiltering" ), TEXT( "RecastNavMeshTileGenerationDebug.bHeightfieldSolidPostRadiusFiltering" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "SimSpaceSettings.WorldAlpha" ), TEXT( "SimSpaceSettings.MasterAlpha" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "LeaderPoseComponent" ), TEXT( "SkinnedMeshComponent.MasterPoseComponent" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "bUseBoundsFromLeaderPoseComponent" ), TEXT( "SkinnedMeshComponent.bUseBoundsFromMasterPoseComponent" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "bIgnoreLeaderPoseComponentLOD" ), TEXT( "SkinnedMeshComponent.bIgnoreMasterPoseComponentLOD" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Function, TEXT( "SetLeaderPoseComponent" ), TEXT( "SkinnedMeshComponent.SetMasterPoseComponent" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "SkinnedMeshComponent.SetMasterPoseComponent.NewLeaderBoneComponent" ), TEXT( "SkinnedMeshComponent.SetMasterPoseComponent.NewMasterBoneComponent" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "bPropagateCurvesToFollowers" ), TEXT( "SkeletalMeshComponent.bPropagateCurvesToSlaves" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Function, TEXT( "BindClothToLeaderPoseComponent" ), TEXT( "SkeletalMeshComponent.BindClothToMasterPoseComponent" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Function, TEXT( "UnbindClothFromLeaderPoseComponent" ), TEXT( "SkeletalMeshComponent.UnbindClothFromMasterPoseComponent" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Function, TEXT( "SetSkinnedAssetAndUpdate" ), TEXT( "SkinnedMeshComponent.SetSkeletalMesh" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Function, TEXT( "SetSkeletalMeshAsset" ), TEXT( "SkinnedMeshComponent.SetSkeletalMesh" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "PrimaryPC" ), TEXT( "AGameplayAbilityWorldReticle.MasterPC" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "PrimaryPC" ), TEXT( "AGameplayAbilityTargetActor.MasterPC" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/EngineCameras.LegacyCameraShake" ), TEXT( "CameraShake" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/EngineCameras.LegacyCameraShake" ), TEXT( "MatineeCameraShake" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/EngineCameras.LegacyCameraShakePattern" ), TEXT( "MatineeCameraShakePattern" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/EngineCameras.LegacyCameraShakeFunctionLibrary" ), TEXT( "MatineeCameraShakeFunctionLibrary" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Function, TEXT( "LegacyCameraShake.StartLegacyCameraShake" ), TEXT( "LegacyCameraShake.StartMatineeCameraShake" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Function, TEXT( "LegacyCameraShake.StartLegacyCameraShakeFromSource" ), TEXT( "LegacyCameraShake.StartMatineeCameraShakeFromSource" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Function, TEXT( "LegacyCameraShakeFunctionLibrary.Conv_LegacyCameraShake" ), TEXT( "LegacyCameraShakeFunctionLibrary.Conv_MatineeCameraShake" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "BoneTimecodeAnimationAttributeNameSettings" ), TEXT( "AnimationSettings.BoneTimecodeCustomAttributeNameSettings" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "BoneAnimationAttributesNames" ), TEXT( "AnimationSettings.BoneCustomAttributesNames" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "BoneNamesWithAnimationAttributes" ), TEXT( "AnimationSettings.BoneNamesWithCustomAttributes" ) ) );

	//Wonder how this got here ?
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/Engine.MaterialExpressionDistanceFieldApproxAO" ), TEXT( "MaterialExpressionConstant" ) ) );

	//Manual replacement, still better than disappearing nodes
	if (!RedirectExists( TEXT( "RigVMAggregateNode" ), Redirects ))
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "RigVMAggregateNode" ), TEXT( "RigVMCollapseNode" ) ) );

	//Niagara
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/Niagara.NiagaraDataInterfacePhysicsAsset" ), TEXT( "NiagaraDataInterfacePhysicsAsset" ) ) );
#endif
#if (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION <= 27) || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION <= 1)
	//5.2
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/Engine.InputDeviceLibrary" ), TEXT( "/Script/Engine.PlatformInputDeviceMapperLibrary" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/Engine.ActivateDevicePropertyParams" ), TEXT( "/Script/Engine.SetDevicePropertyParams" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Function, TEXT( "InputDeviceLibrary.GetPlayerControllerFromInputDevice" ), TEXT( "InputDeviceSubsystem.GetPlayerControllerFromInputDevice" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Function, TEXT( "InputDeviceLibrary.GetPlayerControllerFromPlatformUser" ), TEXT( "InputDeviceSubsystem.GetPlayerControllerFromPlatformUser" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Function, TEXT( "InputDeviceLibrary.IsDevicePropertyHandleValid" ), TEXT( "InputDeviceSubsystem.IsDevicePropertyHandleValid" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "bHeightfieldFromRasterization" ), TEXT( "/Script/NavigationSystem.RecastNavMeshTileGenerationDebug.bHeightfieldSolidFromRasterization" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "bHeightfieldPostInclusionBoundsFiltering" ), TEXT( "/Script/NavigationSystem.RecastNavMeshTileGenerationDebug.bHeightfieldSolidPostInclusionBoundsFiltering" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "bHeightfieldPostHeightFiltering" ), TEXT( "/Script/NavigationSystem.RecastNavMeshTileGenerationDebug.bHeightfieldSolidPostHeightFiltering" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "bHeightfieldBounds" ), TEXT( "/Script/NavigationSystem.RecastNavMeshTileGenerationDebug.bHeightfieldSolidBounds" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/AssetDefinition.RevisionInfo" ), TEXT( "/Script/AssetTools.RevisionInfo" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "DecalComponent.SetFadeIn.Duration" ), TEXT( "DecalComponent.SetFadeIn.Duaration" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "BlendSpace.ManualPerBoneOverrides" ), TEXT( "BlendSpace.PerBoneBlend" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/MovieRenderPipelineCore.MoviePipelinePrimaryConfig" ), TEXT( "/Script/MovieRenderPipelineCore.MoviePipelineMasterConfig" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Function, TEXT( "Conv_DoubleToLinearColor" ), TEXT( "KismetMathLibrary.Conv_FloatToLinearColor" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Function, TEXT( "Conv_ByteToDouble" ), TEXT( "KismetMathLibrary.Conv_ByteToFloat" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Function, TEXT( "Conv_IntToDouble" ), TEXT( "KismetMathLibrary.Conv_IntToFloat" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Function, TEXT( "Conv_BoolToDouble" ), TEXT( "KismetMathLibrary.Conv_BoolToFloat" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Function, TEXT( "Conv_DoubleToVector" ), TEXT( "KismetMathLibrary.Conv_FloatToVector" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "Conv_DoubleToLinearColor.InDouble" ), TEXT( "KismetMathLibrary.Conv_DoubleToLinearColor.InFloat" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "Conv_DoubleToVector.InDouble" ), TEXT( "KismetMathLibrary.Conv_DoubleToVector.InFloat" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "KismetSystemLibrary.MakeTopLevelAssetPath.PackageName" ), TEXT( "KismetSystemLibrary.MakeTopLevelAssetPath.FullPathOrPackageName" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "/Script/NavigationSystem.NavigationSystemV1.GeometryExportTriangleCountWarningThreshold" ), TEXT( "/Script/NavigationSystem.NavigationSystemV1.GeometryExportVertexCountWarningThreshold" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "ISMComponentDescriptor.bReverseCulling" ), TEXT( "ISMComponentDescriptor.bIsLocalToWorldDeterminantNegative" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "MaterialInstanceBasePropertyOverrides.MaxWorldPositionOffsetDisplacement" ), TEXT( "MaterialInstanceBasePropertyOverrides.MaxWorldPositionOffsetDistance" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "MaterialInstanceBasePropertyOverrides.bOverride_MaxWorldPositionOffsetDisplacement" ), TEXT( "MaterialInstanceBasePropertyOverrides.bOverride_MaxWorldPositionOffsetDistance" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "Material.MaxWorldPositionOffsetDisplacement" ), TEXT( "Material.MaxWorldPositionOffsetDistance" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "NonSpatializedRadiusStart" ), TEXT( "/Script/Engine.SoundAttenuationSettings.OmniRadius" ) ) );

	//Downgrader function dependencies
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/RigVM.RigVMBlueprintGeneratedClass" ), TEXT( "/Script/Downgrader.RigVMBlueprintGeneratedClass_Downgrader" ) ) );//Fixes ControlRigBlueprints
	//AnimBlueprintExtensions
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/AnimGraph.AnimBlueprintExtension_SharedLinkedAnimLayers" ), TEXT( "/Script/Engine.BlueprintExtension" ) ) );//first appeared in 5.2

	//ControlRig - Most of it is from 5.2
	bool AddControlRigRedirects = true;

	if (AddControlRigRedirects)
	{
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "RigUnit_SetTransform.Value" ), TEXT( "RigUnit_SetTransform.Transform" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "RigUnit_SetTranslation.Value" ), TEXT( "RigUnit_SetTranslation.Translation" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "RigUnit_SetRotation.Value" ), TEXT( "RigUnit_SetRotation.Rotation" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "RigUnit_ToWorldSpace_Transform.Value" ), TEXT( "RigUnit_ToWorldSpace_Transform.Transform" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "RigUnit_ToWorldSpace_Transform.Value" ), TEXT( "RigUnit_ToWorldSpace_Transform.Transform" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "RigUnit_ToWorldSpace_Location.Value" ), TEXT( "RigUnit_ToWorldSpace_Location.Location" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "RigUnit_ToWorldSpace_Rotation.Value" ), TEXT( "RigUnit_ToWorldSpace_Rotation.Rotation" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "RigUnit_ToRigSpace_Transform.Value" ), TEXT( "RigUnit_ToRigSpace_Transform.Transform" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "RigUnit_ToRigSpace_Location.Value" ), TEXT( "RigUnit_ToRigSpace_Location.Location" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "RigUnit_ToRigSpace_Rotation.Value" ), TEXT( "RigUnit_ToRigSpace_Rotation.Rotation" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "RigVMFunction_MathQuaternionRotateVector.Transform" ), TEXT( "RigVMFunction_MathQuaternionRotateVector.Quaternion" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "RigVMFunction_MathTransformRotateVector.Vector" ), TEXT( "RigVMFunction_MathTransformRotateVector.Direction" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "RigUnit_SetRelativeTransformForItem.Value" ), TEXT( "RigUnit_SetRelativeTransformForItem.RelativeTransform" ) ) );
		//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/RigVMDeveloper.RigVMGraph" ), TEXT( "RigVMGraph" ) ) );
		//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Enum, TEXT( "" ), TEXT( "ERigElementType" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Enum, TEXT( "/Script/AnimationCore.EEulerRotationOrder" ), TEXT( "/Script/ControlRig.EControlRigRotationOrder" ) ) );
		//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Enum, TEXT( "/Script/AnimationCore.EEulerRotationOrder" ), TEXT( "EControlRigRotationOrder" ) ) );

		//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "OnPreConstructionDelegate" ), TEXT( "/Script/ControlRig.ControlRigComponent.OnPreSetupDelegate" ) ) );
		//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "OnPostConstructionDelegate" ), TEXT( "/Script/ControlRig.ControlRigComponent.OnPostSetupDelegate" ) ) );
		//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "ConstructionEventBorderColor" ), TEXT( "/Script/ControlRig.UControlRigSettings.SetupEventBorderColor" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_NameBase" ), TEXT( "/Script/ControlRig.RigUnit_NameBase" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_NameConcat" ), TEXT( "/Script/ControlRig.RigUnit_NameConcat" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_NameTruncate" ), TEXT( "/Script/ControlRig.RigUnit_NameTruncate" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_NameReplace" ), TEXT( "/Script/ControlRig.RigUnit_NameReplace" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_EndsWith" ), TEXT( "/Script/ControlRig.RigUnit_EndsWith" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_StartsWith" ), TEXT( "/Script/ControlRig.RigUnit_StartsWith" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_Contains" ), TEXT( "/Script/ControlRig.RigUnit_Contains" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMDispatch_CoreBase" ), TEXT( "/Script/ControlRig.RigDispatch_CoreBase" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMDispatch_CoreEquals" ), TEXT( "/Script/ControlRig.RigDispatch_CoreEquals" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMDispatch_CoreNotEquals" ), TEXT( "/Script/ControlRig.RigDispatch_CoreNotEquals" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMDispatch_Print" ), TEXT( "/Script/ControlRig.RigDispatch_Print" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_StringBase" ), TEXT( "/Script/ControlRig.RigUnit_StringBase" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_StringConcat" ), TEXT( "/Script/ControlRig.RigUnit_StringConcat" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_StringTruncate" ), TEXT( "/Script/ControlRig.RigUnit_StringTruncate" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_StringReplace" ), TEXT( "/Script/ControlRig.RigUnit_StringReplace" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_StringEndsWith" ), TEXT( "/Script/ControlRig.RigUnit_StringEndsWith" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_StringStartsWith" ), TEXT( "/Script/ControlRig.RigUnit_StringStartsWith" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_StringContains" ), TEXT( "/Script/ControlRig.RigUnit_StringContains" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_StringLength" ), TEXT( "/Script/ControlRig.RigUnit_StringLength" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_StringTrimWhitespace" ), TEXT( "/Script/ControlRig.RigUnit_StringTrimWhitespace" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_StringToUppercase" ), TEXT( "/Script/ControlRig.RigUnit_StringToUppercase" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_StringToLowercase" ), TEXT( "/Script/ControlRig.RigUnit_StringToLowercase" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_StringReverse" ), TEXT( "/Script/ControlRig.RigUnit_StringReverse" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_StringLeft" ), TEXT( "/Script/ControlRig.RigUnit_StringLeft" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_StringRight" ), TEXT( "/Script/ControlRig.RigUnit_StringRight" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_StringMiddle" ), TEXT( "/Script/ControlRig.RigUnit_StringMiddle" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_StringFind" ), TEXT( "/Script/ControlRig.RigUnit_StringFind" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_StringSplit" ), TEXT( "/Script/ControlRig.RigUnit_StringSplit" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_StringJoin" ), TEXT( "/Script/ControlRig.RigUnit_StringJoin" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_StringPadInteger" ), TEXT( "/Script/ControlRig.RigUnit_StringPadInteger" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMDrawInstruction" ), TEXT( "/Script/ControlRig.ControlRigDrawInstruction" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMDrawContainer" ), TEXT( "/Script/ControlRig.ControlRigDrawContainer" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMDrawInterface" ), TEXT( "/Script/ControlRig.ControlRigDrawInterface" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Enum, TEXT( "/Script/RigVM.ERigVMTransformSpace" ), TEXT( "EBoneGetterSetterMode" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Enum, TEXT( "/Script/RigVM.ERigVMAnimEasingType" ), TEXT( "EControlRigAnimEasingType" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Enum, TEXT( "/Script/RigVM.ERigVMClampSpatialMode" ), TEXT( "EControlRigClampSpatialMode" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Enum, TEXT( "/Script/RigVM.ERigUnitDebugTransformMode" ), TEXT( "/Script/ControlRig.ERigUnitDebugTransformMode" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFourPointBezier" ), TEXT( "/Script/ControlRig.CRFourPointBezier" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMMirrorSettings" ), TEXT( "/Script/ControlRig.RigMirrorSettings" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathBase" ), TEXT( "/Script/ControlRig.RigUnit_MathBase" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathMutableBase" ), TEXT( "/Script/ControlRig.RigUnit_MathMutableBase" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathBoolBase" ), TEXT( "/Script/ControlRig.RigUnit_MathBoolBase" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathBoolConstant" ), TEXT( "/Script/ControlRig.RigUnit_MathBoolConstant" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathBoolUnaryOp" ), TEXT( "/Script/ControlRig.RigUnit_MathBoolUnaryOp" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathBoolBinaryOp" ), TEXT( "/Script/ControlRig.RigUnit_MathBoolBinaryOp" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathBoolBinaryAggregateOp" ), TEXT( "/Script/ControlRig.RigUnit_MathBoolBinaryAggregateOp" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathBoolMake" ), TEXT( "/Script/ControlRig.RigUnit_MathBoolMake" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathBoolConstTrue" ), TEXT( "/Script/ControlRig.RigUnit_MathBoolConstTrue" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathBoolConstFalse" ), TEXT( "/Script/ControlRig.RigUnit_MathBoolConstFalse" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathBoolNot" ), TEXT( "/Script/ControlRig.RigUnit_MathBoolNot" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathBoolAnd" ), TEXT( "/Script/ControlRig.RigUnit_MathBoolAnd" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathBoolNand" ), TEXT( "/Script/ControlRig.RigUnit_MathBoolNand" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathBoolNand2" ), TEXT( "/Script/ControlRig.RigUnit_MathBoolNand2" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathBoolOr" ), TEXT( "/Script/ControlRig.RigUnit_MathBoolOr" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathBoolEquals" ), TEXT( "/Script/ControlRig.RigUnit_MathBoolEquals" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathBoolNotEquals" ), TEXT( "/Script/ControlRig.RigUnit_MathBoolNotEquals" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathBoolToggled" ), TEXT( "/Script/ControlRig.RigUnit_MathBoolToggled" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathBoolFlipFlop" ), TEXT( "/Script/ControlRig.RigUnit_MathBoolFlipFlop" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathBoolOnce" ), TEXT( "/Script/ControlRig.RigUnit_MathBoolOnce" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathBoolToFloat" ), TEXT( "/Script/ControlRig.RigUnit_MathBoolToFloat" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathBoolToInteger" ), TEXT( "/Script/ControlRig.RigUnit_MathBoolToInteger" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathColorBase" ), TEXT( "/Script/ControlRig.RigUnit_MathColorBase" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathColorBinaryOp" ), TEXT( "/Script/ControlRig.RigUnit_MathColorBinaryOp" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathColorBinaryAggregateOp" ), TEXT( "/Script/ControlRig.RigUnit_MathColorBinaryAggregateOp" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathColorMake" ), TEXT( "/Script/ControlRig.RigUnit_MathColorMake" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathColorFromFloat" ), TEXT( "/Script/ControlRig.RigUnit_MathColorFromFloat" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathColorFromDouble" ), TEXT( "/Script/ControlRig.RigUnit_MathColorFromDouble" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathColorAdd" ), TEXT( "/Script/ControlRig.RigUnit_MathColorAdd" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathColorSub" ), TEXT( "/Script/ControlRig.RigUnit_MathColorSub" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathColorMul" ), TEXT( "/Script/ControlRig.RigUnit_MathColorMul" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathColorLerp" ), TEXT( "/Script/ControlRig.RigUnit_MathColorLerp" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathDoubleBase" ), TEXT( "/Script/ControlRig.RigUnit_MathDoubleBase" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathDoubleConstant" ), TEXT( "/Script/ControlRig.RigUnit_MathDoubleConstant" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathDoubleUnaryOp" ), TEXT( "/Script/ControlRig.RigUnit_MathDoubleUnaryOp" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathDoubleBinaryOp" ), TEXT( "/Script/ControlRig.RigUnit_MathDoubleBinaryOp" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathDoubleBinaryAggregateOp" ), TEXT( "/Script/ControlRig.RigUnit_MathDoubleBinaryAggregateOp" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathDoubleMake" ), TEXT( "/Script/ControlRig.RigUnit_MathDoubleMake" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathDoubleConstPi" ), TEXT( "/Script/ControlRig.RigUnit_MathDoubleConstPi" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathDoubleConstHalfPi" ), TEXT( "/Script/ControlRig.RigUnit_MathDoubleConstHalfPi" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathDoubleConstTwoPi" ), TEXT( "/Script/ControlRig.RigUnit_MathDoubleConstTwoPi" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathDoubleConstE" ), TEXT( "/Script/ControlRig.RigUnit_MathDoubleConstE" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathDoubleAdd" ), TEXT( "/Script/ControlRig.RigUnit_MathDoubleAdd" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathDoubleSub" ), TEXT( "/Script/ControlRig.RigUnit_MathDoubleSub" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathDoubleMul" ), TEXT( "/Script/ControlRig.RigUnit_MathDoubleMul" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathDoubleDiv" ), TEXT( "/Script/ControlRig.RigUnit_MathDoubleDiv" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathDoubleMod" ), TEXT( "/Script/ControlRig.RigUnit_MathDoubleMod" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathDoubleMin" ), TEXT( "/Script/ControlRig.RigUnit_MathDoubleMin" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathDoubleMax" ), TEXT( "/Script/ControlRig.RigUnit_MathDoubleMax" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathDoublePow" ), TEXT( "/Script/ControlRig.RigUnit_MathDoublePow" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathDoubleSqrt" ), TEXT( "/Script/ControlRig.RigUnit_MathDoubleSqrt" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathDoubleNegate" ), TEXT( "/Script/ControlRig.RigUnit_MathDoubleNegate" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathDoubleAbs" ), TEXT( "/Script/ControlRig.RigUnit_MathDoubleAbs" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathDoubleFloor" ), TEXT( "/Script/ControlRig.RigUnit_MathDoubleFloor" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathDoubleCeil" ), TEXT( "/Script/ControlRig.RigUnit_MathDoubleCeil" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathDoubleRound" ), TEXT( "/Script/ControlRig.RigUnit_MathDoubleRound" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathDoubleToInt" ), TEXT( "/Script/ControlRig.RigUnit_MathDoubleToInt" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathDoubleSign" ), TEXT( "/Script/ControlRig.RigUnit_MathDoubleSign" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathDoubleClamp" ), TEXT( "/Script/ControlRig.RigUnit_MathDoubleClamp" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathDoubleLerp" ), TEXT( "/Script/ControlRig.RigUnit_MathDoubleLerp" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathDoubleRemap" ), TEXT( "/Script/ControlRig.RigUnit_MathDoubleRemap" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathDoubleEquals" ), TEXT( "/Script/ControlRig.RigUnit_MathDoubleEquals" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathDoubleNotEquals" ), TEXT( "/Script/ControlRig.RigUnit_MathDoubleNotEquals" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathDoubleGreater" ), TEXT( "/Script/ControlRig.RigUnit_MathDoubleGreater" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathDoubleLess" ), TEXT( "/Script/ControlRig.RigUnit_MathDoubleLess" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathDoubleGreaterEqual" ), TEXT( "/Script/ControlRig.RigUnit_MathDoubleGreaterEqual" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathDoubleLessEqual" ), TEXT( "/Script/ControlRig.RigUnit_MathDoubleLessEqual" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathDoubleIsNearlyZero" ), TEXT( "/Script/ControlRig.RigUnit_MathDoubleIsNearlyZero" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathDoubleIsNearlyEqual" ), TEXT( "/Script/ControlRig.RigUnit_MathDoubleIsNearlyEqual" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathDoubleDeg" ), TEXT( "/Script/ControlRig.RigUnit_MathDoubleDeg" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathDoubleRad" ), TEXT( "/Script/ControlRig.RigUnit_MathDoubleRad" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathDoubleSin" ), TEXT( "/Script/ControlRig.RigUnit_MathDoubleSin" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathDoubleCos" ), TEXT( "/Script/ControlRig.RigUnit_MathDoubleCos" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathDoubleTan" ), TEXT( "/Script/ControlRig.RigUnit_MathDoubleTan" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathDoubleAsin" ), TEXT( "/Script/ControlRig.RigUnit_MathDoubleAsin" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathDoubleAcos" ), TEXT( "/Script/ControlRig.RigUnit_MathDoubleAcos" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathDoubleAtan" ), TEXT( "/Script/ControlRig.RigUnit_MathDoubleAtan" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathDoubleLawOfCosine" ), TEXT( "/Script/ControlRig.RigUnit_MathDoubleLawOfCosine" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathDoubleExponential" ), TEXT( "/Script/ControlRig.RigUnit_MathDoubleExponential" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathFloatBase" ), TEXT( "/Script/ControlRig.RigUnit_MathFloatBase" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathFloatConstant" ), TEXT( "/Script/ControlRig.RigUnit_MathFloatConstant" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathFloatUnaryOp" ), TEXT( "/Script/ControlRig.RigUnit_MathFloatUnaryOp" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathFloatBinaryOp" ), TEXT( "/Script/ControlRig.RigUnit_MathFloatBinaryOp" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathFloatBinaryAggregateOp" ), TEXT( "/Script/ControlRig.RigUnit_MathFloatBinaryAggregateOp" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathFloatMake" ), TEXT( "/Script/ControlRig.RigUnit_MathFloatMake" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathFloatConstPi" ), TEXT( "/Script/ControlRig.RigUnit_MathFloatConstPi" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathFloatConstHalfPi" ), TEXT( "/Script/ControlRig.RigUnit_MathFloatConstHalfPi" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathFloatConstTwoPi" ), TEXT( "/Script/ControlRig.RigUnit_MathFloatConstTwoPi" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathFloatConstE" ), TEXT( "/Script/ControlRig.RigUnit_MathFloatConstE" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathFloatAdd" ), TEXT( "/Script/ControlRig.RigUnit_MathFloatAdd" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathFloatSub" ), TEXT( "/Script/ControlRig.RigUnit_MathFloatSub" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathFloatMul" ), TEXT( "/Script/ControlRig.RigUnit_MathFloatMul" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathFloatDiv" ), TEXT( "/Script/ControlRig.RigUnit_MathFloatDiv" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathFloatMod" ), TEXT( "/Script/ControlRig.RigUnit_MathFloatMod" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathFloatMin" ), TEXT( "/Script/ControlRig.RigUnit_MathFloatMin" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathFloatMax" ), TEXT( "/Script/ControlRig.RigUnit_MathFloatMax" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathFloatPow" ), TEXT( "/Script/ControlRig.RigUnit_MathFloatPow" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathFloatSqrt" ), TEXT( "/Script/ControlRig.RigUnit_MathFloatSqrt" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathFloatNegate" ), TEXT( "/Script/ControlRig.RigUnit_MathFloatNegate" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathFloatAbs" ), TEXT( "/Script/ControlRig.RigUnit_MathFloatAbs" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathFloatFloor" ), TEXT( "/Script/ControlRig.RigUnit_MathFloatFloor" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathFloatCeil" ), TEXT( "/Script/ControlRig.RigUnit_MathFloatCeil" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathFloatRound" ), TEXT( "/Script/ControlRig.RigUnit_MathFloatRound" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathFloatToInt" ), TEXT( "/Script/ControlRig.RigUnit_MathFloatToInt" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathFloatSign" ), TEXT( "/Script/ControlRig.RigUnit_MathFloatSign" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathFloatClamp" ), TEXT( "/Script/ControlRig.RigUnit_MathFloatClamp" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathFloatLerp" ), TEXT( "/Script/ControlRig.RigUnit_MathFloatLerp" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathFloatRemap" ), TEXT( "/Script/ControlRig.RigUnit_MathFloatRemap" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathFloatEquals" ), TEXT( "/Script/ControlRig.RigUnit_MathFloatEquals" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathFloatNotEquals" ), TEXT( "/Script/ControlRig.RigUnit_MathFloatNotEquals" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathFloatGreater" ), TEXT( "/Script/ControlRig.RigUnit_MathFloatGreater" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathFloatLess" ), TEXT( "/Script/ControlRig.RigUnit_MathFloatLess" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathFloatGreaterEqual" ), TEXT( "/Script/ControlRig.RigUnit_MathFloatGreaterEqual" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathFloatLessEqual" ), TEXT( "/Script/ControlRig.RigUnit_MathFloatLessEqual" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathFloatIsNearlyZero" ), TEXT( "/Script/ControlRig.RigUnit_MathFloatIsNearlyZero" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathFloatIsNearlyEqual" ), TEXT( "/Script/ControlRig.RigUnit_MathFloatIsNearlyEqual" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathFloatSelectBool" ), TEXT( "/Script/ControlRig.RigUnit_MathFloatSelectBool" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathFloatDeg" ), TEXT( "/Script/ControlRig.RigUnit_MathFloatDeg" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathFloatRad" ), TEXT( "/Script/ControlRig.RigUnit_MathFloatRad" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathFloatSin" ), TEXT( "/Script/ControlRig.RigUnit_MathFloatSin" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathFloatCos" ), TEXT( "/Script/ControlRig.RigUnit_MathFloatCos" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathFloatTan" ), TEXT( "/Script/ControlRig.RigUnit_MathFloatTan" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathFloatAsin" ), TEXT( "/Script/ControlRig.RigUnit_MathFloatAsin" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathFloatAcos" ), TEXT( "/Script/ControlRig.RigUnit_MathFloatAcos" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathFloatAtan" ), TEXT( "/Script/ControlRig.RigUnit_MathFloatAtan" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathFloatLawOfCosine" ), TEXT( "/Script/ControlRig.RigUnit_MathFloatLawOfCosine" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathFloatExponential" ), TEXT( "/Script/ControlRig.RigUnit_MathFloatExponential" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathIntBase" ), TEXT( "/Script/ControlRig.RigUnit_MathIntBase" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathIntUnaryOp" ), TEXT( "/Script/ControlRig.RigUnit_MathIntUnaryOp" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathIntBinaryOp" ), TEXT( "/Script/ControlRig.RigUnit_MathIntBinaryOp" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathIntBinaryAggregateOp" ), TEXT( "/Script/ControlRig.RigUnit_MathIntBinaryAggregateOp" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathIntMake" ), TEXT( "/Script/ControlRig.RigUnit_MathIntMake" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathIntAdd" ), TEXT( "/Script/ControlRig.RigUnit_MathIntAdd" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathIntSub" ), TEXT( "/Script/ControlRig.RigUnit_MathIntSub" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathIntMul" ), TEXT( "/Script/ControlRig.RigUnit_MathIntMul" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathIntDiv" ), TEXT( "/Script/ControlRig.RigUnit_MathIntDiv" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathIntMod" ), TEXT( "/Script/ControlRig.RigUnit_MathIntMod" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathIntMin" ), TEXT( "/Script/ControlRig.RigUnit_MathIntMin" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathIntMax" ), TEXT( "/Script/ControlRig.RigUnit_MathIntMax" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathIntPow" ), TEXT( "/Script/ControlRig.RigUnit_MathIntPow" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathIntNegate" ), TEXT( "/Script/ControlRig.RigUnit_MathIntNegate" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathIntAbs" ), TEXT( "/Script/ControlRig.RigUnit_MathIntAbs" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathIntToFloat" ), TEXT( "/Script/ControlRig.RigUnit_MathIntToFloat" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathIntToDouble" ), TEXT( "/Script/ControlRig.RigUnit_MathIntToDouble" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathIntSign" ), TEXT( "/Script/ControlRig.RigUnit_MathIntSign" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathIntClamp" ), TEXT( "/Script/ControlRig.RigUnit_MathIntClamp" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathIntEquals" ), TEXT( "/Script/ControlRig.RigUnit_MathIntEquals" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathIntNotEquals" ), TEXT( "/Script/ControlRig.RigUnit_MathIntNotEquals" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathIntGreater" ), TEXT( "/Script/ControlRig.RigUnit_MathIntGreater" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathIntLess" ), TEXT( "/Script/ControlRig.RigUnit_MathIntLess" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathIntGreaterEqual" ), TEXT( "/Script/ControlRig.RigUnit_MathIntGreaterEqual" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathIntLessEqual" ), TEXT( "/Script/ControlRig.RigUnit_MathIntLessEqual" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathMatrixBase" ), TEXT( "/Script/ControlRig.RigUnit_MathMatrixBase" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathMatrixUnaryOp" ), TEXT( "/Script/ControlRig.RigUnit_MathMatrixUnaryOp" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathMatrixBinaryOp" ), TEXT( "/Script/ControlRig.RigUnit_MathMatrixBinaryOp" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathMatrixBinaryAggregateOp" ), TEXT( "/Script/ControlRig.RigUnit_MathMatrixBinaryAggregateOp" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathMatrixToTransform" ), TEXT( "/Script/ControlRig.RigUnit_MathMatrixToTransform" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathMatrixFromTransform" ), TEXT( "/Script/ControlRig.RigUnit_MathMatrixFromTransform" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathMatrixFromTransformV2" ), TEXT( "/Script/ControlRig.RigUnit_MathMatrixFromTransformV2" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathMatrixToVectors" ), TEXT( "/Script/ControlRig.RigUnit_MathMatrixToVectors" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathMatrixFromVectors" ), TEXT( "/Script/ControlRig.RigUnit_MathMatrixFromVectors" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathMatrixMul" ), TEXT( "/Script/ControlRig.RigUnit_MathMatrixMul" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathMatrixInverse" ), TEXT( "/Script/ControlRig.RigUnit_MathMatrixInverse" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathQuaternionBase" ), TEXT( "/Script/ControlRig.RigUnit_MathQuaternionBase" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathQuaternionUnaryOp" ), TEXT( "/Script/ControlRig.RigUnit_MathQuaternionUnaryOp" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathQuaternionBinaryOp" ), TEXT( "/Script/ControlRig.RigUnit_MathQuaternionBinaryOp" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathQuaternionBinaryAggregateOp" ), TEXT( "/Script/ControlRig.RigUnit_MathQuaternionBinaryAggregateOp" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathQuaternionMake" ), TEXT( "/Script/ControlRig.RigUnit_MathQuaternionMake" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathQuaternionFromAxisAndAngle" ), TEXT( "/Script/ControlRig.RigUnit_MathQuaternionFromAxisAndAngle" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathQuaternionFromEuler" ), TEXT( "/Script/ControlRig.RigUnit_MathQuaternionFromEuler" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathQuaternionFromRotator" ), TEXT( "/Script/ControlRig.RigUnit_MathQuaternionFromRotator" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathQuaternionFromRotatorV2" ), TEXT( "/Script/ControlRig.RigUnit_MathQuaternionFromRotator" ) ) );//V2 doesn't exist in 4.27 apparently
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathQuaternionFromTwoVectors" ), TEXT( "/Script/ControlRig.RigUnit_MathQuaternionFromTwoVectors" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathQuaternionToAxisAndAngle" ), TEXT( "/Script/ControlRig.RigUnit_MathQuaternionToAxisAndAngle" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathQuaternionScale" ), TEXT( "/Script/ControlRig.RigUnit_MathQuaternionScale" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathQuaternionScaleV2" ), TEXT( "/Script/ControlRig.RigUnit_MathQuaternionScaleV2" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathQuaternionToEuler" ), TEXT( "/Script/ControlRig.RigUnit_MathQuaternionToEuler" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathQuaternionToRotator" ), TEXT( "/Script/ControlRig.RigUnit_MathQuaternionToRotator" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathQuaternionMul" ), TEXT( "/Script/ControlRig.RigUnit_MathQuaternionMul" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathQuaternionInverse" ), TEXT( "/Script/ControlRig.RigUnit_MathQuaternionInverse" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathQuaternionSlerp" ), TEXT( "/Script/ControlRig.RigUnit_MathQuaternionSlerp" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathQuaternionEquals" ), TEXT( "/Script/ControlRig.RigUnit_MathQuaternionEquals" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathQuaternionNotEquals" ), TEXT( "/Script/ControlRig.RigUnit_MathQuaternionNotEquals" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathQuaternionSelectBool" ), TEXT( "/Script/ControlRig.RigUnit_MathQuaternionSelectBool" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathQuaternionDot" ), TEXT( "/Script/ControlRig.RigUnit_MathQuaternionDot" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathQuaternionUnit" ), TEXT( "/Script/ControlRig.RigUnit_MathQuaternionUnit" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathQuaternionRotateVector" ), TEXT( "/Script/ControlRig.RigUnit_MathQuaternionRotateVector" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathQuaternionGetAxis" ), TEXT( "/Script/ControlRig.RigUnit_MathQuaternionGetAxis" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathQuaternionSwingTwist" ), TEXT( "/Script/ControlRig.RigUnit_MathQuaternionSwingTwist" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathQuaternionRotationOrder" ), TEXT( "/Script/ControlRig.RigUnit_MathQuaternionRotationOrder" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathQuaternionMakeRelative" ), TEXT( "/Script/ControlRig.RigUnit_MathQuaternionMakeRelative" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathQuaternionMakeAbsolute" ), TEXT( "/Script/ControlRig.RigUnit_MathQuaternionMakeAbsolute" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathQuaternionMirrorTransform" ), TEXT( "/Script/ControlRig.RigUnit_MathQuaternionMirrorTransform" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathRBFQuatWeightFunctor" ), TEXT( "/Script/ControlRig.RigUnit_MathRBFQuatWeightFunctor" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathRBFVectorWeightFunctor" ), TEXT( "/Script/ControlRig.RigUnit_MathRBFVectorWeightFunctor" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathRBFInterpolateQuatWorkData" ), TEXT( "/Script/ControlRig.RigUnit_MathRBFInterpolateQuatWorkData" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathRBFInterpolateVectorWorkData" ), TEXT( "/Script/ControlRig.RigUnit_MathRBFInterpolateVectorWorkData" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathRBFInterpolateBase" ), TEXT( "/Script/ControlRig.RigUnit_MathRBFInterpolateBase" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathRBFInterpolateQuatBase" ), TEXT( "/Script/ControlRig.RigUnit_MathRBFInterpolateQuatBase" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathRBFInterpolateVectorBase" ), TEXT( "/Script/ControlRig.RigUnit_MathRBFInterpolateVectorBase" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathRBFInterpolateQuatFloat" ), TEXT( "/Script/ControlRig.RigUnit_MathRBFInterpolateQuatFloat" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathRBFInterpolateQuatVector" ), TEXT( "/Script/ControlRig.RigUnit_MathRBFInterpolateQuatVector" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathRBFInterpolateQuatColor" ), TEXT( "/Script/ControlRig.RigUnit_MathRBFInterpolateQuatColor" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathRBFInterpolateQuatQuat" ), TEXT( "/Script/ControlRig.RigUnit_MathRBFInterpolateQuatQuat" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathRBFInterpolateQuatXform" ), TEXT( "/Script/ControlRig.RigUnit_MathRBFInterpolateQuatXform" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathRBFInterpolateVectorFloat" ), TEXT( "/Script/ControlRig.RigUnit_MathRBFInterpolateVectorFloat" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathRBFInterpolateVectorVector" ), TEXT( "/Script/ControlRig.RigUnit_MathRBFInterpolateVectorVector" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathRBFInterpolateVectorColor" ), TEXT( "/Script/ControlRig.RigUnit_MathRBFInterpolateVectorColor" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathRBFInterpolateVectorQuat" ), TEXT( "/Script/ControlRig.RigUnit_MathRBFInterpolateVectorQuat" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathRBFInterpolateVectorXform" ), TEXT( "/Script/ControlRig.RigUnit_MathRBFInterpolateVectorXform" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathTransformBase" ), TEXT( "/Script/ControlRig.RigUnit_MathTransformBase" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathTransformMutableBase" ), TEXT( "/Script/ControlRig.RigUnit_MathTransformMutableBase" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathTransformUnaryOp" ), TEXT( "/Script/ControlRig.RigUnit_MathTransformUnaryOp" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathTransformBinaryOp" ), TEXT( "/Script/ControlRig.RigUnit_MathTransformBinaryOp" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathTransformBinaryAggregateOp" ), TEXT( "/Script/ControlRig.RigUnit_MathTransformBinaryAggregateOp" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathTransformMake" ), TEXT( "/Script/ControlRig.RigUnit_MathTransformMake" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathTransformFromEulerTransform" ), TEXT( "/Script/ControlRig.RigUnit_MathTransformFromEulerTransform" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathTransformFromEulerTransformV2" ), TEXT( "/Script/ControlRig.RigUnit_MathTransformFromEulerTransformV2" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathTransformToEulerTransform" ), TEXT( "/Script/ControlRig.RigUnit_MathTransformToEulerTransform" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathTransformMul" ), TEXT( "/Script/ControlRig.RigUnit_MathTransformMul" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathTransformMakeRelative" ), TEXT( "/Script/ControlRig.RigUnit_MathTransformMakeRelative" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathTransformMakeAbsolute" ), TEXT( "/Script/ControlRig.RigUnit_MathTransformMakeAbsolute" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathTransformAccumulateArray" ), TEXT( "/Script/ControlRig.RigUnit_MathTransformAccumulateArray" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathTransformInverse" ), TEXT( "/Script/ControlRig.RigUnit_MathTransformInverse" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathTransformLerp" ), TEXT( "/Script/ControlRig.RigUnit_MathTransformLerp" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathTransformSelectBool" ), TEXT( "/Script/ControlRig.RigUnit_MathTransformSelectBool" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathTransformRotateVector" ), TEXT( "/Script/ControlRig.RigUnit_MathTransformRotateVector" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathTransformTransformVector" ), TEXT( "/Script/ControlRig.RigUnit_MathTransformTransformVector" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathTransformFromSRT" ), TEXT( "/Script/ControlRig.RigUnit_MathTransformFromSRT" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathTransformArrayToSRT" ), TEXT( "/Script/ControlRig.RigUnit_MathTransformArrayToSRT" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathTransformClampSpatially" ), TEXT( "/Script/ControlRig.RigUnit_MathTransformClampSpatially" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathTransformMirrorTransform" ), TEXT( "/Script/ControlRig.RigUnit_MathTransformMirrorTransform" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathVectorBase" ), TEXT( "/Script/ControlRig.RigUnit_MathVectorBase" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathVectorUnaryOp" ), TEXT( "/Script/ControlRig.RigUnit_MathVectorUnaryOp" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathVectorBinaryOp" ), TEXT( "/Script/ControlRig.RigUnit_MathVectorBinaryOp" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathVectorBinaryAggregateOp" ), TEXT( "/Script/ControlRig.RigUnit_MathVectorBinaryAggregateOp" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathVectorMake" ), TEXT( "/Script/ControlRig.RigUnit_MathVectorMake" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathVectorFromFloat" ), TEXT( "/Script/ControlRig.RigUnit_MathVectorFromFloat" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathVectorFromDouble" ), TEXT( "/Script/ControlRig.RigUnit_MathVectorFromDouble" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathVectorAdd" ), TEXT( "/Script/ControlRig.RigUnit_MathVectorAdd" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathVectorSub" ), TEXT( "/Script/ControlRig.RigUnit_MathVectorSub" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathVectorMul" ), TEXT( "/Script/ControlRig.RigUnit_MathVectorMul" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathVectorScale" ), TEXT( "/Script/ControlRig.RigUnit_MathVectorScale" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathVectorDiv" ), TEXT( "/Script/ControlRig.RigUnit_MathVectorDiv" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathVectorMod" ), TEXT( "/Script/ControlRig.RigUnit_MathVectorMod" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathVectorMin" ), TEXT( "/Script/ControlRig.RigUnit_MathVectorMin" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathVectorMax" ), TEXT( "/Script/ControlRig.RigUnit_MathVectorMax" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathVectorNegate" ), TEXT( "/Script/ControlRig.RigUnit_MathVectorNegate" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathVectorAbs" ), TEXT( "/Script/ControlRig.RigUnit_MathVectorAbs" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathVectorFloor" ), TEXT( "/Script/ControlRig.RigUnit_MathVectorFloor" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathVectorCeil" ), TEXT( "/Script/ControlRig.RigUnit_MathVectorCeil" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathVectorRound" ), TEXT( "/Script/ControlRig.RigUnit_MathVectorRound" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathVectorSign" ), TEXT( "/Script/ControlRig.RigUnit_MathVectorSign" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathVectorClamp" ), TEXT( "/Script/ControlRig.RigUnit_MathVectorClamp" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathVectorLerp" ), TEXT( "/Script/ControlRig.RigUnit_MathVectorLerp" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathVectorRemap" ), TEXT( "/Script/ControlRig.RigUnit_MathVectorRemap" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathVectorEquals" ), TEXT( "/Script/ControlRig.RigUnit_MathVectorEquals" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathVectorNotEquals" ), TEXT( "/Script/ControlRig.RigUnit_MathVectorNotEquals" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathVectorIsNearlyZero" ), TEXT( "/Script/ControlRig.RigUnit_MathVectorIsNearlyZero" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathVectorIsNearlyEqual" ), TEXT( "/Script/ControlRig.RigUnit_MathVectorIsNearlyEqual" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathVectorSelectBool" ), TEXT( "/Script/ControlRig.RigUnit_MathVectorSelectBool" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathVectorDeg" ), TEXT( "/Script/ControlRig.RigUnit_MathVectorDeg" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathVectorRad" ), TEXT( "/Script/ControlRig.RigUnit_MathVectorRad" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathVectorLengthSquared" ), TEXT( "/Script/ControlRig.RigUnit_MathVectorLengthSquared" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathVectorLength" ), TEXT( "/Script/ControlRig.RigUnit_MathVectorLength" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathVectorDistance" ), TEXT( "/Script/ControlRig.RigUnit_MathVectorDistance" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathVectorCross" ), TEXT( "/Script/ControlRig.RigUnit_MathVectorCross" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathVectorDot" ), TEXT( "/Script/ControlRig.RigUnit_MathVectorDot" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathVectorUnit" ), TEXT( "/Script/ControlRig.RigUnit_MathVectorUnit" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathVectorSetLength" ), TEXT( "/Script/ControlRig.RigUnit_MathVectorSetLength" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathVectorClampLength" ), TEXT( "/Script/ControlRig.RigUnit_MathVectorClampLength" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathVectorMirror" ), TEXT( "/Script/ControlRig.RigUnit_MathVectorMirror" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathVectorAngle" ), TEXT( "/Script/ControlRig.RigUnit_MathVectorAngle" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathVectorParallel" ), TEXT( "/Script/ControlRig.RigUnit_MathVectorParallel" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathVectorOrthogonal" ), TEXT( "/Script/ControlRig.RigUnit_MathVectorOrthogonal" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathVectorBezierFourPoint" ), TEXT( "/Script/ControlRig.RigUnit_MathVectorBezierFourPoint" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathVectorMakeBezierFourPoint" ), TEXT( "/Script/ControlRig.RigUnit_MathVectorMakeBezierFourPoint" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathVectorClampSpatially" ), TEXT( "/Script/ControlRig.RigUnit_MathVectorClampSpatially" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathIntersectPlane" ), TEXT( "/Script/ControlRig.RigUnit_MathIntersectPlane" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathDistanceToPlane" ), TEXT( "/Script/ControlRig.RigUnit_MathDistanceToPlane" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathVectorMakeRelative" ), TEXT( "/Script/ControlRig.RigUnit_MathVectorMakeRelative" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathVectorMakeAbsolute" ), TEXT( "/Script/ControlRig.RigUnit_MathVectorMakeAbsolute" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_MathVectorMirrorTransform" ), TEXT( "/Script/ControlRig.RigUnit_MathVectorMirrorTransform" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_NoiseFloat" ), TEXT( "/Script/ControlRig.RigUnit_NoiseFloat" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_NoiseDouble" ), TEXT( "/Script/ControlRig.RigUnit_NoiseDouble" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_NoiseVector" ), TEXT( "/Script/ControlRig.RigUnit_NoiseVector" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_NoiseVector2" ), TEXT( "/Script/ControlRig.RigUnit_NoiseVector2" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_RandomFloat" ), TEXT( "/Script/ControlRig.RigUnit_RandomFloat" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_RandomVector" ), TEXT( "/Script/ControlRig.RigUnit_RandomVector" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Enum, TEXT( "/Script/RigVM.ERigVMSimPointIntegrateType" ), TEXT( "ECRSimPointIntegrateType" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMSimPoint" ), TEXT( "/Script/ControlRig.CRSimPoint" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_ForLoopCount" ), TEXT( "/Script/ControlRig.RigUnit_ForLoopCount" ) ) );
		//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_Sequence" ), TEXT( "/Script/ControlRig.RigUnit_SequenceAggregate" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_Sequence" ), TEXT( "/Script/ControlRig.RigUnit_SequenceExecution" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_UserDefinedEvent" ), TEXT( "/Script/ControlRig.RigUnit_UserDefinedEvent" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_AccumulateBase" ), TEXT( "/Script/ControlRig.RigUnit_AccumulateBase" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_AccumulateFloatAdd" ), TEXT( "/Script/ControlRig.RigUnit_AccumulateFloatAdd" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_AccumulateVectorAdd" ), TEXT( "/Script/ControlRig.RigUnit_AccumulateVectorAdd" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_AccumulateFloatMul" ), TEXT( "/Script/ControlRig.RigUnit_AccumulateFloatMul" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_AccumulateVectorMul" ), TEXT( "/Script/ControlRig.RigUnit_AccumulateVectorMul" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_AccumulateQuatMul" ), TEXT( "/Script/ControlRig.RigUnit_AccumulateQuatMul" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_AccumulateTransformMul" ), TEXT( "/Script/ControlRig.RigUnit_AccumulateTransformMul" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_AccumulateFloatLerp" ), TEXT( "/Script/ControlRig.RigUnit_AccumulateFloatLerp" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_AccumulateVectorLerp" ), TEXT( "/Script/ControlRig.RigUnit_AccumulateVectorLerp" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_AccumulateQuatLerp" ), TEXT( "/Script/ControlRig.RigUnit_AccumulateQuatLerp" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_AccumulateTransformLerp" ), TEXT( "/Script/ControlRig.RigUnit_AccumulateTransformLerp" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_AccumulateFloatRange" ), TEXT( "/Script/ControlRig.RigUnit_AccumulateFloatRange" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_AccumulateVectorRange" ), TEXT( "/Script/ControlRig.RigUnit_AccumulateVectorRange" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_AlphaInterp" ), TEXT( "/Script/ControlRig.RigUnit_AlphaInterp" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_AlphaInterpVector" ), TEXT( "/Script/ControlRig.RigUnit_AlphaInterpVector" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_AlphaInterpQuat" ), TEXT( "/Script/ControlRig.RigUnit_AlphaInterpQuat" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_DeltaFromPreviousFloat" ), TEXT( "/Script/ControlRig.RigUnit_DeltaFromPreviousFloat" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_DeltaFromPreviousVector" ), TEXT( "/Script/ControlRig.RigUnit_DeltaFromPreviousVector" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_DeltaFromPreviousQuat" ), TEXT( "/Script/ControlRig.RigUnit_DeltaFromPreviousQuat" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_DeltaFromPreviousTransform" ), TEXT( "/Script/ControlRig.RigUnit_DeltaFromPreviousTransform" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_KalmanFloat" ), TEXT( "/Script/ControlRig.RigUnit_KalmanFloat" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_KalmanVector" ), TEXT( "/Script/ControlRig.RigUnit_KalmanVector" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_KalmanTransform" ), TEXT( "/Script/ControlRig.RigUnit_KalmanTransform" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_SimBase" ), TEXT( "/Script/ControlRig.RigUnit_SimBase" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_SimBaseMutable" ), TEXT( "/Script/ControlRig.RigUnit_SimBaseMutable" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_Timeline" ), TEXT( "/Script/ControlRig.RigUnit_Timeline" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_TimeLoop" ), TEXT( "/Script/ControlRig.RigUnit_TimeLoop" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_TimeOffsetFloat" ), TEXT( "/Script/ControlRig.RigUnit_TimeOffsetFloat" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_TimeOffsetVector" ), TEXT( "/Script/ControlRig.RigUnit_TimeOffsetVector" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_TimeOffsetTransform" ), TEXT( "/Script/ControlRig.RigUnit_TimeOffsetTransform" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_VerletIntegrateVector" ), TEXT( "/Script/ControlRig.RigUnit_VerletIntegrateVector" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_DebugPoint" ), TEXT( "/Script/ControlRig.RigUnit_DebugPoint" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_DebugPointMutable" ), TEXT( "/Script/ControlRig.RigUnit_DebugPointMutable" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_AnimBase" ), TEXT( "/Script/ControlRig.RigUnit_AnimBase" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_AnimEasingType" ), TEXT( "/Script/ControlRig.RigUnit_AnimEasingType" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_AnimEasing" ), TEXT( "/Script/ControlRig.RigUnit_AnimEasing" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_AnimEvalRichCurve" ), TEXT( "/Script/ControlRig.RigUnit_AnimEvalRichCurve" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_AnimRichCurve" ), TEXT( "/Script/ControlRig.RigUnit_AnimRichCurve" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_GetDeltaTime" ), TEXT( "/Script/ControlRig.RigUnit_GetDeltaTime" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_GetWorldTime" ), TEXT( "/Script/ControlRig.RigUnit_GetWorldTime" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_FramesToSeconds" ), TEXT( "/Script/ControlRig.RigUnit_FramesToSeconds" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMFunction_SecondsToFrames" ), TEXT( "/Script/ControlRig.RigUnit_SecondsToFrames" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVMDeveloper.RigVMEdGraphDisplaySettings" ), TEXT( "/Script/ControlRig.RigGraphDisplaySettings" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVMDeveloper.RigVMPythonSettings" ), TEXT( "/Script/ControlRig.ControlRigPythonSettings" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVMDeveloper.RigVMOldPublicFunctionArg" ), TEXT( "/Script/ControlRig.ControlRigPublicFunctionArg" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVMDeveloper.RigVMOldPublicFunctionData" ), TEXT( "/Script/ControlRig.ControlRigPublicFunctionData" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/RigVM.NameSpacedUserData" ), TEXT( "/Script/ControlRig.NameSpacedUserData" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/RigVM.DataAssetLink" ), TEXT( "/Script/ControlRig.DataAssetLink" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/ControlRig.ControlRigReplayVariable" ), TEXT( "/Script/ControlRig.ControlRigTestDataVariable" ) ) );
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Enum, TEXT( "/Script/ControlRig.EControlRigReplayPlaybackMode" ), TEXT( "EControlRigTestDataPlaybackMode" ) ) );
	}
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/RigVM.RigVMExecuteContext" ), TEXT( "/Script/ControlRig.ControlRigExecuteContext" ) ) );

	//EnhancedInput
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Function, TEXT( "/Script/EnhancedInput.EnhancedInputSubsystemInterface.K2_AddPlayerMappedKeyInSlot" ), TEXT( "/Script/EnhancedInput.EnhancedInputSubsystemInterface.AddPlayerMappedKey" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Function, TEXT( "/Script/EnhancedInput.EnhancedInputSubsystemInterface.K2_RemovePlayerMappedKeyInSlot" ), TEXT( "/Script/EnhancedInput.EnhancedInputSubsystemInterface.RemovePlayerMappedKey" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Function, TEXT( "/Script/EnhancedInput.EnhancedInputSubsystemInterface.K2_GetPlayerMappedKeyInSlot" ), TEXT( "/Script/EnhancedInput.EnhancedInputSubsystemInterface.GetPlayerMappedKey" ) ) );
#endif
#if (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION <= 27) || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION <= 2)
	//5.3
	if (!RedirectExists( TEXT( "BreakVector" ), Redirects ))
		Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Function, TEXT( "BreakVector" ), TEXT( "KismetMathLibrary.BreakVector3f" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/FieldNotification.FieldNotificationId" ), TEXT( "/Script/UMG.FieldNotificationId" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/FieldNotification.NotifyFieldValueChanged" ), TEXT( "/Script/UMG.NotifyFieldValueChanged" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/ScriptableEditorWidgets.DetailsView" ), TEXT( "/Script/UMGEditor.DetailsView" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/ScriptableEditorWidgets.SinglePropertyView" ), TEXT( "/Script/UMGEditor.SinglePropertyView" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/ScriptableEditorWidgets.PropertyViewBase" ), TEXT( "/Script/UMGEditor.PropertyViewBase" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/Engine.AnimNode_StateResult" ), TEXT( "/Script/AnimGraphRuntime.AnimNode_StateResult" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/XRBase.HeadMountedDisplayFunctionLibrary" ), TEXT( "HeadMountedDisplayFunctionLibrary" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/XRBase.MotionTrackedDeviceFunctionLibrary" ), TEXT( "MotionTrackedDeviceFunctionLibrary" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/XRBase.VRNotificationsComponent" ), TEXT( "VRNotificationsComponent" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/XRBase.XRAssetFunctionLibrary" ), TEXT( "XRAssetFunctionLibrary" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/XRBase.AsyncTask_LoadXRDeviceVisComponent" ), TEXT( "AsyncTask_LoadXRDeviceVisComponent" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/XRBase.XRDeviceVisualizationComponent" ), TEXT( "XRDeviceVisualizationComponent" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/XRBase.XRLoadingScreenFunctionLibrary" ), TEXT( "XRLoadingScreenFunctionLibrary" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Enum, TEXT( "/Script/Engine.ECameraShakePatternUpdateResultFlags" ), TEXT( "ECameraShakeUpdateResultFlags" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/Engine.CameraShakePatternStartParams" ), TEXT( "CameraShakeStartParams" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/Engine.CameraShakePatternUpdateParams" ), TEXT( "CameraShakeUpdateParams" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/Engine.CameraShakePatternScrubParams" ), TEXT( "CameraShakeScrubParams" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/Engine.CameraShakePatternStopParams" ), TEXT( "CameraShakeStopParams" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/Engine.CameraShakePatternUpdateResult" ), TEXT( "CameraShakeUpdateResult" ) ) );

	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/Engine.MaterialExpressionExponential" ), TEXT( "/Script/InterchangeImport.MaterialExpressionMaterialXExponential" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/Engine.MaterialExpressionHsvToRgb" ), TEXT( "/Script/InterchangeImport.MaterialExpressionMaterialXHsvToRgb" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/Engine.MaterialExpressionLength" ), TEXT( "/Script/InterchangeImport.MaterialExpressionMaterialXLength" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/Engine.MaterialExpressionLogarithm" ), TEXT( "/Script/InterchangeImport.MaterialExpressionMaterialXLogarithm" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/Engine.MaterialExpressionRgbToHsv" ), TEXT( "/Script/InterchangeImport.MaterialExpressionMaterialXRgbToHsv" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/Engine.MaterialExpressionExponential" ), TEXT( "/Script/InterchangeImport.MaterialExpressionExponential" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/Engine.MaterialExpressionHsvToRgb" ), TEXT( "/Script/InterchangeImport.MaterialExpressionHsvToRgb" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/Engine.MaterialExpressionLength" ), TEXT( "/Script/InterchangeImport.MaterialExpressionLength" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/Engine.MaterialExpressionLogarithm" ), TEXT( "/Script/InterchangeImport.MaterialExpressionLogarithm" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/Engine.MaterialExpressionRgbToHsv" ), TEXT( "/Script/InterchangeImport.MaterialExpressionRgbToHsv" ) ) );

	//BaseInterchange.ini
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/InterchangeFactoryNodes.InterchangePhysicalCameraFactoryNode" ), TEXT( "/Script/InterchangeFactoryNodes.InterchangeCineCameraFactoryNode" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/InterchangeImport.MaterialExpressionMaterialXAppend3Vector" ), TEXT( "/Script/InterchangeImport.MaterialExpressionAppend3Vector" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/InterchangeImport.MaterialExpressionMaterialXAppend4Vector" ), TEXT( "/Script/InterchangeImport.MaterialExpressionAppend4Vector" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/InterchangeImport.MaterialExpressionMaterialXBurn" ), TEXT( "/Script/InterchangeImport.MaterialExpressionBurn" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/InterchangeImport.MaterialExpressionMaterialXDifference" ), TEXT( "/Script/InterchangeImport.MaterialExpressionDifference" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/InterchangeImport.MaterialExpressionMaterialXDisjointOver" ), TEXT( "/Script/InterchangeImport.MaterialExpressionDisjointOver" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/InterchangeImport.MaterialExpressionMaterialXDodge" ), TEXT( "/Script/InterchangeImport.MaterialExpressionDodge" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/InterchangeImport.MaterialExpressionMaterialXFractal3D" ), TEXT( "/Script/InterchangeImport.MaterialExpressionFractal3D" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/InterchangeImport.MaterialExpressionMaterialXIn" ), TEXT( "/Script/InterchangeImport.MaterialExpressionIn" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/InterchangeImport.MaterialExpressionMaterialXLuminance" ), TEXT( "/Script/InterchangeImport.MaterialExpressionLuminance" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/InterchangeImport.MaterialExpressionMaterialXMask" ), TEXT( "/Script/InterchangeImport.MaterialExpressionMask" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/InterchangeImport.MaterialExpressionMaterialXMatte" ), TEXT( "/Script/InterchangeImport.MaterialExpressionMatte" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/InterchangeImport.MaterialExpressionMaterialXMinus" ), TEXT( "/Script/InterchangeImport.MaterialExpressionMinus" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/InterchangeImport.MaterialExpressionMaterialXOut" ), TEXT( "/Script/InterchangeImport.MaterialExpressionOut" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/InterchangeImport.MaterialExpressionMaterialXOver" ), TEXT( "/Script/InterchangeImport.MaterialExpressionOver" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/InterchangeImport.MaterialExpressionMaterialXOverlay" ), TEXT( "/Script/InterchangeImport.MaterialExpressionOverlay" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/InterchangeImport.MaterialExpressionMaterialXPlace2D" ), TEXT( "/Script/InterchangeImport.MaterialExpressionPlace2D" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/InterchangeImport.MaterialExpressionMaterialXPlus" ), TEXT( "/Script/InterchangeImport.MaterialExpressionPlus" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/InterchangeImport.MaterialExpressionMaterialXPremult" ), TEXT( "/Script/InterchangeImport.MaterialExpressionPremult" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/InterchangeImport.MaterialExpressionMaterialXRamp4" ), TEXT( "/Script/InterchangeImport.MaterialExpressionRamp4" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/InterchangeImport.MaterialExpressionMaterialXRampLeftRight" ), TEXT( "/Script/InterchangeImport.MaterialExpressionRampLeftRight" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/InterchangeImport.MaterialExpressionMaterialXRampTopBottom" ), TEXT( "/Script/InterchangeImport.MaterialExpressionRampTopBottom" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/InterchangeImport.MaterialExpressionMaterialXRemap" ), TEXT( "/Script/InterchangeImport.MaterialExpressionRemap" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/InterchangeImport.MaterialExpressionMaterialXRotate2D" ), TEXT( "/Script/InterchangeImport.MaterialExpressionRotate2D" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/InterchangeImport.MaterialExpressionMaterialXScreen" ), TEXT( "/Script/InterchangeImport.MaterialExpressionScreen" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/InterchangeImport.MaterialExpressionMaterialXSplitLeftRight" ), TEXT( "/Script/InterchangeImport.MaterialExpressionSplitLeftRight" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/InterchangeImport.MaterialExpressionMaterialXSplitTopBottom" ), TEXT( "/Script/InterchangeImport.MaterialExpressionSplitTopBottom" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/InterchangeImport.MaterialExpressionMaterialXTextureSampleParameterBlur" ), TEXT( "/Script/InterchangeImport.MaterialExpressionTextureSampleParameterBlur" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/InterchangeImport.MaterialExpressionMaterialXUnpremult" ), TEXT( "/Script/InterchangeImport.MaterialExpressionUnpremult" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Enum, TEXT( "EMaterialXTextureSampleBlurFilter" ), TEXT( "ETextureSampleBlurFilter" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Enum, TEXT( "EMaterialXLuminanceMode" ), TEXT( "ELuminanceMode" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Enum, TEXT( "/Script/InterchangeCommon.EInterchangeMaterialXShaders>" ), TEXT( "/Script/InterchangePipelines.EInterchangeMaterialXShaders>" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Enum, TEXT( "/Script/InterchangeCommon.EInterchangeMaterialXBSDF>" ), TEXT( "/Script/InterchangePipelines.EInterchangeMaterialXBSDF>" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Enum, TEXT( "/Script/InterchangeCommon.EInterchangeMaterialXEDF>" ), TEXT( "/Script/InterchangePipelines.EInterchangeMaterialXEDF>" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Enum, TEXT( "/Script/InterchangeCommon.EInterchangeMaterialXVDF>" ), TEXT( "/Script/InterchangePipelines.EInterchangeMaterialXVDF>" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/InterchangeImport.InterchangeLevelSequenceFactory" ), TEXT( "/Script/InterchangeImport.InterchangeAnimationTrackSetFactory" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/InterchangeFactoryNodes.InterchangeLevelSequenceFactoryNode" ), TEXT( "/Script/InterchangeFactoryNodes.InterchangeAnimationTrackSetFactoryNode" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/Materials/PBRSurfaceMaterial_MR.PBRSurfaceMaterial_MR" ), TEXT( "/Interchange/Materials/PBRSurfaceMaterial.PBRSurfaceMaterial" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/Materials/ClearCoatMaterial_MR.ClearCoatMaterial_MR" ), TEXT( "/Interchange/Materials/ClearCoatMaterial.ClearCoatMaterial" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/Materials/SheenMaterial_MR.SheenMaterial_MR" ), TEXT( "/Interchange/Materials/SheenMaterial.SheenMaterial" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/Materials/SubsurfaceMaterial_MR.SubsurfaceMaterial_MR" ), TEXT( "/Interchange/Materials/SubsurfaceMaterial.SubsurfaceMaterial" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/Materials/ThinTranslucentMaterial_MR.ThinTranslucentMaterial_MR" ), TEXT( "/Interchange/Materials/ThinTranslucentMaterial.ThinTranslucentMaterial" ) ) );

	//Niagara
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "NiagaraParameterBinding.ResolvedParameter" ), TEXT( "NiagaraParameterBinding.Parameter" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/Niagara.NiagaraSystemScalabilitySettings" ), TEXT( "NiagaraScalabilitySettings" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/Niagara.NiagaraSystemScalabilityOverride" ), TEXT( "NiagaraScalabilityOverrides" ) ) );
#endif
#if (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION <= 27) || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION <= 3)
	//5.4
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/Engine.MaterialExpressionSubstrateBSDF" ), TEXT( "/Script/Engine.MaterialExpressionStrataBSDF" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/Engine.MaterialExpressionSubstrateShadingModels" ), TEXT( "/Script/Engine.MaterialExpressionStrataLegacyConversion" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/Engine.MaterialExpressionSubstrateSlabBSDF" ), TEXT( "/Script/Engine.MaterialExpressionStrataSlabBSDF" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/Engine.MaterialExpressionSubstrateSimpleClearCoatBSDF" ), TEXT( "/Script/Engine.MaterialExpressionStrataSimpleClearCoatBSDF" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/Engine.MaterialExpressionSubstrateVolumetricFogCloudBSDF" ), TEXT( "/Script/Engine.MaterialExpressionStrataVolumetricFogCloudBSDF" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/Engine.MaterialExpressionSubstrateUnlitBSDF" ), TEXT( "/Script/Engine.MaterialExpressionStrataUnlitBSDF" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/Engine.MaterialExpressionSubstrateHairBSDF" ), TEXT( "/Script/Engine.MaterialExpressionStrataHairBSDF" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/Engine.MaterialExpressionSubstrateEyeBSDF" ), TEXT( "/Script/Engine.MaterialExpressionStrataEyeBSDF" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/Engine.MaterialExpressionSubstrateSingleLayerWaterBSDF" ), TEXT( "/Script/Engine.MaterialExpressionStrataSingleLayerWaterBSDF" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/Engine.MaterialExpressionSubstrateLightFunction" ), TEXT( "/Script/Engine.MaterialExpressionStrataLightFunction" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/Engine.MaterialExpressionSubstratePostProcess" ), TEXT( "/Script/Engine.MaterialExpressionStrataPostProcess" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/Engine.MaterialExpressionSubstrateUI" ), TEXT( "/Script/Engine.MaterialExpressionStrataUI" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/Engine.MaterialExpressionSubstrateConvertToDecal" ), TEXT( "/Script/Engine.MaterialExpressionStrataConvertToDecal" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/Engine.MaterialExpressionSubstrateHorizontalMixing" ), TEXT( "/Script/Engine.MaterialExpressionStrataHorizontalMixing" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/Engine.MaterialExpressionSubstrateVerticalLayering" ), TEXT( "/Script/Engine.MaterialExpressionStrataVerticalLayering" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/Engine.MaterialExpressionSubstrateAdd" ), TEXT( "/Script/Engine.MaterialExpressionStrataAdd" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/Engine.MaterialExpressionSubstrateWeight" ), TEXT( "/Script/Engine.MaterialExpressionStrataWeight" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/Engine.MaterialExpressionSubstrateUtilityBase" ), TEXT( "/Script/Engine.MaterialExpressionStrataUtilityBase" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/Engine.MaterialExpressionSubstrateTransmittanceToMFP" ), TEXT( "/Script/Engine.MaterialExpressionStrataTransmittanceToMFP" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/Engine.MaterialExpressionSubstrateMetalnessToDiffuseAlbedoF0" ), TEXT( "/Script/Engine.MaterialExpressionStrataMetalnessToDiffuseAlbedoF0" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/Engine.MaterialExpressionSubstrateHazinessToSecondaryRoughness" ), TEXT( "/Script/Engine.MaterialExpressionStrataHazinessToSecondaryRoughness" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/Engine.MaterialExpressionSubstrateThinFilm" ), TEXT( "/Script/Engine.MaterialExpressionStrataThinFilm" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/Engine.SubstrateMaterialInput" ), TEXT( "/Script/Engine.StrataMaterialInput" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/Engine/Functions/Substrate/SMF_UE4Legacy.SMF_UE4Legacy" ), TEXT( "/Engine/Functions/Strata/SMF_UE4Disney.SMF_UE4Disney" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/Engine/Functions/Substrate/SMF_UE4Legacy.SMF_UE4Legacy" ), TEXT( "/Engine/Functions/Substrate/SMF_UE4Disney.SMF_UE4Disney" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Enum, TEXT( "/Script/CoreUObject.EDataValidationUsecase" ), TEXT( "/Script/DataValidation.EDataValidationUsecase" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Function, TEXT( "EditorValidatorBase.K2_ValidateLoadedAsset" ), TEXT( "EditorValidatorBase.ValidateLoadedAsset" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Function, TEXT( "EditorValidatorBase.K2_CanValidate" ), TEXT( "EditorValidatorBase.CanValidate" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Function, TEXT( "EditorValidatorBase.K2_CanValidateAsset" ), TEXT( "EditorValidatorBase.CanValidateAsset" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Enum, TEXT( "/Script/Engine.ESendLevelControlMethod" ), TEXT( "/Script/Engine.ESubmixSendMethod" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "AttenuationSubmixSendSettings.SoundSubmix" ), TEXT( "AttenuationSubmixSendSettings.Submix" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "AttenuationSubmixSendSettings.SendLevelControlMethod" ), TEXT( "AttenuationSubmixSendSettings.SubmixSendMethod" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "AttenuationSubmixSendSettings.MinSendLevel" ), TEXT( "AttenuationSubmixSendSettings.SubmixSendLevelMin" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "AttenuationSubmixSendSettings.MaxSendLevel" ), TEXT( "AttenuationSubmixSendSettings.SubmixSendLevelMax" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "AttenuationSubmixSendSettings.MinSendDistance" ), TEXT( "AttenuationSubmixSendSettings.SubmixSendDistanceMin" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "AttenuationSubmixSendSettings.MaxSendDistance" ), TEXT( "AttenuationSubmixSendSettings.SubmixSendDistanceMax" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "AttenuationSubmixSendSettings.SendLevel" ), TEXT( "AttenuationSubmixSendSettings.ManualSubmixSendLevel" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "AttenuationSubmixSendSettings.CustomSendLevelCurve" ), TEXT( "AttenuationSubmixSendSettings.CustomSubmixSendCurve" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Enum, TEXT( "" ), TEXT( "ETargetingTraceType" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Enum, TEXT( "/Script/MovieScene.EMovieSceneTimeUnit" ), TEXT( "/Script/SequencerScripting.ESequenceTimeUnit" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/Engine.WorldPartitionRuntimeCellObjectMapping" ), TEXT( "/Script/Engine.HLODSubActor" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "Package" ), TEXT( "WorldPartitionRuntimeCellObjectMapping.ActorPackage" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "Path" ), TEXT( "WorldPartitionRuntimeCellObjectMapping.ActorPath" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "ActorInstanceGuid" ), TEXT( "WorldPartitionRuntimeCellObjectMapping.ActorGuid" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "AnimationSequenceBase" ), TEXT( "AnimationBlueprintLibrary.GetAnimationCurveNames.AnimationSequence" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "AnimationSequenceBase" ), TEXT( "AnimationBlueprintLibrary.AddCurve.AnimationSequence" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "AnimationSequenceBase" ), TEXT( "AnimationBlueprintLibrary.RemoveCurve.AnimationSequence" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "AnimationSequenceBase" ), TEXT( "AnimationBlueprintLibrary.RemoveAllCurveData.AnimationSequence" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "AnimationSequenceBase" ), TEXT( "AnimationBlueprintLibrary.AddTransformationCurveKey.AnimationSequence" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "AnimationSequenceBase" ), TEXT( "AnimationBlueprintLibrary.AddTransformationCurveKeys.AnimationSequence" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "AnimationSequenceBase" ), TEXT( "AnimationBlueprintLibrary.AddFloatCurveKey.AnimationSequence" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "AnimationSequenceBase" ), TEXT( "AnimationBlueprintLibrary.AddFloatCurveKeys.AnimationSequence" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "AnimationSequenceBase" ), TEXT( "AnimationBlueprintLibrary.AddVectorCurveKey.AnimationSequence" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "AnimationSequenceBase" ), TEXT( "AnimationBlueprintLibrary.AddVectorCurveKeys.AnimationSequence" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "AnimationSequenceBase" ), TEXT( "AnimationBlueprintLibrary.GetFloatKeys.AnimationSequence" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "AnimationSequenceBase" ), TEXT( "AnimationBlueprintLibrary.GetVectorKeys.AnimationSequence" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "AnimationSequenceBase" ), TEXT( "AnimationBlueprintLibrary.GetTransformationKeys.AnimationSequence" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "AnimationSequenceBase" ), TEXT( "AnimationBlueprintLibrary.DoesCurveExist.AnimationSequence" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Enum, TEXT( "" ), TEXT( "/Script/HeadMountedDisplay.EHMDTrackingOrigin" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "FNetworkPhysicsData" ), TEXT( "FNetworkPhysicsDatas" ) ) );
#endif

#if (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION <= 27) || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION <= 4)
	//5.5
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/Script/WindowsTargetPlatformSettings" ), TEXT( "/Script/WindowsTargetPlatform" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/Script/MacTargetPlatformSettings" ), TEXT( "/Script/MacTargetPlatform" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/Script/CoreUObject" ), TEXT( "/Script/StructUtils" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/Script/Engine" ), TEXT( "/Script/StructUtilsEngine" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/Script/BlueprintGraph" ), TEXT( "/Script/StructUtilsNodes" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/CoreUObject.UserDefinedStruct" ), TEXT( "/Script/Engine.UserDefinedStruct" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/Engine.BlueprintInstancedStructLibrary" ), TEXT( "/Script/StructUtils.StructUtilsFunctionLibrary" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/Engine.BlueprintInstancedStructLibrary" ), TEXT( "/Script/StructUtilsEngine.StructUtilsFunctionLibrary" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/CoreUObject.VerseClass" ), TEXT( "/Script/Solaris.VerseClass" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/CoreUObject.VerseEnum" ), TEXT( "/Script/Solaris.VerseEnum" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/CoreUObject.VerseStruct" ), TEXT( "/Script/Solaris.VerseStruct" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/CoreUObject.InstancedStruct" ), TEXT( "/Script/StructUtils.UniqueScriptStructPtr" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/CoreUObject.InstancedStructContainer" ), TEXT( "/Script/StructUtils.InstancedStructArray" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Enum, TEXT( "/Script/CoreUObject.EUserDefinedStructureStatus" ), TEXT( "/Script/Engine.EUserDefinedStructureStatus" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Enum, TEXT( "/Script/CoreUObject.EVerseFalse" ), TEXT( "/Script/Solaris.EVerseFalse" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Enum, TEXT( "/Script/CoreUObject.EVerseTrue" ), TEXT( "/Script/Solaris.EVerseTrue" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Enum, TEXT( "/Script/CoreUObject.EVersePackageScope" ), TEXT( "/Script/Solaris.EVersePackageScope" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Enum, TEXT( "/Script/CoreUObject.EVersePackageType" ), TEXT( "/Script/Solaris.EVersePackageType" ) ) );
	////Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "bDefaultsToPureFunc" ), TEXT( "K2Node_CallFunction.bIsPureFunc" ) ) );
	////Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Function, TEXT( "FMod64" ), TEXT( "KismetMathLibrary.FMod" ) ) );
	////Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Function, TEXT( "MinAreaRectangle" ), TEXT( "KismetMathLibrary.MinimumAreaRectangle" ) ) );
	////Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "InPoints" ), TEXT( "KismetMathLibrary.MinAreaRectangle.InVerts" ) ) );
	////Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "bOverrideResimulationErrorPositionThreshold" ), TEXT( "NetworkPhysicsSettingsResimulation.bOverrideResimulationErrorThreshold" ) ) );
	////Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "ResimulationErrorPositionThreshold" ), TEXT( "NetworkPhysicsSettingsResimulation.ResimulationErrorThreshold" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/EngineCameras.CompositeCameraShakePattern" ), TEXT( "/Script/GameplayCameras.CompositeCameraShakePattern" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/EngineCameras.DefaultCameraShakeBase" ), TEXT( "/Script/GameplayCameras.DefaultCameraShakeBase" ) ) );
	////Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/EngineCameras.LegacyCameraShake" ), TEXT( "/Script/GameplayCameras.LegacyCameraShake" ) ) );
	////Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/EngineCameras.LegacyCameraShakePattern" ), TEXT( "/Script/GameplayCameras.LegacyCameraShakePattern" ) ) );
	////Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/EngineCameras.LegacyCameraShakeFunctionLibrary" ), TEXT( "/Script/GameplayCameras.LegacyCameraShakeFunctionLibrary" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/EngineCameras.PerlinNoiseCameraShakePattern" ), TEXT( "/Script/GameplayCameras.PerlinNoiseCameraShakePattern" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/EngineCameras.SimpleCameraShakePattern" ), TEXT( "/Script/GameplayCameras.SimpleCameraShakePattern" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/EngineCameras.WaveOscillatorCameraShakePattern" ), TEXT( "/Script/GameplayCameras.WaveOscillatorCameraShakePattern" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/EngineCameras.CameraAnimationCameraModifier" ), TEXT( "/Script/GameplayCameras.CameraAnimationCameraModifier" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/EngineCameras.EngineCameraAnimationFunctionLibrary" ), TEXT( "/Script/GameplayCameras.GameplayCamerasFunctionLibrary" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/EngineCameras.EngineCamerasSubsystem" ), TEXT( "/Script/GameplayCameras.GameplayCamerasSubsystem" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/EngineCameras.PerlinNoiseShaker" ), TEXT( "/Script/GameplayCameras.PerlinNoiseShaker" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/EngineCameras.WaveOscillator" ), TEXT( "/Script/GameplayCameras.WaveOscillator" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/EngineCameras.CameraAnimationParams" ), TEXT( "/Script/GameplayCameras.CameraAnimationParams" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/EngineCameras.CameraAnimationHandle" ), TEXT( "/Script/GameplayCameras.CameraAnimationHandle" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/EngineCameras.ActiveCameraAnimationInfo" ), TEXT( "/Script/GameplayCameras.ActiveCameraAnimationInfo" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Enum, TEXT( "/Script/EngineCameras.EInitialWaveOscillatorOffsetType" ), TEXT( "/Script/GameplayCameras.EInitialWaveOscillatorOffsetType" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Enum, TEXT( "/Script/EngineCameras.ECameraAnimationPlaySpace" ), TEXT( "/Script/GameplayCameras.ECameraAnimationPlaySpace" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Enum, TEXT( "/Script/EngineCameras.ECameraAnimationEasingType" ), TEXT( "/Script/GameplayCameras.ECameraAnimationEasingType" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Function, TEXT( "SetAnimInstanceClass" ), TEXT( "SkeletalMeshComponent.SetAnimClass" ) ) );
	////Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Function, TEXT( "SetAnimInstanceClass" ), TEXT( "SkeletalMeshComponent.K2_SetAnimInstanceClass" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Function, TEXT( "BP_GetOwnedGameplayTags" ), TEXT( "/Script/GameplayTags.GameplayTagAssetInterface.GetOwnedGameplayTags" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "ReturnValue" ), TEXT( "GameplayTagAssetInterface.BP_GetOwnedGameplayTags.TagContainer" ) ) );

	//AnimBlueprintExtensions
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/AnimGraph.AnimBlueprintExtension_LinkedInputPose" ), TEXT( "/Script/Engine.BlueprintExtension" ) ) );

	//BaseInterchangeAssets.ini	
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/Functions/MF_Iridescence.MF_Iridescence" ), TEXT( "/Interchange/Functions/MF_Iridescence.MF_Iridescence" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/Functions/MF_OrenNayerView.MF_OrenNayerView" ), TEXT( "/Interchange/Functions/MF_OrenNayerView.MF_OrenNayerView" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/Functions/MF_PhongToMetalRoughness.MF_PhongToMetalRoughness" ), TEXT( "/Interchange/Functions/MF_PhongToMetalRoughness.MF_PhongToMetalRoughness" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/Functions/MF_SchlickApprox.MF_SchlickApprox" ), TEXT( "/Interchange/Functions/MF_SchlickApprox.MF_SchlickApprox" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/Functions/MX_AbsorptionVDF.MX_AbsorptionVDF" ), TEXT( "/Interchange/Functions/MX_AbsorptionVDF.MX_AbsorptionVDF" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/Functions/MX_AnisotropicVDF.MX_AnisotropicVDF" ), TEXT( "/Interchange/Functions/MX_AnisotropicVDF.MX_AnisotropicVDF" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/Functions/MX_Artistic_IOR.MX_Artistic_IOR" ), TEXT( "/Interchange/Functions/MX_Artistic_IOR.MX_Artistic_IOR" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/Functions/MX_BurleyDiffuseBSDF.MX_BurleyDiffuseBSDF" ), TEXT( "/Interchange/Functions/MX_BurleyDiffuseBSDF.MX_BurleyDiffuseBSDF" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/Functions/MX_ConductorBSDF.MX_ConductorBSDF" ), TEXT( "/Interchange/Functions/MX_ConductorBSDF.MX_ConductorBSDF" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/Functions/MX_ConicalEDF.MX_ConicalEDF" ), TEXT( "/Interchange/Functions/MX_ConicalEDF.MX_ConicalEDF" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/Functions/MX_DielectricBSDF.MX_DielectricBSDF" ), TEXT( "/Interchange/Functions/MX_DielectricBSDF.MX_DielectricBSDF" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/Functions/MX_GeneralizedSchlickBSDF.MX_GeneralizedSchlickBSDF" ), TEXT( "/Interchange/Functions/MX_GeneralizedSchlickBSDF.MX_GeneralizedSchlickBSDF" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/Functions/MX_MeasuredEDF.MX_MeasuredEDF" ), TEXT( "/Interchange/Functions/MX_MeasuredEDF.MX_MeasuredEDF" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/Functions/MX_OpenPBR_Opaque.MX_OpenPBR_Opaque" ), TEXT( "/Interchange/Functions/MX_OpenPBR_Opaque.MX_OpenPBR_Opaque" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/Functions/MX_OpenPBR_Translucent.MX_OpenPBR_Translucent" ), TEXT( "/Interchange/Functions/MX_OpenPBR_Translucent.MX_OpenPBR_Translucent" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/Functions/MX_OrenNayarBSDF.MX_OrenNayarBSDF" ), TEXT( "/Interchange/Functions/MX_OrenNayarBSDF.MX_OrenNayarBSDF" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/Functions/MX_Place2D.MX_Place2D" ), TEXT( "/Interchange/Functions/MX_Place2D.MX_Place2D" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/Functions/MX_Roughness_Anisotropy.MX_Roughness_Anisotropy" ), TEXT( "/Interchange/Functions/MX_Roughness_Anisotropy.MX_Roughness_Anisotropy" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/Functions/MX_Roughness_Dual.MX_Roughness_Dual" ), TEXT( "/Interchange/Functions/MX_Roughness_Dual.MX_Roughness_Dual" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/Functions/MX_SheenBSDF.MX_SheenBSDF" ), TEXT( "/Interchange/Functions/MX_SheenBSDF.MX_SheenBSDF" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/Functions/MX_StandardSurface.MX_StandardSurface" ), TEXT( "/Interchange/Functions/MX_StandardSurface.MX_StandardSurface" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/Functions/MX_SubsurfaceBSDF.MX_SubsurfaceBSDF" ), TEXT( "/Interchange/Functions/MX_SubsurfaceBSDF.MX_SubsurfaceBSDF" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/Functions/MX_Surface.MX_Surface" ), TEXT( "/Interchange/Functions/MX_Surface.MX_Surface" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/Functions/MX_SurfaceUnlit.MX_SurfaceUnlit" ), TEXT( "/Interchange/Functions/MX_SurfaceUnlit.MX_SurfaceUnlit" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/Functions/MX_ThinFilmBSDF.MX_ThinFilmBSDF" ), TEXT( "/Interchange/Functions/MX_ThinFilmBSDF.MX_ThinFilmBSDF" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/Functions/MX_TranslucentBSDF.MX_TranslucentBSDF" ), TEXT( "/Interchange/Functions/MX_TranslucentBSDF.MX_TranslucentBSDF" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/Functions/MX_Transmission.MX_Transmission" ), TEXT( "/Interchange/Functions/MX_Transmission.MX_Transmission" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/Functions/MX_TransmissionSurface.MX_TransmissionSurface" ), TEXT( "/Interchange/Functions/MX_TransmissionSurface.MX_TransmissionSurface" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/Functions/MX_UniformEDF.MX_UniformEDF" ), TEXT( "/Interchange/Functions/MX_UniformEDF.MX_UniformEDF" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/Functions/MX_UsdPreviewSurface.MX_UsdPreviewSurface" ), TEXT( "/Interchange/Functions/MX_UsdPreviewSurface.MX_UsdPreviewSurface" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/gltf/M_ClearCoat.M_ClearCoat" ), TEXT( "/Interchange/gltf/M_ClearCoat.M_ClearCoat" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/gltf/M_Default.M_Default" ), TEXT( "/Interchange/gltf/M_Default.M_Default" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/gltf/M_Sheen.M_Sheen" ), TEXT( "/Interchange/gltf/M_Sheen.M_Sheen" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/gltf/M_SpecularGlossiness.M_SpecularGlossiness" ), TEXT( "/Interchange/gltf/M_SpecularGlossiness.M_SpecularGlossiness" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/gltf/M_Transmission.M_Transmission" ), TEXT( "/Interchange/gltf/M_Transmission.M_Transmission" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/gltf/M_Unlit.M_Unlit" ), TEXT( "/Interchange/gltf/M_Unlit.M_Unlit" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/gltf/PerceivedBrightness.PerceivedBrightness" ), TEXT( "/Interchange/gltf/PerceivedBrightness.PerceivedBrightness" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/gltf/SpecGlossToMetalRoughness.SpecGlossToMetalRoughness" ), TEXT( "/Interchange/gltf/SpecGlossToMetalRoughness.SpecGlossToMetalRoughness" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/gltf/MaterialBodies/MF_ClearCoat_Body.MF_ClearCoat_Body" ), TEXT( "/Interchange/gltf/MaterialBodies/MF_ClearCoat_Body.MF_ClearCoat_Body" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/gltf/MaterialBodies/MF_Default_Body.MF_Default_Body" ), TEXT( "/Interchange/gltf/MaterialBodies/MF_Default_Body.MF_Default_Body" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/gltf/MaterialBodies/MF_Sheen_Body.MF_Sheen_Body" ), TEXT( "/Interchange/gltf/MaterialBodies/MF_Sheen_Body.MF_Sheen_Body" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/gltf/MaterialBodies/MF_SpecularGlossiness_Body.MF_SpecularGlossiness_Body" ), TEXT( "/Interchange/gltf/MaterialBodies/MF_SpecularGlossiness_Body.MF_SpecularGlossiness_Body" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/gltf/MaterialBodies/MF_Transmission_Body.MF_Transmission_Body" ), TEXT( "/Interchange/gltf/MaterialBodies/MF_Transmission_Body.MF_Transmission_Body" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/gltf/MaterialBodies/MF_Unlit_Body.MF_Unlit_Body" ), TEXT( "/Interchange/gltf/MaterialBodies/MF_Unlit_Body.MF_Unlit_Body" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/gltf/MaterialFunctions/MF_BaseColor.MF_BaseColor" ), TEXT( "/Interchange/gltf/MaterialFunctions/MF_BaseColor.MF_BaseColor" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/gltf/MaterialFunctions/MF_Clearcoat.MF_Clearcoat" ), TEXT( "/Interchange/gltf/MaterialFunctions/MF_Clearcoat.MF_Clearcoat" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/gltf/MaterialFunctions/MF_DiffuseSpecGloss.MF_DiffuseSpecGloss" ), TEXT( "/Interchange/gltf/MaterialFunctions/MF_DiffuseSpecGloss.MF_DiffuseSpecGloss" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/gltf/MaterialFunctions/MF_Emissive.MF_Emissive" ), TEXT( "/Interchange/gltf/MaterialFunctions/MF_Emissive.MF_Emissive" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/gltf/MaterialFunctions/MF_Fresnel_DS.MF_Fresnel_DS" ), TEXT( "/Interchange/gltf/MaterialFunctions/MF_Fresnel_DS.MF_Fresnel_DS" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/gltf/MaterialFunctions/MF_IOR.MF_IOR" ), TEXT( "/Interchange/gltf/MaterialFunctions/MF_IOR.MF_IOR" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/gltf/MaterialFunctions/MF_Iridescence.MF_Iridescence" ), TEXT( "/Interchange/gltf/MaterialFunctions/MF_Iridescence.MF_Iridescence" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/gltf/MaterialFunctions/MF_MaxComponentValue.MF_MaxComponentValue" ), TEXT( "/Interchange/gltf/MaterialFunctions/MF_MaxComponentValue.MF_MaxComponentValue" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/gltf/MaterialFunctions/MF_MetallicRoughness.MF_MetallicRoughness" ), TEXT( "/Interchange/gltf/MaterialFunctions/MF_MetallicRoughness.MF_MetallicRoughness" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/gltf/MaterialFunctions/MF_Normals.MF_Normals" ), TEXT( "/Interchange/gltf/MaterialFunctions/MF_Normals.MF_Normals" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/gltf/MaterialFunctions/MF_Occlusion.MF_Occlusion" ), TEXT( "/Interchange/gltf/MaterialFunctions/MF_Occlusion.MF_Occlusion" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/gltf/MaterialFunctions/MF_PerceivedBrightness.MF_PerceivedBrightness" ), TEXT( "/Interchange/gltf/MaterialFunctions/MF_PerceivedBrightness.MF_PerceivedBrightness" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/gltf/MaterialFunctions/MF_RotateNormals_TS.MF_RotateNormals_TS" ), TEXT( "/Interchange/gltf/MaterialFunctions/MF_RotateNormals_TS.MF_RotateNormals_TS" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/gltf/MaterialFunctions/MF_RotateV2.MF_RotateV2" ), TEXT( "/Interchange/gltf/MaterialFunctions/MF_RotateV2.MF_RotateV2" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/gltf/MaterialFunctions/MF_ScaleNormal.MF_ScaleNormal" ), TEXT( "/Interchange/gltf/MaterialFunctions/MF_ScaleNormal.MF_ScaleNormal" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/gltf/MaterialFunctions/MF_Sheen.MF_Sheen" ), TEXT( "/Interchange/gltf/MaterialFunctions/MF_Sheen.MF_Sheen" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/gltf/MaterialFunctions/MF_SpecGlossToMetalRoughness.MF_SpecGlossToMetalRoughness" ), TEXT( "/Interchange/gltf/MaterialFunctions/MF_SpecGlossToMetalRoughness.MF_SpecGlossToMetalRoughness" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/gltf/MaterialFunctions/MF_Specular.MF_Specular" ), TEXT( "/Interchange/gltf/MaterialFunctions/MF_Specular.MF_Specular" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/gltf/MaterialFunctions/MF_SpecularAnisotropy.MF_SpecularAnisotropy" ), TEXT( "/Interchange/gltf/MaterialFunctions/MF_SpecularAnisotropy.MF_SpecularAnisotropy" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/gltf/MaterialFunctions/MF_Temporal_Blur.MF_Temporal_Blur" ), TEXT( "/Interchange/gltf/MaterialFunctions/MF_Temporal_Blur.MF_Temporal_Blur" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/gltf/MaterialFunctions/MF_TransformUVs.MF_TransformUVs" ), TEXT( "/Interchange/gltf/MaterialFunctions/MF_TransformUVs.MF_TransformUVs" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/gltf/MaterialFunctions/MF_Transmission.MF_Transmission" ), TEXT( "/Interchange/gltf/MaterialFunctions/MF_Transmission.MF_Transmission" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/gltf/MaterialFunctions/MF_TransmissionOpacity.MF_TransmissionOpacity" ), TEXT( "/Interchange/gltf/MaterialFunctions/MF_TransmissionOpacity.MF_TransmissionOpacity" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/gltf/MaterialInstances/MI_ClearCoat_Blend.MI_ClearCoat_Blend" ), TEXT( "/Interchange/gltf/MaterialInstances/MI_ClearCoat_Blend.MI_ClearCoat_Blend" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/gltf/MaterialInstances/MI_ClearCoat_Blend_DS.MI_ClearCoat_Blend_DS" ), TEXT( "/Interchange/gltf/MaterialInstances/MI_ClearCoat_Blend_DS.MI_ClearCoat_Blend_DS" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/gltf/MaterialInstances/MI_ClearCoat_Mask.MI_ClearCoat_Mask" ), TEXT( "/Interchange/gltf/MaterialInstances/MI_ClearCoat_Mask.MI_ClearCoat_Mask" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/gltf/MaterialInstances/MI_ClearCoat_Mask_DS.MI_ClearCoat_Mask_DS" ), TEXT( "/Interchange/gltf/MaterialInstances/MI_ClearCoat_Mask_DS.MI_ClearCoat_Mask_DS" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/gltf/MaterialInstances/MI_ClearCoat_Opaque.MI_ClearCoat_Opaque" ), TEXT( "/Interchange/gltf/MaterialInstances/MI_ClearCoat_Opaque.MI_ClearCoat_Opaque" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/gltf/MaterialInstances/MI_ClearCoat_Opaque_DS.MI_ClearCoat_Opaque_DS" ), TEXT( "/Interchange/gltf/MaterialInstances/MI_ClearCoat_Opaque_DS.MI_ClearCoat_Opaque_DS" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/gltf/MaterialInstances/MI_Default_Blend.MI_Default_Blend" ), TEXT( "/Interchange/gltf/MaterialInstances/MI_Default_Blend.MI_Default_Blend" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/gltf/MaterialInstances/MI_Default_Blend_DS.MI_Default_Blend_DS" ), TEXT( "/Interchange/gltf/MaterialInstances/MI_Default_Blend_DS.MI_Default_Blend_DS" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/gltf/MaterialInstances/MI_Default_Mask.MI_Default_Mask" ), TEXT( "/Interchange/gltf/MaterialInstances/MI_Default_Mask.MI_Default_Mask" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/gltf/MaterialInstances/MI_Default_Mask_DS.MI_Default_Mask_DS" ), TEXT( "/Interchange/gltf/MaterialInstances/MI_Default_Mask_DS.MI_Default_Mask_DS" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/gltf/MaterialInstances/MI_Default_Opaque.MI_Default_Opaque" ), TEXT( "/Interchange/gltf/MaterialInstances/MI_Default_Opaque.MI_Default_Opaque" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/gltf/MaterialInstances/MI_Default_Opaque_DS.MI_Default_Opaque_DS" ), TEXT( "/Interchange/gltf/MaterialInstances/MI_Default_Opaque_DS.MI_Default_Opaque_DS" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/gltf/MaterialInstances/MI_Sheen_Blend.MI_Sheen_Blend" ), TEXT( "/Interchange/gltf/MaterialInstances/MI_Sheen_Blend.MI_Sheen_Blend" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/gltf/MaterialInstances/MI_Sheen_Blend_DS.MI_Sheen_Blend_DS" ), TEXT( "/Interchange/gltf/MaterialInstances/MI_Sheen_Blend_DS.MI_Sheen_Blend_DS" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/gltf/MaterialInstances/MI_Sheen_Mask.MI_Sheen_Mask" ), TEXT( "/Interchange/gltf/MaterialInstances/MI_Sheen_Mask.MI_Sheen_Mask" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/gltf/MaterialInstances/MI_Sheen_Mask_DS.MI_Sheen_Mask_DS" ), TEXT( "/Interchange/gltf/MaterialInstances/MI_Sheen_Mask_DS.MI_Sheen_Mask_DS" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/gltf/MaterialInstances/MI_Sheen_Opaque.MI_Sheen_Opaque" ), TEXT( "/Interchange/gltf/MaterialInstances/MI_Sheen_Opaque.MI_Sheen_Opaque" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/gltf/MaterialInstances/MI_Sheen_Opaque_DS.MI_Sheen_Opaque_DS" ), TEXT( "/Interchange/gltf/MaterialInstances/MI_Sheen_Opaque_DS.MI_Sheen_Opaque_DS" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/gltf/MaterialInstances/MI_SpecularGlossiness_Blend.MI_SpecularGlossiness_Blend" ), TEXT( "/Interchange/gltf/MaterialInstances/MI_SpecularGlossiness_Blend.MI_SpecularGlossiness_Blend" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/gltf/MaterialInstances/MI_SpecularGlossiness_Blend_DS.MI_SpecularGlossiness_Blend_DS" ), TEXT( "/Interchange/gltf/MaterialInstances/MI_SpecularGlossiness_Blend_DS.MI_SpecularGlossiness_Blend_DS" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/gltf/MaterialInstances/MI_SpecularGlossiness_Mask.MI_SpecularGlossiness_Mask" ), TEXT( "/Interchange/gltf/MaterialInstances/MI_SpecularGlossiness_Mask.MI_SpecularGlossiness_Mask" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/gltf/MaterialInstances/MI_SpecularGlossiness_Mask_DS.MI_SpecularGlossiness_Mask_DS" ), TEXT( "/Interchange/gltf/MaterialInstances/MI_SpecularGlossiness_Mask_DS.MI_SpecularGlossiness_Mask_DS" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/gltf/MaterialInstances/MI_SpecularGlossiness_Opaque.MI_SpecularGlossiness_Opaque" ), TEXT( "/Interchange/gltf/MaterialInstances/MI_SpecularGlossiness_Opaque.MI_SpecularGlossiness_Opaque" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/gltf/MaterialInstances/MI_SpecularGlossiness_Opaque_DS.MI_SpecularGlossiness_Opaque_DS" ), TEXT( "/Interchange/gltf/MaterialInstances/MI_SpecularGlossiness_Opaque_DS.MI_SpecularGlossiness_Opaque_DS" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/gltf/MaterialInstances/MI_Transmission.MI_Transmission" ), TEXT( "/Interchange/gltf/MaterialInstances/MI_Transmission.MI_Transmission" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/gltf/MaterialInstances/MI_Transmission_DS.MI_Transmission_DS" ), TEXT( "/Interchange/gltf/MaterialInstances/MI_Transmission_DS.MI_Transmission_DS" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/gltf/MaterialInstances/MI_Unlit_Blend.MI_Unlit_Blend" ), TEXT( "/Interchange/gltf/MaterialInstances/MI_Unlit_Blend.MI_Unlit_Blend" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/gltf/MaterialInstances/MI_Unlit_Blend_DS.MI_Unlit_Blend_DS" ), TEXT( "/Interchange/gltf/MaterialInstances/MI_Unlit_Blend_DS.MI_Unlit_Blend_DS" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/gltf/MaterialInstances/MI_Unlit_Mask.MI_Unlit_Mask" ), TEXT( "/Interchange/gltf/MaterialInstances/MI_Unlit_Mask.MI_Unlit_Mask" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/gltf/MaterialInstances/MI_Unlit_Mask_DS.MI_Unlit_Mask_DS" ), TEXT( "/Interchange/gltf/MaterialInstances/MI_Unlit_Mask_DS.MI_Unlit_Mask_DS" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/gltf/MaterialInstances/MI_Unlit_Opaque.MI_Unlit_Opaque" ), TEXT( "/Interchange/gltf/MaterialInstances/MI_Unlit_Opaque.MI_Unlit_Opaque" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/gltf/MaterialInstances/MI_Unlit_Opaque_DS.MI_Unlit_Opaque_DS" ), TEXT( "/Interchange/gltf/MaterialInstances/MI_Unlit_Opaque_DS.MI_Unlit_Opaque_DS" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/gltf/Textures/T_Anisotropy_Direction_Linear.T_Anisotropy_Direction_Linear" ), TEXT( "/Interchange/gltf/Textures/T_Anisotropy_Direction_Linear.T_Anisotropy_Direction_Linear" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/gltf/Textures/T_Black_srgb.T_Black_srgb" ), TEXT( "/Interchange/gltf/Textures/T_Black_srgb.T_Black_srgb" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/gltf/Textures/T_Generic_N.T_Generic_N" ), TEXT( "/Interchange/gltf/Textures/T_Generic_N.T_Generic_N" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/gltf/Textures/T_Gray_Linear.T_Gray_Linear" ), TEXT( "/Interchange/gltf/Textures/T_Gray_Linear.T_Gray_Linear" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/gltf/Textures/T_White_Linear.T_White_Linear" ), TEXT( "/Interchange/gltf/Textures/T_White_Linear.T_White_Linear" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/gltf/Textures/T_White_srgb.T_White_srgb" ), TEXT( "/Interchange/gltf/Textures/T_White_srgb.T_White_srgb" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/Materials/ClearCoatMaterial_MR.ClearCoatMaterial_MR" ), TEXT( "/Interchange/Materials/ClearCoatMaterial_MR.ClearCoatMaterial_MR" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/Materials/ClearCoatMaterial_SG.ClearCoatMaterial_SG" ), TEXT( "/Interchange/Materials/ClearCoatMaterial_SG.ClearCoatMaterial_SG" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/Materials/CommonMaterial.CommonMaterial" ), TEXT( "/Interchange/Materials/CommonMaterial.CommonMaterial" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/Materials/DecalMaterial.DecalMaterial" ), TEXT( "/Interchange/Materials/DecalMaterial.DecalMaterial" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/Materials/LambertSurfaceMaterial.LambertSurfaceMaterial" ), TEXT( "/Interchange/Materials/LambertSurfaceMaterial.LambertSurfaceMaterial" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/Materials/MapOrColorFunction.MapOrColorFunction" ), TEXT( "/Interchange/Materials/MapOrColorFunction.MapOrColorFunction" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/Materials/MapOrLinearColorFunction.MapOrLinearColorFunction" ), TEXT( "/Interchange/Materials/MapOrLinearColorFunction.MapOrLinearColorFunction" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/Materials/MapOrScalarFunction.MapOrScalarFunction" ), TEXT( "/Interchange/Materials/MapOrScalarFunction.MapOrScalarFunction" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/Materials/OrenNayerMaterial.OrenNayerMaterial" ), TEXT( "/Interchange/Materials/OrenNayerMaterial.OrenNayerMaterial" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/Materials/PBRSurfaceFunction_MR.PBRSurfaceFunction_MR" ), TEXT( "/Interchange/Materials/PBRSurfaceFunction_MR.PBRSurfaceFunction_MR" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/Materials/PBRSurfaceFunction_SG.PBRSurfaceFunction_SG" ), TEXT( "/Interchange/Materials/PBRSurfaceFunction_SG.PBRSurfaceFunction_SG" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/Materials/PBRSurfaceMaterial_MR.PBRSurfaceMaterial_MR" ), TEXT( "/Interchange/Materials/PBRSurfaceMaterial_MRPBRSurfaceMaterial_MR" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/Materials/PBRSurfaceMaterial_SG.PBRSurfaceMaterial_SG" ), TEXT( "/Interchange/Materials/PBRSurfaceMaterial_SG.PBRSurfaceMaterial_SG" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/Materials/PhongSurfaceMaterial.PhongSurfaceMaterial" ), TEXT( "/Interchange/Materials/PhongSurfaceMaterial.PhongSurfaceMaterial" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/Materials/SheenMaterial_MR.SheenMaterial_MR" ), TEXT( "/Interchange/Materials/SheenMaterial_MR.SheenMaterial_MR" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/Materials/SheenMaterial_SG.SheenMaterial_SG" ), TEXT( "/Interchange/Materials/SheenMaterial_SG.SheenMaterial_SG" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/Materials/SubsurfaceMaterial_MR.SubsurfaceMaterial_MR" ), TEXT( "/Interchange/Materials/SubsurfaceMaterial_MR.SubsurfaceMaterial_MR" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/Materials/SubsurfaceMaterial_SG.SubsurfaceMaterial_SG" ), TEXT( "/Interchange/Materials/SubsurfaceMaterial_SG.SubsurfaceMaterial_SG" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/Materials/ThinTranslucentMaterial_MR.ThinTranslucentMaterial_MR" ), TEXT( "/Interchange/Materials/ThinTranslucentMaterial_MR.ThinTranslucentMaterial_MR" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/Materials/ThinTranslucentMaterial_SG.ThinTranslucentMaterial_SG" ), TEXT( "/Interchange/Materials/ThinTranslucentMaterial_SG.ThinTranslucentMaterial_SG" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/Materials/UnlitMaterial.UnlitMaterial" ), TEXT( "/Interchange/Materials/UnlitMaterial.UnlitMaterial" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/Substrate/MX_AbsorptionVDF.MX_AbsorptionVDF" ), TEXT( "/Interchange/Substrate/MX_AbsorptionVDF.MX_AbsorptionVDF" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/Substrate/MX_AnisotropicVDF.MX_AnisotropicVDF" ), TEXT( "/Interchange/Substrate/MX_AnisotropicVDF.MX_AnisotropicVDF" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/Substrate/MX_BurleyDiffuseBSDF.MX_BurleyDiffuseBSDF" ), TEXT( "/Interchange/Substrate/MX_BurleyDiffuseBSDF.MX_BurleyDiffuseBSDF" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/Substrate/MX_ConductorBSDF.MX_ConductorBSDF" ), TEXT( "/Interchange/Substrate/MX_ConductorBSDF.MX_ConductorBSDF" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/Substrate/MX_ConicalEDF.MX_ConicalEDF" ), TEXT( "/Interchange/Substrate/MX_ConicalEDF.MX_ConicalEDF" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/Substrate/MX_DielectricBSDF.MX_DielectricBSDF" ), TEXT( "/Interchange/Substrate/MX_DielectricBSDF.MX_DielectricBSDF" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/Substrate/MX_GeneralizedSchlickBSDF.MX_GeneralizedSchlickBSDF" ), TEXT( "/Interchange/Substrate/MX_GeneralizedSchlickBSDF.MX_GeneralizedSchlickBSDF" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/Substrate/MX_MeasuredEDF.MX_MeasuredEDF" ), TEXT( "/Interchange/Substrate/MX_MeasuredEDF.MX_MeasuredEDF" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/Substrate/MX_OrenNayarBSDF.MX_OrenNayarBSDF" ), TEXT( "/Interchange/Substrate/MX_OrenNayarBSDF.MX_OrenNayarBSDF" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/Substrate/MX_SheenBSDF.MX_SheenBSDF" ), TEXT( "/Interchange/Substrate/MX_SheenBSDF.MX_SheenBSDF" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/Substrate/MX_SubsurfaceBSDF.MX_SubsurfaceBSDF" ), TEXT( "/Interchange/Substrate/MX_SubsurfaceBSDF.MX_SubsurfaceBSDF" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/Substrate/MX_Surface.MX_Surface" ), TEXT( "/Interchange/Substrate/MX_Surface.MX_Surface" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/Substrate/MX_SurfaceUnlit.MX_SurfaceUnlit" ), TEXT( "/Interchange/Substrate/MX_SurfaceUnlit.MX_SurfaceUnlit" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/Substrate/MX_ThinFilmBSDF.MX_ThinFilmBSDF" ), TEXT( "/Interchange/Substrate/MX_ThinFilmBSDF.MX_ThinFilmBSDF" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/Substrate/MX_TranslucentBSDF.MX_TranslucentBSDF" ), TEXT( "/Interchange/Substrate/MX_TranslucentBSDF.MX_TranslucentBSDF" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/Substrate/MX_UniformEDF.MX_UniformEDF" ), TEXT( "/Interchange/Substrate/MX_UniformEDF.MX_UniformEDF" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/Substrate/MX_UsdPreviewSurface.MX_UsdPreviewSurface" ), TEXT( "/Interchange/Substrate/MX_UsdPreviewSurface.MX_UsdPreviewSurface" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/Utilities/New_LUT.New_LUT" ), TEXT( "/Interchange/Utilities/New_LUT.New_LUT" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/InterchangeAssets/Utilities/T_Bayer64_Grayscale_64px.T_Bayer64_Grayscale_64px" ), TEXT( "/Interchange/Utilities/T_Bayer64_Grayscale_64px.T_Bayer64_Grayscale_64px" ) ) );

	//Niagara
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Enum, TEXT( "/Script/Niagara.ENiagaraExecutionStateManagement" ), TEXT( "/Niagara/Enums/ENiagaraExecutionStateManagement.ENiagaraExecutionStateManagement" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/Script/Niagara" ), TEXT( "/Niagara/Enums/ENiagaraExecutionStateManagement" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/Niagara/DefaultAssets/Templates/Emitters/Minimal" ), TEXT( "/Niagara/DefaultAssets/Templates/Emitters/Empty" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/Niagara/DefaultAssets/Templates/Emitters/Minimal.Minimal" ), TEXT( "/Niagara/DefaultAssets/Templates/Emitters/Empty.Empty" ) ) );
#endif
#if (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION <= 27) || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION <= 5)
	//5.6
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Package, TEXT( "/Script/DataHierarchyEditor" ), TEXT( "/Script/HierarchyEditor" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/DataHierarchyEditor.HierarchyElementIdentity" ), TEXT( "/Script/HierarchyEditor.HierarchyElementIdentity" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/DataHierarchyEditor.HierarchyElement" ), TEXT( "/Script/HierarchyEditor.HierarchyElement" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/DataHierarchyEditor.HierarchyRoot" ), TEXT( "/Script/HierarchyEditor.HierarchyRoot" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/DataHierarchyEditor.HierarchyItem" ), TEXT( "/Script/HierarchyEditor.HierarchyItem" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/DataHierarchyEditor.HierarchySection" ), TEXT( "/Script/HierarchyEditor.HierarchySection" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/DataHierarchyEditor.HierarchyCategory" ), TEXT( "/Script/HierarchyEditor.HierarchyCategory" ) ) );
	//Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Enum, TEXT( "" ), TEXT( "EMaterialExpressionConvertType" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Enum, TEXT( "/Script/Engine.ECollectionScriptingShareType" ), TEXT( "/Script/AssetTags.ECollectionScriptingShareType" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/MassEntity.MassProcessingContext_DEPRECATED" ), TEXT( "/Script/MassEntity.MassProcessingContext" ) ) );

	//EnhancedInput
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "EnhancedInput.PlayerKeyMapping.AssociatedInputActionSoft" ), TEXT( "EnhancedInput.PlayerKeyMapping.AssociatedInputAction" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Function, TEXT( "/Script/EnhancedInput.EnhancedInputUserSettings.GetActiveKeyProfile" ), TEXT( "/Script/EnhancedInput.EnhancedInputUserSettings.GetCurrentKeyProfile" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Function, TEXT( "/Script/EnhancedInput.EnhancedInputUserSettings.GetKeyProfileWithId" ), TEXT( "/Script/EnhancedInput.EnhancedInputUserSettings.GetKeyProfileWithIdentifier" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Function, TEXT( "/Script/EnhancedInput.EnhancedInputUserSettings.SetActiveKeyProfile" ), TEXT( "/Script/EnhancedInput.EnhancedInputUserSettings.SetKeyProfile" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Function, TEXT( "/Script/EnhancedInput.EnhancedInputUserSettings.GetActiveKeyProfileId" ), TEXT( "/Script/EnhancedInput.EnhancedInputUserSettings.GetCurrentKeyProfileIdentifier" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Function, TEXT( "/Script/EnhancedInput.EnhancedPlayerMappableKeyProfile.GetProfileIdString" ), TEXT( "/Script/EnhancedInput.EnhancedPlayerMappableKeyProfile.GetProfileIdentifer" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "EnhancedInput.MapPlayerKeyArgs.ProfileIdString" ), TEXT( "EnhancedInput.MapPlayerKeyArgs.ProfileId" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "EnhancedInput.PlayerMappableKeyProfileCreationArgs.ProfileStringIdentifier" ), TEXT( "EnhancedInput.PlayerMappableKeyProfileCreationArgs.ProfileIdentifier" ) ) );

	//Niagara
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/Niagara.NDIRenderTargetSimCacheData" ), TEXT( "NDIRenderTargetVolumeSimCacheData" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/Niagara.NDIRenderTargetSimCacheFrame" ), TEXT( "NDIRenderTargetVolumeSimCacheFrame" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/DataHierarchyEditor.HierarchyElementIdentity" ), TEXT( "/Script/NiagaraEditor.NiagaraHierarchyIdentity" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/DataHierarchyEditor.HierarchyElement" ), TEXT( "/Script/NiagaraEditor.NiagaraHierarchyItemBase" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/DataHierarchyEditor.HierarchyRoot" ), TEXT( "/Script/NiagaraEditor.NiagaraHierarchyRoot" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/DataHierarchyEditor.HierarchyItem" ), TEXT( "/Script/NiagaraEditor.NiagaraHierarchyItem" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/DataHierarchyEditor.HierarchySection" ), TEXT( "/Script/NiagaraEditor.NiagaraHierarchySection" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/DataHierarchyEditor.HierarchyCategory" ), TEXT( "/Script/NiagaraEditor.NiagaraHierarchyCategory" ) ) );
#endif
#if (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION <= 27) || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION <= 6)
	//5.7
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "ShapePreservation" ), TEXT( "MeshNaniteSettings.bPreserveArea" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "SectionBase" ), TEXT( "LandscapeProxy.LandscapeSectionOffset" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Struct, TEXT( "/Script/MovieSceneTracks.MovieSceneTextChannel" ), TEXT( "/Script/MovieSceneTextTrack.MovieSceneTextChannel" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/MovieSceneTracks.MovieSceneTextPropertySystem" ), TEXT( "/Script/MovieSceneTextTrack.MovieSceneTextPropertySystem" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/MovieSceneTracks.MovieSceneTextSection" ), TEXT( "/Script/MovieSceneTextTrack.MovieSceneTextSection" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/MovieSceneTracks.MovieSceneTextTrack" ), TEXT( "/Script/MovieSceneTextTrack.MovieSceneTextTrack" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/MovieSceneTracks.TextChannelEvaluatorSystem" ), TEXT( "/Script/MovieSceneTextTrack.TextChannelEvaluatorSystem" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/MovieSceneTools.MovieSceneTextKeyStruct" ), TEXT( "/Script/MovieSceneTextTrackEditor.MovieSceneTextKeyStruct" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "/Script/MovieScene.MovieSceneSectionTimingParametersSeconds.bClampToInnerRange" ), TEXT( "/Script/MovieScene.MovieSceneSectionTimingParametersSeconds.bClamp" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "/Script/MovieScene.FMovieSceneSectionTimingParametersFrames.bClampToInnerRange" ), TEXT( "/Script/MovieScene.FMovieSceneSectionTimingParametersFrames.bClamp" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Class, TEXT( "/Script/Engine.WorldPartitionDestructibleHLODComponent" ), TEXT( "/Script/Engine.WorldPartitionDestructibleHLODMeshComponent" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Function, TEXT( "BP_GetOwnedGameplayTags" ), TEXT( "/Script/GameplayTags.GameplayTagAssetInterface.GetOwnedGameplayTags" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "ReturnValue" ), TEXT( "GameplayTagAssetInterface.BP_GetOwnedGameplayTags.TagContainer" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "/Script/UnrealEd.LevelEditorViewportSettings.TransformWidgetSizeAdjustment_DEPRECATED" ), TEXT( "/Script/UnrealEd.LevelEditorViewportSettings.TransformWidgetSizeAdjustment" ) ) );

	//EnhancedInput
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "EnhancedInput.InputMappingContext.DefaultKeyMappings.Mappings" ), TEXT( "EnhancedInput.InputMappingContext.Mappings" ) ) );
#endif	

	bool Result = FCoreRedirects::AddRedirectList( Redirects, TEXT( "Downgrader" ) );
}
void RemoveRedirects()
{
	TArray<FCoreRedirect> Redirects;

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION == 3
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "AnimCurveBase.CurveName" ), TEXT( "LastObservedName" ) ) );

	bool Result = FCoreRedirects::RemoveRedirectList( Redirects, TEXT( "Downgrader->RemoveRedirects" ) );
#endif
}
void RemoveBlueprintRedirects( bool Undo )
{
	TArray<FCoreRedirect> Redirects;
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Function, TEXT( "KismetMathLibrary.Multiply_FloatFloat" ), TEXT( "Multiply_DoubleDouble" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Function, TEXT( "KismetMathLibrary.Divide_FloatFloat" ), TEXT( "Divide_DoubleDouble" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Function, TEXT( "KismetMathLibrary.Add_FloatFloat" ), TEXT( "Add_DoubleDouble" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Function, TEXT( "KismetMathLibrary.Subtract_FloatFloat" ), TEXT( "Subtract_DoubleDouble" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Function, TEXT( "KismetMathLibrary.Less_FloatFloat" ), TEXT( "Less_DoubleDouble" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Function, TEXT( "KismetMathLibrary.Greater_FloatFloat" ), TEXT( "Greater_DoubleDouble" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Function, TEXT( "KismetMathLibrary.LessEqual_FloatFloat" ), TEXT( "LessEqual_DoubleDouble" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Function, TEXT( "KismetMathLibrary.GreaterEqual_FloatFloat" ), TEXT( "GreaterEqual_DoubleDouble" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Function, TEXT( "KismetMathLibrary.EqualEqual_FloatFloat" ), TEXT( "EqualEqual_DoubleDouble" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Function, TEXT( "KismetMathLibrary.NotEqual_FloatFloat" ), TEXT( "NotEqual_DoubleDouble" ) ) );

	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Function, TEXT( "KismetStringLibrary.Conv_FloatToString" ), TEXT( "Conv_DoubleToString" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "KismetStringLibrary.Conv_DoubleToString.InFloat" ), TEXT( "Conv_DoubleToString.InDouble" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Function, TEXT( "KismetStringLibrary.Conv_StringToFloat" ), TEXT( "Conv_StringToDouble" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Function, TEXT( "KismetStringLibrary.BuildString_Float" ), TEXT( "BuildString_Double" ) ) );
	Redirects.Add( FCoreRedirect( ECoreRedirectFlags::Type_Property, TEXT( "KismetStringLibrary.BuildString_Double.InFloat" ), TEXT( "BuildString_Double.InDouble" ) ) );

	//RemoveFunctionFlag( "Multiply_FloatFloat", FUNC_None );

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4
	bool Result = false;
	if (Undo)
	{
		Result = FCoreRedirects::AddRedirectList( Redirects, TEXT( "Downgrader->RemoveBlueprintRedirects->Undo" ) );
	}
	else
		Result = FCoreRedirects::RemoveRedirectList( Redirects, TEXT( "Downgrader->RemoveBlueprintRedirects" ) );
#endif
}
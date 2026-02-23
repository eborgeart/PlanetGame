// Copyright Ciprian Stanciu 2024

#include "DowngraderModule.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"

#include "LevelEditor.h"

#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION < 26
#include "AssetRegistryModule.h"
#else
#include "AssetRegistry/AssetRegistryModule.h"
#endif
#include "IContentBrowserSingleton.h"
#include "ContentBrowserModule.h"
#include <functional>

#include "GameMapsSettings.h"
#include "Widgets/Layout/SScrollBox.h"
#include "HAL/FileManagerGeneric.h"
#include "UObject/SavePackage.h"
#include "Misc/ScopedSlowTask.h"
#include "UObject/DevObjectVersion.h"
#include "NiagaraSystem.h"
#include "EngineUtils.h"
#include "GameProjectGenerationModule.h"
#include "Interfaces/IProjectManager.h"
#include "Misc/FileHelper.h"
#include "Landscape.h"
#include "LandscapeInfo.h"
#include "Materials/MaterialInstanceConstant.h"
#include "InstancedFoliageActor.h"

//This is a compile optimization for 4.27 because we don't need to include these in builds where we don't downgrade and 4.27 hangs anyways
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
#include "Editor/MaterialEditor/Public/MaterialEditingLibrary.h"
#include "Materials/MaterialExpressionTransform.h"
#include "Materials/MaterialExpressionTransformPosition.h"
#include "Materials/MaterialExpressionPerInstanceCustomData.h"
#include "Materials/MaterialExpressionMaterialAttributeLayers.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionMakeMaterialAttributes.h"
#include "Materials/MaterialExpressionBreakMaterialAttributes.h"
#include "Materials/MaterialExpressionBlendMaterialAttributes.h"
#include "Materials/MaterialExpressionSetMaterialAttributes.h"
#include "Materials/MaterialExpressionGetMaterialAttributes.h"
#include "Materials/MaterialExpressionWorldPosition.h"
#include "Materials/MaterialExpressionComponentMask.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionTextureSample.h"
#include "Materials/MaterialExpressionTextureObject.h"
#include "Materials/MaterialExpressionTextureObjectParameter.h"
#include "Materials/MaterialExpressionFunctionInput.h"
#include "Materials/MaterialExpressionDotProduct.h"
#include "Materials/MaterialExpressionDistance.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionConstant2Vector.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "Materials/MaterialExpressionNormalize.h"
#include "Materials/MaterialExpressionCameraVectorWS.h"
#include "Materials/MaterialExpressionCameraPositionWS.h"
#include "Materials/MaterialExpressionPixelNormalWS.h"
#include "Materials/MaterialExpressionVertexNormalWS.h"
#include "Materials/MaterialExpressionPreSkinnedNormal.h"
#include "Materials/MaterialExpressionConstant4Vector.h"
#include "Materials/MaterialExpressionAppendVector.h"
#include "Materials/MaterialExpressionNamedReroute.h"
#include "Materials/MaterialExpressionEyeAdaptationInverse.h"
#include "Materials/MaterialExpressionStaticBool.h"
#include "Materials/MaterialExpressionViewProperty.h"
#include "VT/VirtualTextureBuilder.h"
#include "AnimGraphNode_Base.h"
#include "AnimGraphNode_StateMachine.h"
#include "AnimationStateMachineGraph.h"
#include "AnimStateTransitionNode.h"
#include "UObject/CoreRedirects.h"
#include "Animation/AnimCompressionDerivedDataPublic.h"
#include "AnimationGraphSchema.h"
#include "NiagaraScriptSourceBase.h"
#include "NiagaraVariant.h"
#include "NiagaraComponent.h"
#include "LevelSequence.h"
#include "Engine/LevelScriptBlueprint.h"
#include "Misc/Paths.h"
#include "ImageUtils.h"
#include "NavMesh/RecastVersion.h"
#include "GameFramework/NavMovementComponent.h"
#include "Interfaces/IProjectManager.h"
#include "ISourceControlModule.h"
#include "SourceControlOperations.h"
#include "K2Node_VariableGet.h"
#include "Engine/Selection.h"
#include "Factories/FbxSkeletalMeshImportData.h"
#include "Animation/AnimMontage.h"

#include "Runtime/Engine/Classes/Components/DirectionalLightComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/MultiLineEditableTextBox.h"
#include "Rendering/SkeletalMeshModel.h"
#include "RawMesh.h"

#include "StaticMeshAttributes.h"
#include "StaticMeshOperations.h"
#include "SkeletalRenderPublic.h"
#include "Engine/RendererSettings.h"
#include "Editor/UnrealEd/Public/FileHelpers.h"

#include "Misc/FileHelper.h"
#include "NiagaraEmitter.h"
#include "Animation/AnimSequence.h"

#include "RigVMModel/RigVMGraph.h"
#include "ParticleHelper.h"
#include "VT/VirtualTexture.h"
#include "Engine/VolumeTexture.h"
#include "AI/NavigationSystemConfig.h"
#include "EditorUtilityWidgetBlueprint.h"
#include "Components/EditableTextBox.h"
#include "Blueprint/WidgetTree.h"
#include "Materials/MaterialInstance.h"
#include "Materials/MaterialInstanceConstant.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "Landscape.h"
#include "LandscapeInfo.h"
#include "Engine/SkeletalMeshEditorData.h"
#include "Engine/MapBuildDataRegistry.h"
#include "ShadowMap.h"
#include "VT/LightmapVirtualTexture.h"
#include "Engine/ShadowMapTexture2D.h"
//#include "EnhancedInput/InputBlueprintNodes/K2Node_EnhancedInputAction.h"
#include "Serialization/LargeMemoryWriter.h"
#include "Settings/EditorLoadingSavingSettings.h"
#include "PropertyEditorModule.h"
#include "Widgets/Input/SButton.h"
#include "Modules/ModuleManager.h"
#include "Engine/PoseWatch.h"
#include "Materials/MaterialFunction.h"
#include "Misc/MessageDialog.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_FunctionResult.h"
#include "K2Node_CommutativeAssociativeBinaryOperator.h"
#include "K2Node_MacroInstance.h"
#include "K2Node_GetArrayItem.h"
#include "K2Node_GenericCreateObject.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/StructureEditorUtils.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "UserDefinedStructure/UserDefinedStructEditorData.h"

#include "UObject/FrameworkObjectVersion.h"
#include "UObject/EnterpriseObjectVersion.h"
#include "UObject/NiagaraObjectVersion.h"
//#include "Engine/Private/WorldSettingsCustomVersion.h"
#include "ControlRigObjectVersion.h"
#include "NiagaraCustomVersion.h"
#include "UObject/FortniteReleaseBranchCustomObjectVersion.h"
#include "Animation/AnimBoneCompressionSettings.h"
#include "Animation/AnimCurveCompressionSettings.h"
#include "Animation/VariableFrameStrippingSettings.h"
#include "InputMappingContext.h"
#include "Engine/PostProcessVolume.h"
#include "GroomAsset.h"

#endif

#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 27 || ENGINE_MAJOR_VERSION >= 5
	#include "NiagaraGraph.h"
	#include "EdGraphSchema_Niagara.h"
	#include "NiagaraScriptSource.h"
	#include "NiagaraNodeFunctionCall.h"
	#include "NiagaraNodeOutput.h"
	#include "ViewModels/Stack/NiagaraStackGraphUtilities.h"	
#endif

#if ENGINE_MAJOR_VERSION >= 5
	#include "WorldPartition/WorldPartition.h"
	#include "WorldPartition/WorldPartitionMiniMap.h"
	#include "WorldPartition/WorldPartitionEditorSpatialHash.h"
	#include "WorldPartition/WorldPartitionRuntimeSpatialHash.h"
	#include "WorldPartition/HLOD/HLODActor.h"
	#include "Components/DynamicMeshComponent.h"

	#include "GeometryCollection/GeometryCollectionObject.h"
	#include "GeometryCollection/GeometryCollectionComponent.h"
	#include "GeometryCollection/GeometryCollectionSimulationTypes.h"
	#include "GeometryCollection/GeometryCollectionEngineSizeSpecificUtility.h"

	#include "Materials/MaterialExpressionComposite.h"
	#include "Materials/MaterialExpressionDoubleVectorParameter.h"
	#include "Materials/MaterialExpressionGenericConstant.h"
	#include "StaticMeshCompiler.h"
	#include "ShaderCompiler.h"
	#include "TextureCompiler.h"
	#include "AssetCompilingManager.h"

	#include "K2Node_PromotableOperator.h"
	#include "AnimGraph/AnimGraphNode_OrientationWarping.h"
	#include "AnimGraph/AnimGraphNode_SlopeWarping.h"
	#include "AnimGraph/AnimGraphNode_StrideWarping.h"
	#include "LevelInstance/LevelInstanceActor.h"
	#include "MetasoundSource.h"
	#include "InterchangeAssetImportData.h"
	#include "Materials/MaterialExpressionPathTracingQualitySwitch.h"
	#include "PackedLevelActor/PackedLevelActor.h"

	#include "Runtime/Engine/Private/WorldSettingsCustomVersion.h"
	#include "UObject/UE5LWCRenderingStreamObjectVersion.h"
	#include "IKRigObjectVersion.h"
	#include "UObject/PackageTrailer.h"
	#include "Animation/AnimData/AnimDataModel.h"
	#include "Kismet2/WildcardNodeUtils.h"
#endif
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION == 0
	#include "AssetRegistry/Private/PackageReader.h"
#endif
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
	#include "Engine/SkinnedAssetCommon.h"
	#include "AnimGraph/AnimGraphNode_FootPlacement.h"
	#include "AnimGraph/AnimGraphNode_OffsetRootBone.h"
	#include "RigVMModel/Nodes/RigVMAggregateNode.h"
	#include "WorldPartition/WorldPartitionActorDescArchive.h"
#endif
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1 && ENGINE_MINOR_VERSION <= 3
	#include "Materials/MaterialExpressionStrata.h"
#endif

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 2
	#include "StaticMeshComponentLODInfo.h"
	#include "Materials/MaterialExpressionSwitch.h"
	#include "PCGCustomVersion.h"
	#include "Dataflow/DataflowObject.h"
	#include "Materials/MaterialAttributeDefinitionMap.h"
#endif
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
	#include "RigVMObjectVersion.h"
	#include "InterchangeCustomVersion.h"
	#include "AnimStateAliasNode.h"
	#include "Materials/MaterialExpressionDistanceFieldApproxAO.h"
	#include "NiagaraVolumeRendererProperties.h"
	#include "Materials/MaterialExpressionLength.h"
	#include "Materials/MaterialExpressionRgbToHsv.h"
	#include "Materials/MaterialExpressionHsvToRgb.h"
	#include "UObject/FortniteValkyrieBranchObjectVersion.h"
#endif
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3 && ENGINE_MINOR_VERSION <= 5
	#include "AssetRegistry/PackageReader.h"
#endif
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4
	#include "Materials/MaterialExpressionSubstrate.h"
	#include "PoseSearch/PoseSearchDatabase.h"
	#include "AnimationBlendStackGraph.h"
	#include "AnimGraph/AnimGraphNode_Steering.h"
#endif
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5
	#include "TextureSourceDataUtils.h"
	#include "WorldPartition/HLOD/HLODSourceActorsFromCell.h"
	#include "LandscapeEditLayer.h"
#endif
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 6
	#include "Runtime/AssetRegistry/Internal/AssetRegistry/PackageReader.h"
	#include "UObject/UE5SpecialProjectStreamObjectVersion.h"
	#include "DataHierarchyViewModelBase.h"
#endif

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 7
	#include "ControlRigBlueprintLegacy.h"
#else
	#include "ControlRigBlueprint.h"
#endif

#define LOCTEXT_NAMESPACE "FDowngraderModule"

template<class TYPE>
void RegisterObjectClassWithEngineOuter(FString NewName)
{
	UClass* StaticMeshClass = UStaticMesh::StaticClass();
	UObject* EngineOuter = StaticMeshClass->GetOuter();

	UObject* NewObjectOfType = NewObject< TYPE>();
	UClass* TheClass = NewObjectOfType->GetClass();
	TheClass->Rename( *NewName, EngineOuter );
	TheClass->AddToRoot();//Prevent GC
}

void RegisterObjectClassWithEngineOuter( UObject* Object )
{
	UClass* StaticMeshClass = UStaticMesh::StaticClass();
	UObject* EngineOuter = StaticMeshClass->GetOuter();

	UClass* TheClass = Object->GetClass();
	TheClass->Rename( *TheClass->GetName(), EngineOuter);
}
template<class TYPE>
void RegisterStructWithEngineOuter( FString NewName )
{
	UClass* StaticMeshClass = UStaticMesh::StaticClass();
	UObject* EngineOuter = StaticMeshClass->GetOuter();

	//auto NewObjectOfType = NewObject< TYPE>();
	UStruct* TheStruct = TYPE::StaticStruct();
	TheStruct->Rename( *NewName, EngineOuter );
}
template<int N>
void AddCustomVersion( FGuid InKey, int32 Version, const TCHAR( &InFriendlyName )[ N ] )
{
	FDevVersionRegistration* RemoveRenderingObjectVersion = new FDevVersionRegistration( InKey, Version, InFriendlyName );
	FCustomVersionContainer Container = FCurrentCustomVersions::GetAll();//Process Que for addition
}
void AddRedirects();
void RemoveRedirects();
void RemoveBlueprintRedirects( bool Undo = false );
void FDowngraderModule::StartupModule()
{
	//return;
	FDowngrader_Commands::Register();
	BindEditorCommands();
	
	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>( "LevelEditor" );
	TSharedPtr<FExtender> ToolbarExtender = MakeShareable( new FExtender );
	ToolbarExtender->AddToolBarExtension(
	#if ENGINE_MAJOR_VERSION == 4
		"Game",
	#else
		"File",
	#endif
		EExtensionHook::After,
		NULL,
		FToolBarExtensionDelegate::CreateRaw( this, &FDowngraderModule::AddCustomMenu )
	);

	LevelEditorModule.GetToolBarExtensibilityManager()->AddExtender( ToolbarExtender );

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION == 0
	UClass* StaticMeshClass = UStaticMesh::StaticClass();
	UObject* EngineOuter = StaticMeshClass->GetOuter();

	//RegisterObjectClassWithEngineOuter< UMaterialEditorOnlyData_Downgrader>( TEXT("MaterialEditorOnlyData"));
	//RegisterObjectClassWithEngineOuter< UMaterialInstanceEditorOnlyData_Downgrader>( TEXT( "MaterialInstanceEditorOnlyData" ) );
	//RegisterObjectClassWithEngineOuter< UMaterialFunctionEditorOnlyData_Downgrader>( TEXT( "MaterialFunctionEditorOnlyData" ) );
	//RegisterStructWithEngineOuter< FMaterialExpressionCollection_Downgrader>( TEXT( "MaterialExpressionCollection" ) );
	//RegisterStructWithEngineOuter< FStaticParameterSetEditorOnlyData_Downgrader>( TEXT( "StaticParameterSetEditorOnlyData" ) );
	
#endif

	AddRedirects();
	RemoveRedirects();

	#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION == 0
	bool Value = false;
	GConfig->GetBool( TEXT( "Core.System" ), TEXT( "UsePackageTrailer" ), Value, GEngineIni );
	if( !Value )
	{
		FString DialogText = FString::Printf( TEXT(
			"Downgrader Error!\n\n"
			"UsePackageTrailer is not enabled ! Imports may crash the editor!\n"
			"Go to Config/DefaultEngine.ini and add\n"
			"\n"
			"[Core.System]\n"
			"UsePackageTrailer=True\n"
		) );
		FText Txt = FText::FromString( DialogText );
		FMessageDialog::Open( EAppMsgType::Ok, Txt );
	}
	#endif
	UEditorLoadingSavingSettings* EditorSettings = GetMutableDefault< UEditorLoadingSavingSettings >();
	//Disable autosave so modified materials don't get saved
	if( EditorSettings )
	{
		EditorSettings->bAutoSaveEnable = 0;
	}
	//Reset startup map so in case of crashes, at least don't crash the editor at startup
	UGameMapsSettings* GameMapsSettings = GetMutableDefault< UGameMapsSettings >();
	if( GameMapsSettings )
	{
		if( GameMapsSettings->EditorStartupMap.IsValid() )
		{
			GameMapsSettings->EditorStartupMap.Reset();
		}
	}
	
	FCoreDelegates::OnAsyncLoadingFlushUpdate.AddRaw( this, &FDowngraderModule::GlobalPostLoad );

	FEngineVersion& CurrentVersion = (FEngineVersion&)FEngineVersion::Current();
	FEngineVersion& CompatibleWith = (FEngineVersion&)FEngineVersion::CompatibleWith();

	UE_LOG( LogTemp, Warning, TEXT("CurrentVersion.Set( %d, %d, %d, %d, TEXT(\"%s\") );" ), CurrentVersion.GetMajor(), CurrentVersion.GetMinor(), CurrentVersion.GetPatch(),
			CurrentVersion.GetChangelist(), *CurrentVersion.GetBranch() );
}
void SpoofEngineVersion( EEngineVersion TargetVersion )
{
	if (TargetVersion == EEngineVersion::EV_5_7)
	{
		FEngineVersion& CurrentVersion = (FEngineVersion&)FEngineVersion::Current();
		FEngineVersion& CompatibleWith = (FEngineVersion&)FEngineVersion::CompatibleWith();
		CurrentVersion.Set( 5, 7, 3, 50162420, TEXT( "++UE5+Release-5.7" ) );
		CompatibleWith.Set( 5, 7, 0, 47537391, TEXT( "++UE5+Release-5.7" ) );
	}
	if (TargetVersion == EEngineVersion::EV_5_6)
	{
		FEngineVersion& CurrentVersion = (FEngineVersion&)FEngineVersion::Current();
		FEngineVersion& CompatibleWith = (FEngineVersion&)FEngineVersion::CompatibleWith();
		CurrentVersion.Set( 5, 6, 1, 44394996, TEXT( "++UE5+Release-5.6" ) );
		CompatibleWith.Set( 5, 6, 0, 43139311, TEXT( "++UE5+Release-5.6" ) );
	}
	if (TargetVersion == EEngineVersion::EV_5_5)
	{
		FEngineVersion& CurrentVersion = (FEngineVersion&)FEngineVersion::Current();
		FEngineVersion& CompatibleWith = (FEngineVersion&)FEngineVersion::CompatibleWith();
		CurrentVersion.Set( 5, 5, 4, 40574608, TEXT( "++UE5+Release-5.5" ) );
		CompatibleWith.Set( 5, 5, 0, 37670630, TEXT( "++UE5+Release-5.5" ) );
	}
	if (TargetVersion == EEngineVersion::EV_5_4)
	{
		FEngineVersion& CurrentVersion = (FEngineVersion&)FEngineVersion::Current();
		FEngineVersion& CompatibleWith = (FEngineVersion&)FEngineVersion::CompatibleWith();
		CurrentVersion.Set( 5, 4, 4, 35576357, TEXT( "++UE5+Release-5.4" ) );
		CompatibleWith.Set( 5, 4, 0, 32235091, TEXT( "++UE5+Release-5.4" ) );
	}
	if (TargetVersion == EEngineVersion::EV_5_3)
	{
		FEngineVersion& CurrentVersion = (FEngineVersion&)FEngineVersion::Current();
		FEngineVersion& CompatibleWith = (FEngineVersion&)FEngineVersion::CompatibleWith();
		CurrentVersion.Set( 5, 3, 2, 29314046, TEXT( "++UE5+Release-5.3" ) );
		CompatibleWith.Set( 5, 3, 0, 27405482, TEXT( "++UE5+Release-5.3" ) );
	}
	if (TargetVersion == EEngineVersion::EV_5_2)
	{
		FEngineVersion& CurrentVersion = (FEngineVersion&)FEngineVersion::Current();
		FEngineVersion& CompatibleWith = (FEngineVersion&)FEngineVersion::CompatibleWith();
		CurrentVersion.Set( 5, 2, 1, 26001984, TEXT( "++UE5+Release-5.2" ) );
		CompatibleWith.Set( 5, 2, 0, 25360045, TEXT( "++UE5+Release-5.2" ) );
	}
	if (TargetVersion == EEngineVersion::EV_5_1)
	{
		FEngineVersion& CurrentVersion = (FEngineVersion&)FEngineVersion::Current();
		FEngineVersion& CompatibleWith = (FEngineVersion&)FEngineVersion::CompatibleWith();
		CurrentVersion.Set( 5, 1, 1, 23901901, TEXT( "++UE5+Release-5.1" ) );
		CompatibleWith.Set( 5, 1, 0, 23058290, TEXT( "++UE5+Release-5.1" ) );
	}
	if (TargetVersion == EEngineVersion::EV_5_0)
	{
		FEngineVersion& CurrentVersion = (FEngineVersion&)FEngineVersion::Current();
		FEngineVersion& CompatibleWith = (FEngineVersion&)FEngineVersion::CompatibleWith();
		CurrentVersion.Set( 5, 0, 3, 20979098, TEXT( "++UE5+Release-5.0" ) );
		CompatibleWith.Set( 5, 0, 0, 19505902, TEXT( "++UE5+Release-5.0" ) );
	}
	if (TargetVersion == EEngineVersion::EV_4_27)
	{
		FEngineVersion& CurrentVersion = (FEngineVersion&)FEngineVersion::Current();
		FEngineVersion& CompatibleWith = (FEngineVersion&)FEngineVersion::CompatibleWith();
		CurrentVersion.Set( 4, 27, 2, 18319896, TEXT( "++UE4+Release-4.27" ) );
		CompatibleWith.Set( 4, 27, 0, 17155196, TEXT( "++UE4+Release-4.27" ) );
	}
	if (TargetVersion == EEngineVersion::EV_4_26)
	{
		FEngineVersion& CurrentVersion = (FEngineVersion&)FEngineVersion::Current();
		FEngineVersion& CompatibleWith = (FEngineVersion&)FEngineVersion::CompatibleWith();
		CurrentVersion.Set( 4, 26, 2, 15973114, TEXT( "++UE4+Release-4.26" ) );
		CompatibleWith.Set( 4, 26, 0, 14830424, TEXT( "++UE4+Release-4.26" ) );
	}
}
UDowngraderParams::UDowngraderParams()
{
	TargetVersion = EEngineVersion::EV_4_27;
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5 && !defined DOWNGRADER_CUSTOM_ENGINE
	TargetVersion = EEngineVersion::EV_5_4;
#endif
	//ExternalActors = false;
}
UDowngraderParams* DowngraderSettings = nullptr;
class SFunctionParamDialog : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(SFunctionParamDialog) {}

	/** Text to display on the "OK" button */
	SLATE_ARGUMENT(FText, OkButtonText)

	/** Tooltip text for the "OK" button */
	SLATE_ARGUMENT(FText, OkButtonTooltipText)

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TWeakPtr<SWindow> InParentWindow)
	{
		bOKPressed = false;

		// Initialize details view
		FDetailsViewArgs DetailsViewArgs;
		{
			DetailsViewArgs.bAllowSearch = false;
			DetailsViewArgs.bHideSelectionTip = true;
			DetailsViewArgs.bLockable = false;
			DetailsViewArgs.bSearchInitialKeyFocus = true;
			DetailsViewArgs.bUpdatesFromSelection = false;
			DetailsViewArgs.bShowOptions = false;
			DetailsViewArgs.bShowModifiedPropertiesOption = false;
			#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 0
				DetailsViewArgs.bShowObjectLabel = false;
			#else
				DetailsViewArgs.bShowActorLabel = false;
			#endif
			DetailsViewArgs.bForceHiddenPropertyVisibility = true;
			DetailsViewArgs.bShowScrollBar = false;
		}
	
		FStructureDetailsViewArgs StructureViewArgs;
		{
			StructureViewArgs.bShowObjects = true;
			StructureViewArgs.bShowAssets = true;
			StructureViewArgs.bShowClasses = true;
			StructureViewArgs.bShowInterfaces = true;
		}

		FPropertyEditorModule& PropertyEditorModule = FModuleManager::Get().LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
		TSharedPtr<class IDetailsView> StructureDetailsView = PropertyEditorModule.CreateDetailView( DetailsViewArgs );

		ChildSlot
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				SNew(SScrollBox)
				+SScrollBox::Slot()
				[
					StructureDetailsView.ToSharedRef()
				]
			]
			+SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SBorder)
				.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Right)
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					.Padding(2.0f)
					.AutoWidth()
					[
						SNew(SButton)
						.ButtonStyle(FEditorStyle::Get(), "FlatButton.Success")
						.ForegroundColor(FLinearColor::White)
						.ContentPadding(FMargin(6, 2))
						.OnClicked_Lambda([this, InParentWindow, InArgs]()
						{
							if(InParentWindow.IsValid())
							{
								InParentWindow.Pin()->RequestDestroyWindow();
							}
							bOKPressed = true;
							return FReply::Handled(); 
						})
						.ToolTipText(InArgs._OkButtonTooltipText)
						[
							SNew(STextBlock)
							.TextStyle(FEditorStyle::Get(), "ContentBrowser.TopBar.Font")
							.Text(InArgs._OkButtonText)
						]
					]
					+SHorizontalBox::Slot()
					.Padding(2.0f)
					.AutoWidth()
					[
						SNew(SButton)
						.ButtonStyle(FEditorStyle::Get(), "FlatButton")
						.ForegroundColor(FLinearColor::White)
						.ContentPadding(FMargin(6, 2))
						.OnClicked_Lambda([InParentWindow]()
						{ 
							if(InParentWindow.IsValid())
							{
								InParentWindow.Pin()->RequestDestroyWindow();
							}
							return FReply::Handled(); 
						})
						[
							SNew(STextBlock)
							.TextStyle(FEditorStyle::Get(), "ContentBrowser.TopBar.Font")
							.Text(LOCTEXT("Cancel", "Cancel"))
						]
					]
				]
			]
		];

		StructureDetailsView->SetObject( DowngraderSettings );
	}

	bool bOKPressed;
};
void FDowngraderModule::GlobalPostLoad()
{
//#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 0
//	for( int i = 0; i < GlobalPostLoads.Num(); i++ )
//	{
//		UObject* Obj = GlobalPostLoads[ i ];
//		UMaterial* Mat = Cast<UMaterial>( Obj );
//		if( Mat )
//		{
//			Mat->UpdateCachedExpressionData();
//		}
//		Obj->RemoveFromRoot();
//	}
//	GlobalPostLoads.Reset( 0 );
//#endif
}

#define MapActionHelper(x) EditorCommands->MapAction( Commands.x, FExecuteAction::CreateRaw(this, &FDowngraderModule::x), FCanExecuteAction(), FIsActionChecked())

void FDowngraderModule::BindEditorCommands()
{
	if( !EditorCommands.IsValid() )
	{
		EditorCommands = MakeShareable( new FUICommandList() );
	}

	const FDowngrader_Commands& Commands = FDowngrader_Commands::Get();	
		
	MapActionHelper( DowngradeSelectedAssets );
	MapActionHelper( PreloadSelectedAssets );
	MapActionHelper( SaveSelectedAssets );	
	MapActionHelper( RevertSubstrateMaterials );
	MapActionHelper( FixLandscape );
	MapActionHelper( WriteCustomVersions );
	MapActionHelper( DeleteUnloadedWorldPartitionActors );
	MapActionHelper( RemoveWorldPartitionFromSelectedAssets );
	MapActionHelper( BreakPackedActors );
	MapActionHelper( UpdateUEBuild );
	MapActionHelper( UpdateAllPluginVersions );	
}
void FDowngraderModule::AddCustomMenu( FToolBarBuilder& ToolbarBuilder )
{
	ToolbarBuilder.BeginSection( "Downgrader" );
#if ENGINE_MAJOR_VERSION >= 5
	ToolbarBuilder.SetLabelVisibility( EVisibility::Visible );
#endif
	{
		ToolbarBuilder.AddComboButton(
			FUIAction()
			, FOnGetContent::CreateRaw( this, &FDowngraderModule::CreateMenuContent )
			, LOCTEXT( "DowngraderLabel", "Downgrader" )
			, LOCTEXT( "DowngraderTooltip", "Downgrader" )
			, FSlateIcon( FEditorStyle::GetStyleSetName(), "LevelEditor.GameSettings" )
		);
	}
	ToolbarBuilder.EndSection();
}
TSharedRef<SWidget> FDowngraderModule::CreateMenuContent()
{
	FMenuBuilder MenuBuilder( true, EditorCommands );
	
	MenuBuilder.AddMenuEntry( FDowngrader_Commands::Get().DowngradeSelectedAssets );
	MenuBuilder.AddMenuEntry( FDowngrader_Commands::Get().PreloadSelectedAssets );
	MenuBuilder.AddMenuEntry( FDowngrader_Commands::Get().SaveSelectedAssets );
	MenuBuilder.AddMenuEntry( FDowngrader_Commands::Get().RevertSubstrateMaterials );
	MenuBuilder.AddMenuEntry( FDowngrader_Commands::Get().FixLandscape );
	MenuBuilder.AddMenuEntry( FDowngrader_Commands::Get().WriteCustomVersions );
#if ENGINE_MAJOR_VERSION >= 5
	MenuBuilder.AddMenuEntry( FDowngrader_Commands::Get().DeleteUnloadedWorldPartitionActors );
	MenuBuilder.AddMenuEntry( FDowngrader_Commands::Get().RemoveWorldPartitionFromSelectedAssets );
	MenuBuilder.AddMenuEntry( FDowngrader_Commands::Get().BreakPackedActors );
#endif
	MenuBuilder.AddMenuEntry( FDowngrader_Commands::Get().UpdateUEBuild );
	MenuBuilder.AddMenuEntry( FDowngrader_Commands::Get().UpdateAllPluginVersions );
	
	return MenuBuilder.MakeWidget();
}

void FDowngrader_Commands::RegisterCommands()
{
	UI_COMMAND( DowngradeSelectedAssets, "DowngradeSelectedAssets", "DowngradeSelectedAssets", EUserInterfaceActionType::Button, FInputChord() );
	UI_COMMAND( PreloadSelectedAssets, "PreloadSelectedAssets", "PreloadSelectedAssets", EUserInterfaceActionType::Button, FInputChord() );
	UI_COMMAND( SaveSelectedAssets, "SaveSelectedAssets", "SaveSelectedAssets", EUserInterfaceActionType::Button, FInputChord() );	
	UI_COMMAND( RevertSubstrateMaterials, "RevertSubstrateMaterials", "RevertSubstrateMaterials", EUserInterfaceActionType::Button, FInputChord() );
	UI_COMMAND( FixLandscape, "FixLandscape", "FixLandscape", EUserInterfaceActionType::Button, FInputChord() );
	UI_COMMAND( WriteCustomVersions, "WriteCustomVersions", "WriteCustomVersions", EUserInterfaceActionType::Button, FInputChord() );
	UI_COMMAND( DeleteUnloadedWorldPartitionActors, "DeleteUnloadedWorldPartitionActors", "DeleteUnloadedWorldPartitionActors", EUserInterfaceActionType::Button, FInputChord() );
	UI_COMMAND( RemoveWorldPartitionFromSelectedAssets, "RemoveWorldPartitionFromSelectedAssets", "RemoveWorldPartitionFromSelectedAssets", EUserInterfaceActionType::Button, FInputChord() );
	UI_COMMAND( BreakPackedActors, "BreakPackedActors", "BreakPackedActors", EUserInterfaceActionType::Button, FInputChord() );
	UI_COMMAND( UpdateUEBuild, "UpdateUEBuild", "UpdateUEBuild", EUserInterfaceActionType::Button, FInputChord() );
	UI_COMMAND( UpdateAllPluginVersions, "UpdateAllPluginVersions", "UpdateAllPluginVersions", EUserInterfaceActionType::Button, FInputChord() );
}

void FDowngraderModule::ShutdownModule()
{	
}
template<int N>
void ChangeCustomVersion( FGuid InKey, int32 Version, const TCHAR( &InFriendlyName )[ N ] )
{
	{
		FDevVersionRegistration RemoveRenderingObjectVersion( InKey, Version, InFriendlyName );
		RemoveRenderingObjectVersion.~FDevVersionRegistration();
	}
	FCustomVersionContainer Container = FCurrentCustomVersions::GetAll();//Process Que for removals
	FDevVersionRegistration* DowngradeRenderingObjectVersion = new FDevVersionRegistration( InKey, Version, InFriendlyName );
	FCustomVersionContainer Container2 = FCurrentCustomVersions::GetAll();//Process Que for Addition
}
template<int N>
void RemoveCustomVersion( FGuid InKey, int32 Version, const TCHAR( &InFriendlyName )[ N ] )
{
	{
		FDevVersionRegistration RemoveRenderingObjectVersion( InKey, Version, InFriendlyName );
		RemoveRenderingObjectVersion.~FDevVersionRegistration();
	}
	FCustomVersionContainer Container = FCurrentCustomVersions::GetAll();//Process Que for removals
}
int GetCustomVersion( FGuid GUID )
{
	FCustomVersionContainer Container = FCurrentCustomVersions::GetAll();
	for( int i = 0; i < Container.GetAllVersions().Num(); i++ )
	{
		if( Container.GetAllVersions()[ i ].Key == GUID )
			return Container.GetAllVersions()[ i ].Version;
	}

	return -1;
}
const FCustomVersion GetCustomVersionByGUID( FGuid GUID, bool& Exists )
{
	FCustomVersionContainer Container = FCurrentCustomVersions::GetAll();
	const FCustomVersionArray& AllVersion = Container.GetAllVersions();
	for( int i = 0; i < AllVersion.Num(); i++ )
	{
		if( AllVersion[ i ].Key == GUID )
		{
			const FCustomVersion& CustomVersion = AllVersion[ i ];
			Exists = true;
			return CustomVersion;
		}
	}
	FCustomVersion Blank;
	Exists = false;
	return Blank;
}
bool IsPluginEnabled( const FString& PluginName )
{
	TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin( *PluginName );
	return Plugin.IsValid() && Plugin->IsEnabled();
}
FString AllVersionStrings[] = {
			TEXT( "4.26" ),
			TEXT( "4.27" ),
			TEXT( "5.0" ),
			TEXT( "5.1" ),
			TEXT( "5.2" ),
			TEXT( "5.3" ),
			TEXT( "5.4" ),
			TEXT( "5.5" ),
			TEXT( "5.6" ),
			TEXT( "5.7" )
	};
const FConfigSection* GetCustomVersionSectionFor( EEngineVersion Version )
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4
	TSharedPtr<IPlugin> ThisPlugin = IPluginManager::Get().FindPlugin( TEXT( "Downgrader" ) );

	FString TargetVersion = FString::Printf( TEXT( "CustomVersions_%s" ), *AllVersionStrings[ (int)Version ] );
	FString VersionIniFile = ThisPlugin->GetBaseDir() / FString::Printf( TEXT( "Config/%s.ini" ), *TargetVersion );

	auto Section = GConfig->GetSection( *TargetVersion, false, VersionIniFile );
	return Section;
#else
	return nullptr;
#endif
}
const FConfigSection* GetNiagaraVersionsFor( EEngineVersion Version )
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4
	TSharedPtr<IPlugin> ThisPlugin = IPluginManager::Get().FindPlugin( TEXT( "Downgrader" ) );

	FString TargetVersion = FString::Printf( TEXT( "NiagaraVersions_%s" ), *AllVersionStrings[(int)Version] );

	FString NiagaraIniFile = ThisPlugin->GetBaseDir() / FString::Printf( TEXT( "Config/%s.ini" ), *TargetVersion );
	//FConfigFile& NiagaraConfigFile = GConfig->Add( NiagaraIniFile, FConfigFile() );
	FString NiagaraSection = TEXT( "Modules" );
	const FConfigSection* Section = GConfig->GetSection( *NiagaraSection, false, NiagaraIniFile );
	return Section;
#else
	return nullptr;
#endif
}
EEngineVersion GetCurrentVersion()
{
	EEngineVersion CurrentVersion = EEngineVersion::EV_4_27;

#if ENGINE_MAJOR_VERSION == 4
	CurrentVersion = (EEngineVersion)( (int) EEngineVersion::EV_4_26 + ENGINE_MINOR_VERSION - 26 );
#endif
#if ENGINE_MAJOR_VERSION == 5
	CurrentVersion = (EEngineVersion)((int)EEngineVersion::EV_5_0 + ENGINE_MINOR_VERSION);
#endif

	return CurrentVersion;
}
void GetCustomVesionsThatAreMissingFor( EEngineVersion Version, TArray<FCustomVersion>& MissingVersions )
{
	const FConfigSection* TargetSection = GetCustomVersionSectionFor( Version );
	EEngineVersion CurrentVersion = GetCurrentVersion();
	const FConfigSection* LatestSection = GetCustomVersionSectionFor( CurrentVersion );

	for( auto It = LatestSection->CreateConstIterator(); It; ++It )
	{
		//FName NameKey = It->Key;
		//FConfigValue NameValue = It->Value;
		//FString NameSavedValue = NameValue.GetSavedValue();
		//++It;
		FName GUIDKey = It->Key;
		FConfigValue GUIDValue = It->Value;
		FString GUIDSavedValue = GUIDValue.GetSavedValue();

		FGuid GUID;
		bool IsValidGUID = FGuid::Parse( GUIDKey.ToString(), GUID );
		if ( IsValidGUID )
		{
			const FConfigValue* ValueThis = TargetSection->Find( GUIDKey );
			if( ValueThis )
			{

			}
			else
			{
			
				bool Exists = false;
				FCustomVersion ExistingCustomVersion = GetCustomVersionByGUID( GUID, Exists);
				if ( Exists )
					MissingVersions.Add( ExistingCustomVersion );
			}
		}
	}
}
void SetCustomVersionsFor( EEngineVersion Version )
{
	const FConfigSection* Section = GetCustomVersionSectionFor( Version );
	FString TargetVersion = FString::Printf( TEXT( "CustomVersions_%s" ), *AllVersionStrings[ (int)Version ] );

	for( auto It = Section->CreateConstIterator(); It; ++It )
	{
		//FName NameKey = It->Key;
		//FConfigValue NameValue = It->Value;
		//FString NameSavedValue = NameValue.GetSavedValue();
		//++It;
		FName GUIDKey = It->Key;
		FConfigValue GUIDValue = It->Value;
		FString GUIDSavedValue = GUIDValue.GetSavedValue();

		//if( NameSavedValue.Compare( GUIDSavedValue ) != 0 )
		//{
		//	FString Text = FString::Printf( TEXT( "CustomVersions config file %s is broken !" ), *TargetVersion );
		//	FText Txt = FText::FromString( Text );
		//	FMessageDialog::Open( EAppMsgType::Ok, Txt );
		//	return;
		//}
		FGuid GUID;
		bool Parsed = FGuid::Parse( GUIDKey.ToString(), GUID);
		if ( Parsed )
		{
			bool Exists = false;
			FCustomVersion ExistingCustomVersion = GetCustomVersionByGUID( GUID, Exists );
			if( !Exists )
			{
				//this means that custom version got deleted in the latest release
				continue;
			}
			//Sometimes Friendly names do change between versions !
			//if( ExistingCustomVersion.GetFriendlyName().ToString().Compare( NameKey.ToString() ) != 0 )
			//{
			//	FString Text = FString::Printf( TEXT( "CustomVersions config file %s is broken !" ), *TargetVersion );
			//	FText Txt = FText::FromString( Text );
			//	FMessageDialog::Open( EAppMsgType::Ok, Txt );
			//	return;
			//}

			int CustomVersionValue = _wtoi( *GUIDSavedValue );
			TCHAR TCharString[ 128 ] = { 0 };
			FString Name = ExistingCustomVersion.GetFriendlyName().ToString();
			memcpy( TCharString, *Name, Name.Len() * 2 );
			ChangeCustomVersion( GUID, CustomVersionValue, TCharString );
		}
	}

	TArray<FCustomVersion> MissingVersions;
	GetCustomVesionsThatAreMissingFor( Version, MissingVersions );
	for( int i = 0; i < MissingVersions.Num(); i++ )
	{
		FCustomVersion& CustomVersion = MissingVersions[ i ];
		TCHAR TCharString[ 128 ] = { 0 };
		memcpy( TCharString, *CustomVersion.GetFriendlyName().ToString(), CustomVersion.GetFriendlyName().ToString().Len() * 2 );
		int BeforeCustomVersionWasAdded = -1;//usually 0 but some require -1
		ChangeCustomVersion( CustomVersion.Key, BeforeCustomVersionWasAdded, TCharString );
	}
}
bool ReadDowngradeLastBatch( TArray<FAssetData>& AllAssets, EEngineVersion& TargetVersion )
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>( "AssetRegistry" );
	TSharedPtr<IPlugin> ThisPlugin = IPluginManager::Get().FindPlugin( TEXT( "Downgrader" ) );
	FString LastBatchIniFile = ThisPlugin->GetBaseDir() / FString::Printf( TEXT( "Config/LastBatch.ini" ) );

	FString Category = TEXT( "LastBatch" );
	int NumAssets = -1;
	int StartIndex = -1;
	int TargetVersionInt = -1;
	bool Result = GConfig->GetInt( *Category, TEXT( "NumFiles" ), NumAssets, *LastBatchIniFile );
	Result = GConfig->GetInt( *Category, TEXT( "StartIndex" ), StartIndex, *LastBatchIniFile );
	Result = GConfig->GetInt( *Category, TEXT( "TargetVersion" ), TargetVersionInt, *LastBatchIniFile );
	if (Result && NumAssets > 0 && StartIndex >= 0 && StartIndex < NumAssets )
	{
		TargetVersion = (EEngineVersion)TargetVersionInt;

		for (int i = StartIndex; i < NumAssets; i++)
		{
			FString Key = FString::Printf( TEXT( "File%d" ), i );
			FString Value;
			Result = GConfig->GetString( *Category, *Key, Value, *LastBatchIniFile );

			TArray<FAssetData> OutAssetData;
			AssetRegistryModule.Get().GetAssetsByPackageName( FName( Value ), OutAssetData, true );
			for (int a = 0; a < OutAssetData.Num();a++)
			{
				AllAssets.Add( OutAssetData[a] );
			}
		}

		return true;
	}

	return false;
}
void WriteDowngradeLastBatch( TArray<FAssetData> AllAssets, EEngineVersion TargetVersion )
{
	TSharedPtr<IPlugin> ThisPlugin = IPluginManager::Get().FindPlugin( TEXT( "Downgrader" ) );
	FString LastBatchIniFile = ThisPlugin->GetBaseDir() / FString::Printf( TEXT( "Config/LastBatch.ini" ) );
	FString Category = TEXT( "LastBatch" );

	FConfigFile& LastBatchConfigFile = GConfig->Add( LastBatchIniFile, FConfigFile() );

	int NumAssets = AllAssets.Num();
	GConfig->SetInt( *Category, TEXT( "NumFiles" ), NumAssets, *LastBatchIniFile );
	GConfig->SetInt( *Category, TEXT( "TargetVersion" ), (int)TargetVersion, *LastBatchIniFile );

	for (int i = 0; i < NumAssets; i++)
	{
		const FAssetData& AssetData = AllAssets[i];

		FString Key = FString::Printf( TEXT( "File%d" ), i );
		GConfig->SetString( *Category, *Key, *AssetData.PackageName.ToString(), *LastBatchIniFile );
	}

	GConfig->Flush( false, LastBatchIniFile );
}
void UpdateLastBatch( int StartIndex )
{
	TSharedPtr<IPlugin> ThisPlugin = IPluginManager::Get().FindPlugin( TEXT( "Downgrader" ) );
	FString LastBatchIniFile = ThisPlugin->GetBaseDir() / FString::Printf( TEXT( "Config/LastBatch.ini" ) );
	FString Category = TEXT( "LastBatch" );

	GConfig->SetInt( *Category, TEXT( "StartIndex" ), StartIndex, *LastBatchIniFile );
	GConfig->Flush( false, LastBatchIniFile );
}
TArray<FAssetData> GetAllSelectedAssets()
{
	IContentBrowserSingleton& ContentBrowser = FModuleManager::LoadModuleChecked<FContentBrowserModule>( "ContentBrowser" ).Get();

	TArray<FAssetData> AllAssets;
	TArray<FAssetData> SelectedAssets;
	ContentBrowser.GetSelectedAssets( SelectedAssets );
	for( int i = 0; i < SelectedAssets.Num(); i++ )
		AllAssets.Add( SelectedAssets[ i ] );
	TArray<FString> SelectedFolders;
	ContentBrowser.GetSelectedFolders( SelectedFolders );
	//ContentBrowser.GetSelectedPathViewFolders( SelectedFolders );

	if( SelectedFolders.Num() > 0 )
	{
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>( "AssetRegistry" );

		for( int f = 0; f < SelectedFolders.Num(); f++ )
		{
			TArray<FAssetData> Assets;
			bool HadAllEngineDataPlugins = SelectedFolders[ f ].RemoveFromStart( TEXT( "/All/EngineData/Plugins" ) );
			bool HadAll = SelectedFolders[ f ].RemoveFromStart( TEXT( "/All" ) );
			bool HadPlugins = SelectedFolders[ f ].RemoveFromStart( TEXT( "/Plugins" ) );
			AssetRegistryModule.Get().GetAssetsByPath( FName( SelectedFolders[ f ] ), Assets, true );
			for( int i = 0; i < Assets.Num(); i++ )
				AllAssets.Add( Assets[ i ] );
		}
	}

	return AllAssets;
}
int GetNumSelectedAssets()
{
	TArray<FAssetData> AllAssets = GetAllSelectedAssets();

	return AllAssets.Num();
}
void IterateOverSelection( std::function<void( FAssetData& )> lambda )
{
	TArray<FAssetData> AllAssets = GetAllSelectedAssets();
	
	int current = 0;
	for( FAssetData& Asset : AllAssets )
	{
		lambda( Asset );
		current++;
	}
}
void IterateOverAllAssetsFromPath( FName PackagePath, std::function<void( FAssetData& )> lambda )
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>( "AssetRegistry" );

	TArray<FAssetData> Assets;
	AssetRegistryModule.Get().GetAssetsByPath( PackagePath, Assets, true );

	int current = 0;
	int Offset = 0;
	for( FAssetData& Asset : Assets )
	{
		if( Offset <= current )
		{
			//if( Asset.PackageName.ToString().Contains( "/Game/Developers/" ) ||
			//	!Asset.PackageName.ToString().StartsWith( "/Game" ) )
			//	continue;

			lambda( Asset );
		}
		current++;
	}
}

uint8 GetMemory( void* Pointer, int Offset )
{
	uint8 Value;
	memcpy( &Value, (uint8*)Pointer + Offset, sizeof(uint8) );
	return Value;
}
void GetMemory( void* SourceObject, int Offset, void *Destination, int Size )
{
	memcpy( Destination, (uint8*)SourceObject + Offset, Size );
}
class FArchiveFileReaderGeneric2 : public FArchiveFileReaderGeneric
{
public:
	TArray64<uint8>& GetBufferArray()
	{
		return BufferArray;
	}
};
class FArchiveFileWriterGeneric2 : public FArchiveFileWriterGeneric
{
public:
	TArray64<uint8>& GetBufferArray()
	{
		return BufferArray;
	}
};
bool ChangeConsoleVariable( const TCHAR* Name, int NewValue )
{
	IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable( Name );
	if( CVar )
	{
		IConsoleVariable* Var = CVar->AsVariable();
		if( Var )
		{
			Var->Set( NewValue );
			return true;
		}
	}

	return false;
}

const FGuid FParticleSystemCustomVersion_GUID( 0x4A56EB40, 0x10F511DC, 0x92D3347E, 0xB2C96AE7 );
const FGuid FDNAAssetCustomVersion_GUID( 0x9DE7BD98, 0x67D445B2, 0x8C0E9D73, 0xFDE1E367 );
const FGuid FInstancedStructCustomVersion_GUID( 0xE21E1CAA, 0xAF47425E, 0x89BF6AD4, 0x4C44A8BB );
const FGuid FHeightmapTextureEdgeSnapshotCustomVersion_GUID( 0x12345678, 0x12345678, 0x12345678, 0x12345678 );
const FGuid FDataHierarchyElementCustomVersion_Guid( 0xC1270362, 0xAB230A1F, 0xCB1EC736, 0x71275FAB );
const FGuid FWaterCustomVersion_GUID( 0x40D2FBA7, 0x4B484CE5, 0xB0385A75, 0x884E499E );
void SpoofCustomVersions( EEngineVersion TargetVersion )
{	
	const FGuid FShapeComponentCustomVersion_GUID( 0xB6E31B1C, 0xD29F11EC, 0x857E9F85, 0x6F9970E2 );
	const FGuid FWorldSettingCustomVersion_GUID( 0x1ED048F4, 0x2F2E4C68, 0x89D053A4, 0xF18F102D );
	const FGuid FSkeletalMeshCustomVersion_GUID( 0xD78A4A00, 0xE8584697, 0xBAA819B5, 0x487D46B4 );
	const FGuid FPropertyBagCustomVersion_GUID( 0x134A157E, 0xD5E249A3, 0x8D4E843C, 0x98FE9E31 );
	const FGuid FPCGCustomVersion_GUID( 0x2763920D, 0x0F784B39, 0x986E4BB3, 0xA88D666D );

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
	if (TargetVersion == EEngineVersion::EV_5_7)
	{
		SetCustomVersionsFor( TargetVersion );
		#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 7		

		ChangeConsoleVariable( TEXT( "downgrader.NavMeshVersion" ), NAVMESHVER_NAVLINK_JUMP_CONFIGS );
		ChangeConsoleVariable( TEXT( "downgrader.NavMeshVersionMinCompatible" ), NAVMESHVER_LWCOORDS_OPTIMIZATION );
		
		((FPackageFileVersion*)&GPackageFileUEVersion)->FileVersionUE4 = VER_UE4_CORRECT_LICENSEE_FLAG;
		((FPackageFileVersion*)&GPackageFileUEVersion)->FileVersionUE5 = (int)EUnrealEngineObjectUE5Version::IMPORT_TYPE_HIERARCHIES;
		#endif
	}
	if (TargetVersion == EEngineVersion::EV_5_6)
	{
		SetCustomVersionsFor( TargetVersion );
		#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 6		

		ChangeConsoleVariable( TEXT( "downgrader.NavMeshVersion" ), NAVMESHVER_TILE_RESOLUTIONS_AGENTMAXSTEPHEIGHT );
		ChangeConsoleVariable( TEXT( "downgrader.NavMeshVersionMinCompatible" ), NAVMESHVER_LWCOORDS_OPTIMIZATION );
		
		((FPackageFileVersion*)&GPackageFileUEVersion)->FileVersionUE4 = VER_UE4_CORRECT_LICENSEE_FLAG;
		((FPackageFileVersion*)&GPackageFileUEVersion)->FileVersionUE5 = (int)EUnrealEngineObjectUE5Version::OS_SUB_OBJECT_SHADOW_SERIALIZATION;
		#endif
	}
	if( TargetVersion == EEngineVersion::EV_5_5 )
	{
		SetCustomVersionsFor( TargetVersion );
	#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5
		ChangeConsoleVariable( TEXT( "downgrader.NavMeshVersion" ), NAVMESHVER_TILE_RESOLUTIONS_AGENTMAXSTEPHEIGHT );
		ChangeConsoleVariable( TEXT( "downgrader.NavMeshVersionMinCompatible" ), NAVMESHVER_LWCOORDS_OPTIMIZATION );

		( (FPackageFileVersion*)&GPackageFileUEVersion )->FileVersionUE4 = VER_UE4_CORRECT_LICENSEE_FLAG;
		( (FPackageFileVersion*)&GPackageFileUEVersion )->FileVersionUE5 = (int)EUnrealEngineObjectUE5Version::ASSETREGISTRY_PACKAGEBUILDDEPENDENCIES;
	#endif
	}
	if( TargetVersion == EEngineVersion::EV_5_4 )
	{
		SetCustomVersionsFor( TargetVersion );
	#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4		
		ChangeConsoleVariable( TEXT( "downgrader.NavMeshVersion" ), NAVMESHVER_TILE_RESOLUTIONS_AGENTMAXSTEPHEIGHT );
		ChangeConsoleVariable( TEXT( "downgrader.NavMeshVersionMinCompatible" ), NAVMESHVER_LWCOORDS_OPTIMIZATION );

		( (FPackageFileVersion*)&GPackageFileUEVersion )->FileVersionUE4 = VER_UE4_CORRECT_LICENSEE_FLAG;
		( (FPackageFileVersion*)&GPackageFileUEVersion )->FileVersionUE5 = (int)EUnrealEngineObjectUE5Version::PROPERTY_TAG_COMPLETE_TYPE_NAME;
	#endif
	}
	if( TargetVersion == EEngineVersion::EV_5_3 )
	{
		SetCustomVersionsFor( TargetVersion );
		#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3			
		ChangeConsoleVariable( TEXT( "downgrader.NavMeshVersion" ), NAVMESHVER_TILE_RESOLUTIONS_AGENTMAXSTEPHEIGHT );
		ChangeConsoleVariable( TEXT( "downgrader.NavMeshVersionMinCompatible" ), NAVMESHVER_LWCOORDS_OPTIMIZATION );

		( (FPackageFileVersion*)&GPackageFileUEVersion )->FileVersionUE4 = VER_UE4_CORRECT_LICENSEE_FLAG;
		( (FPackageFileVersion*)&GPackageFileUEVersion )->FileVersionUE5 = (int)EUnrealEngineObjectUE5Version::DATA_RESOURCES;
		#endif
	}
	
	if( TargetVersion == EEngineVersion::EV_5_2 )
	{
		SetCustomVersionsFor( TargetVersion );
	#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 2		
		ChangeConsoleVariable( TEXT( "downgrader.NavMeshVersion" ), NAVMESHVER_TILE_RESOLUTIONS_CELLHEIGHT );
		ChangeConsoleVariable( TEXT( "downgrader.NavMeshVersionMinCompatible" ), NAVMESHVER_LWCOORDS_OPTIMIZATION );

		( (FPackageFileVersion*)&GPackageFileUEVersion )->FileVersionUE4 = VER_UE4_CORRECT_LICENSEE_FLAG;
		( (FPackageFileVersion*)&GPackageFileUEVersion )->FileVersionUE5 = (int)EUnrealEngineObjectUE5Version::DATA_RESOURCES;
	#endif
	}	
	if( TargetVersion == EEngineVersion::EV_5_1 )
	{
		SetCustomVersionsFor( TargetVersion );
	#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1		
		ChangeConsoleVariable( TEXT( "downgrader.NavMeshVersion" ), NAVMESHVER_OPTIM_FIX_SERIALIZE_PARAMS );
		ChangeConsoleVariable( TEXT( "downgrader.NavMeshVersionMinCompatible" ), NAVMESHVER_LWCOORDS_OPTIMIZATION );

		( (FPackageFileVersion*)&GPackageFileUEVersion )->FileVersionUE4 = VER_UE4_CORRECT_LICENSEE_FLAG;
		( (FPackageFileVersion*)&GPackageFileUEVersion )->FileVersionUE5 = (int)EUnrealEngineObjectUE5Version::ADD_SOFTOBJECTPATH_LIST;
	#endif
	}
	//5.0
	if( TargetVersion == EEngineVersion::EV_5_0 )
	{
		SetCustomVersionsFor( TargetVersion );
	#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 0		
		ChangeConsoleVariable( TEXT( "downgrader.NavMeshVersion" ), NAVMESHVER_OPTIM_FIX_SERIALIZE_PARAMS );
		ChangeConsoleVariable( TEXT( "downgrader.NavMeshVersionMinCompatible" ), NAVMESHVER_LWCOORDS_OPTIMIZATION );

		( (FPackageFileVersion*)&GPackageFileUEVersion )->FileVersionUE4 = VER_UE4_CORRECT_LICENSEE_FLAG;
		( (FPackageFileVersion*)&GPackageFileUEVersion )->FileVersionUE5 = (int)EUnrealEngineObjectUE5Version::LARGE_WORLD_COORDINATES;
	#endif
	}

	if( TargetVersion == EEngineVersion::EV_4_27 )
	{
		ChangeCustomVersion( FRenderingObjectVersion::GUID, FRenderingObjectVersion::VirtualTexturedLightmapsV3, TEXT( "Dev-Rendering" ) );
		ChangeCustomVersion( FReleaseObjectVersion::GUID, FReleaseObjectVersion::LonglatTextureCubeDefaultMaxResolution, TEXT( "Release" ) );
		//ChangeCustomVersion( FEditorObjectVersion::GUID, FEditorObjectVersion::SkeletalMeshSourceDataSupport16bitOfMaterialNumber, TEXT( "Dev-Editor" ) );//4.27
		ChangeCustomVersion( FEditorObjectVersion::GUID, FEditorObjectVersion::ChangeSceneCaptureRootComponent, TEXT( "Dev-Editor" ) );//Required for 4.27 static meshes ?
		ChangeCustomVersion( FFrameworkObjectVersion::GUID, FFrameworkObjectVersion::StoringUCSSerializationIndex, TEXT( "Dev-Framework" ) );
		ChangeCustomVersion( FFortniteMainBranchObjectVersion::GUID, FFortniteMainBranchObjectVersion::RemoveLandscapeWaterInfo, TEXT( "FortniteMain" ) );
		ChangeCustomVersion( FFortniteReleaseBranchCustomObjectVersion::GUID, FFortniteReleaseBranchCustomObjectVersion::DisableLevelset_v14_10, TEXT( "FortniteRelease" ) );
		ChangeCustomVersion( FAnimPhysObjectVersion::GUID, FAnimPhysObjectVersion::GeometryCacheAssetDeprecation, TEXT( "Dev-AnimPhys" ) );
		ChangeCustomVersion( FPhysicsObjectVersion::GUID, FPhysicsObjectVersion::ChaosConvexHasUniqueEdgeSet, TEXT( "Dev-Physics" ) );
		ChangeCustomVersion( FExternalPhysicsCustomObjectVersion::GUID, FExternalPhysicsCustomObjectVersion::AddOneWayInteraction, TEXT( "Dev-Physics-Ext" ) );
		ChangeCustomVersion( FEnterpriseObjectVersion::GUID, FEnterpriseObjectVersion::CoreTechParametricSurfaceOptim, TEXT( "Dev-Enterprise" ) );
		ChangeCustomVersion( FWorldSettingCustomVersion_GUID, FWorldSettingCustomVersion::BeforeCustomVersionWasAdded, TEXT( "WorldSettingVer" ) );
		//ChangeCustomVersion( FSkeletalMeshCustomVersion::GUID, FSkeletalMeshCustomVersion::RemoveEnableClothLOD, TEXT( "SkeletalMeshVer" ) );
		ChangeCustomVersion( FNiagaraObjectVersion::GUID, FNiagaraObjectVersion::SkeletalMeshVertexSampling, TEXT( "Dev-Niagara" ) );
		ChangeCustomVersion( FControlRigObjectVersion::GUID, FControlRigObjectVersion::BlueprintVariableSupport, TEXT( "Dev-ControlRig" ) );
		ChangeCustomVersion( FNiagaraCustomVersion::GUID, FNiagaraCustomVersion::MoveDefaultValueFromFNiagaraVariableMetaDataToUNiagaraScriptVariable, TEXT( "NiagaraVer" ) );
		ChangeCustomVersion( FParticleSystemCustomVersion_GUID, FParticleSystemCustomVersion::FixLegacySpawningBugs, TEXT( "ParticleSystemVer" ) );

	#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 0
		ChangeCustomVersion( FUE5MainStreamObjectVersion::GUID, FUE5MainStreamObjectVersion::BeforeCustomVersionWasAdded, TEXT( "UE5-Main" ) );
		ChangeCustomVersion( FUE5ReleaseStreamObjectVersion::GUID, FUE5ReleaseStreamObjectVersion::BeforeCustomVersionWasAdded, TEXT( "UE5-Release" ) );
	#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 6
		ChangeCustomVersion( FUE5PrivateFrostyStreamObjectVersion::GUID, FUE5PrivateFrostyStreamObjectVersion::BeforeCustomVersionWasAdded, TEXT( "UE5-PrivateFrosty" ) );
	#else
		ChangeCustomVersion( FUE5SpecialProjectStreamObjectVersion::GUID, FUE5SpecialProjectStreamObjectVersion::BeforeCustomVersionWasAdded, TEXT( "UE5-SpecialProject" ) );
	#endif
		ChangeCustomVersion( FUE5LWCRenderingStreamObjectVersion::GUID, FUE5LWCRenderingStreamObjectVersion::BeforeCustomVersionWasAdded, TEXT( "UE5-Dev-LWCRendering" ) );
		ChangeCustomVersion( FIKRigObjectVersion::GUID, FIKRigObjectVersion::BeforeCustomVersionWasAdded, TEXT( "Dev-IKRig" ) );
	#endif
	#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
		ChangeCustomVersion( FFortniteSeasonBranchObjectVersion::GUID, FFortniteSeasonBranchObjectVersion::BeforeCustomVersionWasAdded, TEXT( "FortniteSeason" ) );
		ChangeCustomVersion( FShapeComponentCustomVersion_GUID, 0, TEXT( "ShapeComponentVer" ) );
		ChangeCustomVersion( FRigVMObjectVersion::GUID, -1, TEXT( "Dev-RigVM" ) );
		ChangeCustomVersion( FInterchangeCustomVersion::GUID, FInterchangeCustomVersion::BeforeCustomVersionWasAdded, TEXT( "InterchangeAssetImportDataVer" ) );
		ChangeCustomVersion( FPCGCustomVersion_GUID, -1, TEXT( "PCGBaseVer" ) );
		ChangeCustomVersion( FFortniteValkyrieBranchObjectVersion::GUID, -1, TEXT( "FortniteValkyrie" ) );
		ChangeCustomVersion( FPropertyBagCustomVersion_GUID, /*BeforeCustomVersionWasAdded*/0, TEXT( "PropertyBagCustomVersion" ) );
		ChangeCustomVersion( FHeightmapTextureEdgeSnapshotCustomVersion_GUID, /*BeforeCustomVersionWasAdded*/0, TEXT( "FHeightmapTextureEdgeSnapshotCustomVersion" ) );
		ChangeCustomVersion( FInstancedStructCustomVersion_GUID, -1, TEXT( "InstancedStructCustomVersion" ) );
		ChangeCustomVersion( FDataHierarchyElementCustomVersion_Guid, -1, TEXT( "DataHierarchyElementVersion" ) );
		if (IsPluginEnabled( TEXT( "Water" ) ))
			ChangeCustomVersion( FWaterCustomVersion_GUID, -1, TEXT( "Water" ) );
	#endif

		ChangeConsoleVariable( TEXT( "downgrader.NavMeshVersion" ), NAVMESHVER_LANDSCAPE_HEIGHT );
		ChangeConsoleVariable( TEXT( "downgrader.NavMeshVersionMinCompatible" ), NAVMESHVER_LANDSCAPE_HEIGHT );

		#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 0
		( (FPackageFileVersion*)&GPackageFileUEVersion )->FileVersionUE4 = VER_UE4_CORRECT_LICENSEE_FLAG;
		( (FPackageFileVersion*)&GPackageFileUEVersion )->FileVersionUE5 = (int)EUnrealEngineObjectUE5Version::INITIAL_VERSION;
		#endif
	}
	if( TargetVersion == EEngineVersion::EV_4_26 )
	{
		ChangeCustomVersion( FRenderingObjectVersion::GUID, FRenderingObjectVersion::VolumeExtinctionBecomesRGB, TEXT( "Dev-Rendering" ) );
		ChangeCustomVersion( FReleaseObjectVersion::GUID, FReleaseObjectVersion::StructureDataAddedToConvex, TEXT( "Release" ) );
		//ChangeCustomVersion( FEditorObjectVersion::GUID, FEditorObjectVersion::SkeletalMeshSourceDataSupport16bitOfMaterialNumber, TEXT( "Dev-Editor" ) );//4.27
		ChangeCustomVersion( FEditorObjectVersion::GUID, FEditorObjectVersion::ChangeSceneCaptureRootComponent, TEXT( "Dev-Editor" ) );//Required for 4.27 static meshes ?
		ChangeCustomVersion( FFrameworkObjectVersion::GUID, FFrameworkObjectVersion::StoringUCSSerializationIndex, TEXT( "Dev-Framework" ) );
		ChangeCustomVersion( FFortniteMainBranchObjectVersion::GUID, FFortniteMainBranchObjectVersion::ChaosSolverPropertiesMoved, TEXT( "FortniteMain" ) );
		ChangeCustomVersion( FFortniteReleaseBranchCustomObjectVersion::GUID, FFortniteReleaseBranchCustomObjectVersion::DisableLevelset_v14_10, TEXT( "FortniteRelease" ) );
		ChangeCustomVersion( FAnimPhysObjectVersion::GUID, FAnimPhysObjectVersion::GeometryCacheAssetDeprecation, TEXT( "Dev-AnimPhys" ) );		
		ChangeCustomVersion( FPhysicsObjectVersion::GUID, FPhysicsObjectVersion::GroomWithImportSettings, TEXT( "Dev-Physics" ) );
		ChangeCustomVersion( FExternalPhysicsCustomObjectVersion::GUID, FExternalPhysicsCustomObjectVersion::RemovedAABBTreeFullBounds, TEXT( "Dev-Physics-Ext" ) );
		ChangeCustomVersion( FEnterpriseObjectVersion::GUID, FEnterpriseObjectVersion::CoreTechParametricSurfaceOptim, TEXT( "Dev-Enterprise" ) );
		//ChangeCustomVersion( FSkeletalMeshCustomVersion::GUID, FSkeletalMeshCustomVersion::RemoveEnableClothLOD, TEXT( "SkeletalMeshVer" ) );
		ChangeCustomVersion( FNiagaraObjectVersion::GUID, FNiagaraObjectVersion::SkeletalMeshVertexSampling, TEXT( "Dev-Niagara" ) );
		ChangeCustomVersion( FControlRigObjectVersion::GUID, FControlRigObjectVersion::BlueprintVariableSupport, TEXT( "Dev-ControlRig" ) );
		ChangeCustomVersion( FNiagaraCustomVersion::GUID, FNiagaraCustomVersion::SignificanceHandlers, TEXT( "NiagaraVer" ) );
		ChangeCustomVersion( FParticleSystemCustomVersion_GUID, FParticleSystemCustomVersion::FixLegacySpawningBugs, TEXT( "ParticleSystemVer" ) );

	#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 0
		ChangeCustomVersion( FUE5MainStreamObjectVersion::GUID, FUE5MainStreamObjectVersion::BeforeCustomVersionWasAdded, TEXT( "UE5-Main" ) );
		ChangeCustomVersion( FUE5ReleaseStreamObjectVersion::GUID, FUE5ReleaseStreamObjectVersion::BeforeCustomVersionWasAdded, TEXT( "UE5-Release" ) );
	#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 6
		ChangeCustomVersion( FUE5PrivateFrostyStreamObjectVersion::GUID, FUE5PrivateFrostyStreamObjectVersion::BeforeCustomVersionWasAdded, TEXT( "UE5-PrivateFrosty" ) );
	#else
		ChangeCustomVersion( FUE5SpecialProjectStreamObjectVersion::GUID, FUE5SpecialProjectStreamObjectVersion::BeforeCustomVersionWasAdded, TEXT( "UE5-SpecialProject" ) );
	#endif
		ChangeCustomVersion( FUE5LWCRenderingStreamObjectVersion::GUID, FUE5LWCRenderingStreamObjectVersion::BeforeCustomVersionWasAdded, TEXT( "UE5-Dev-LWCRendering" ) );
		ChangeCustomVersion( FIKRigObjectVersion::GUID, FIKRigObjectVersion::BeforeCustomVersionWasAdded, TEXT( "Dev-IKRig" ) );
	#endif
	#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
		ChangeCustomVersion( FFortniteSeasonBranchObjectVersion::GUID, FFortniteSeasonBranchObjectVersion::BeforeCustomVersionWasAdded, TEXT( "FortniteSeason" ) );
		ChangeCustomVersion( FShapeComponentCustomVersion_GUID, 0, TEXT( "ShapeComponentVer" ) );
		ChangeCustomVersion( FRigVMObjectVersion::GUID, -1, TEXT( "Dev-RigVM" ) );
		ChangeCustomVersion( FInterchangeCustomVersion::GUID, FInterchangeCustomVersion::BeforeCustomVersionWasAdded, TEXT( "InterchangeAssetImportDataVer" ) );
		ChangeCustomVersion( FPCGCustomVersion_GUID, -1, TEXT( "PCGBaseVer" ) );
		ChangeCustomVersion( FFortniteValkyrieBranchObjectVersion::GUID, -1, TEXT( "FortniteValkyrie" ) );
		ChangeCustomVersion( FPropertyBagCustomVersion_GUID, /*BeforeCustomVersionWasAdded*/0, TEXT( "PropertyBagCustomVersion" ) );
		ChangeCustomVersion( FHeightmapTextureEdgeSnapshotCustomVersion_GUID, /*BeforeCustomVersionWasAdded*/0, TEXT( "FHeightmapTextureEdgeSnapshotCustomVersion" ) );
		ChangeCustomVersion( FInstancedStructCustomVersion_GUID, -1, TEXT( "InstancedStructCustomVersion" ) );
		ChangeCustomVersion( FDataHierarchyElementCustomVersion_Guid, -1, TEXT( "DataHierarchyElementVersion" ) );
		if ( IsPluginEnabled(TEXT("Water")) )
			ChangeCustomVersion( FWaterCustomVersion_GUID, -1, TEXT( "Water" ) );
	#endif
		ChangeCustomVersion( FWorldSettingCustomVersion_GUID, FWorldSettingCustomVersion::BeforeCustomVersionWasAdded, TEXT( "WorldSettingVer" ) );
		
		ChangeConsoleVariable( TEXT( "downgrader.NavMeshVersion" ), NAVMESHVER_LANDSCAPE_HEIGHT );
		ChangeConsoleVariable( TEXT( "downgrader.NavMeshVersionMinCompatible" ), NAVMESHVER_LANDSCAPE_HEIGHT );

	#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 0
		( (FPackageFileVersion*)&GPackageFileUEVersion )->FileVersionUE4 = VER_UE4_CORRECT_LICENSEE_FLAG;
		( (FPackageFileVersion*)&GPackageFileUEVersion )->FileVersionUE5 = (int)EUnrealEngineObjectUE5Version::INITIAL_VERSION;
	#endif
	}
	SpoofEngineVersion( TargetVersion );
#endif
}
bool SavePackage(UPackage* Package, FString PackageName, FString Extension )
{
	FString FilePath;
	FilePath = FPackageName::LongPackageNameToFilename( PackageName, Extension );

	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Standalone;
	SaveArgs.Error = nullptr;
	SaveArgs.bWarnOfLongFilename = true;
	SaveArgs.SaveFlags = SAVE_None;
	if( !Package->IsFullyLoaded() )
	{
		Package->FullyLoad();
		if( !Package->IsFullyLoaded() )
			return false;
	}
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 0
	FAssetCompilingManager& Manager = FAssetCompilingManager::Get();
#endif
	bool bWasSuccessful = false;
	#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION <= 27
		bWasSuccessful = GEditor->SavePackage( Package, nullptr, SaveArgs.TopLevelFlags, *FilePath );
	#endif
	#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 0
		bWasSuccessful = GEditor->SavePackage( Package, nullptr, *FilePath, SaveArgs );
	#endif
	return bWasSuccessful;
}
class PackageModification
{
public:
	PackageModification( int pOffset, int pSize )
	{
		Offset = pOffset;
		Size = pSize;
	}
	int Offset;
	int Size;
};
class StructAndPropName
{
public:
	FString StructName;
	FString PropName;

	// Constructor
	StructAndPropName( const FString& InStructName, const FString& InPropName )
		: StructName( InStructName ), PropName( InPropName )
	{
	}

	// Default constructor needed by TMap
	StructAndPropName() = default;

	// Equality operator
	bool operator==( const StructAndPropName& Other ) const
	{
		return StructName == Other.StructName && PropName == Other.PropName;
	}
};
// Hash function for TMap key
FORCEINLINE uint32 GetTypeHash( const StructAndPropName& Key )
{
	return HashCombine( GetTypeHash( Key.StructName ), GetTypeHash( Key.PropName ) );
}

TMap< StructAndPropName, FProperty*> CachedProperties;
FProperty* GetProperty( FString StructName, FString PropName )
{
	StructAndPropName Key;
	Key.StructName = StructName;
	Key.PropName = PropName;
	FProperty** FoundProperty = CachedProperties.Find( Key );
	if (!FoundProperty )
	{
		for( TObjectIterator<UStruct> StructIt; StructIt; ++StructIt )
		{
			UStruct* structure = *StructIt;
			if( structure->GetName().Compare( StructName ) == 0 )
			{
				FProperty* Property = structure->PropertyLink;
				while( Property )
				{
					if( Property->GetName().Compare( PropName ) == 0 )
					{
						CachedProperties.Add( Key, Property );
						return Property;
					}
					Property = Property->PropertyLinkNext;
				}
				return nullptr;
			}
		}

		return nullptr;
	}
	else
	{
		FProperty* Ret = *FoundProperty;
		return Ret;
	}
}
UFunction* GetFunction( FString FuncName )
{
	for( TObjectIterator<UFunction> StructIt; StructIt; ++StructIt )
	{
		UFunction* Func = *StructIt;
		if( Func->GetName().Compare( FuncName ) == 0 )
		{
			return Func;
		}
	}

	return nullptr;
}
UStruct* GetStruct( FString StructName )
{
	for( TObjectIterator<UStruct> StructIt; StructIt; ++StructIt )
	{
		UStruct* structure = *StructIt;
		if( structure->GetName().Compare( StructName ) == 0 )
		{
			return structure;
		}
	}

	return nullptr;
}
void RemoveFunctionFlag( FString FuncName, EFunctionFlags RemoveFlag )
{
	UFunction* Func = GetFunction( FuncName );

	Func->FunctionFlags &= ~RemoveFlag;
}
void AddPropertyFlag( FString StructName, FString PropName, EPropertyFlags AddFlag )
{
	FProperty* Property = GetProperty( StructName, PropName );
			
	EPropertyFlags Flags = Property->GetPropertyFlags();
	Property->SetPropertyFlags( AddFlag );
}
void RemovePropertyFlag( FString StructName, FString PropName, EPropertyFlags RemoveFlag )
{
	FProperty* Property = GetProperty( StructName, PropName );
	
	EPropertyFlags Flags = Property->GetPropertyFlags();
	Property->ClearPropertyFlags( Flags );
	Flags &= ~RemoveFlag;// CPF_Deprecated;
	Property->SetPropertyFlags( Flags );
}
void RemovePropertyFlagOnAllProperties( EPropertyFlags RemoveFlag )
{
	int NumProperties = 0;
	int NumStructs = 0;
	for( TObjectIterator<UStruct> StructIt; StructIt; ++StructIt )
	{
		UStruct* structure = *StructIt;
				
		FProperty* Property = structure->PropertyLink;
		while( Property )
		{
			EPropertyFlags Flags = Property->GetPropertyFlags();
			Property->ClearPropertyFlags( Flags );
			Flags &= ~RemoveFlag;// CPF_Deprecated;
			Property->SetPropertyFlags( Flags );
			NumProperties++;

			Property = Property->PropertyLinkNext;
		}

		NumStructs++;
	}
}
int GetMemberOffset( FString StructName, FString PropName, int Index = 0 )
{
	FProperty* Property = GetProperty( StructName, PropName );
	if( Property )
	{
		int Offset = Property->GetOffset_ForDebug();
		if( Index > 0 )
		{
			if ( Index >= Property->ArrayDim  )
				ensureMsgf( false, TEXT( "GetMemberOffset failed ! with %s->%s[%d]" ), *StructName, *PropName, Index );

			Offset += Index * Property->ElementSize;
		}
		return Offset;
	}
	return -1;
}
class AssetForModification
{
public:
	//FAssetData Asset;
	FName PackageName;
	UObject* AssetObject = nullptr;
	FString Extension;
	int PackageSummarySize = 0;
	int PackageCustomVersions = 0;
	int PackageNameLength = 0;

	TArray64<uint8> PackageFileData;
	FString FilePath;

	UPackage* Package = nullptr;
	bool Downgrade( );
};
int PackageFileSummaryOffset = 168;
int LoaderOffset = 160;
bool DoUnloadPerAsset = true;
bool WriteAssetAfterUnload = false;
bool DoUnloadAll = false;
bool DoFullDowngradingPerAsset = true;

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
FPackageFileSummary* GetPackageSummary( FPackageReader& PackageReader )
{
	FPackageFileSummary* PackageFileSummary = new FPackageFileSummary;
	GetMemory( &PackageReader, PackageFileSummaryOffset, PackageFileSummary, sizeof( FPackageFileSummary ) );
	return PackageFileSummary;
}
#endif
bool AssetForModification::Downgrade( )
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
	EEngineVersion CurrentVersion = GetCurrentVersion();
	TArray<PackageModification> Modifications;

	if( DowngraderSettings->TargetVersion <= EEngineVersion::EV_4_27 )
	{
		//Deletes the UE5Version
		Modifications.Add( PackageModification( 16, 4 ) );
	}
	if( CurrentVersion == EEngineVersion::EV_5_3 )//it's == Because 5.4+ dropped the IsLoading() calls
	{
		//ADD_SOFTOBJECTPATH_LIST
		Modifications.Add( PackageModification( 48, 8 ) );
		//DATA_RESOURCES
		Modifications.Add( PackageModification( 265, 4 ) );
	}

	FEngineVersion EngineVer = FEngineVersion::Current();
	FString Branch = EngineVer.GetBranchDescriptor();

	if( !AssetObject )
		return false;
	Package = AssetObject->GetPackage();

	if( !SavePackage( Package, PackageName.ToString(), Extension ) )
	{
		return false;
	}
	//Read written header to get custom versions and summary size->
	{
		FPackageReader PackageReader;
		FPackageReader::EOpenPackageResult OpenPackageResult;
		auto ReadSuccess = PackageReader.OpenPackageFile( PackageName.ToString(), FilePath, &OpenPackageResult );
		if (ReadSuccess)
		{
			FPackageFileSummary* PackageFileSummary = GetPackageSummary( PackageReader );
			PackageSummarySize = PackageFileSummary->NameOffset;
			PackageCustomVersions = PackageFileSummary->GetCustomVersionContainer().GetAllVersions().Num();
		}
	}

	for (int i = 0; i < Modifications.Num(); i++)
	{
		PackageModification& Modification = Modifications[i];
		int OriginalOffset = Modification.Offset;
		if (OriginalOffset > 20)
		{
			Modification.Offset += PackageCustomVersions * 20 + PackageNameLength;
		}
		if (OriginalOffset > 200)
		{
			Modification.Offset += (Branch.Len() + 1) * 2;
		}
	}

	FFileHelper::LoadFileToArray( PackageFileData, *FilePath );
	
	if( PackageFileData.Num() == 0 )
		return false;

	uint8* Data = PackageFileData.GetData();
//#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4
	//int NameOffsetInSpoofedPackage = 44 + PackageCustomVersions * 20 + PackageNameLength;
	//int NameOffsetValue = *(int32*)( Data + NameOffsetInSpoofedPackage );
	//if( PackageSummarySize > NameOffsetValue )
	//{
	//	PackageSummarySize = NameOffsetValue;
	//}
//#endif
	int CustomVersionsOffset = 24;
	if (CurrentVersion >= EEngineVersion::EV_5_6 && DowngraderSettings->TargetVersion >= EEngineVersion::EV_5_6)
	{
		CustomVersionsOffset = 48;//+PACKAGE_SAVED_HASH
	}
	int NumCustomVersions = *(int32*)( Data + CustomVersionsOffset );
	for( int i = 0; i < NumCustomVersions; i++ )
	{
		FCustomVersion* CustomVersion = (FCustomVersion*)( Data + CustomVersionsOffset + 4 + i * 20 );
		
		if (CustomVersion->Version <= 0)
		{
			bool ReplaceWithSafeCustomVersion = true;
			//DNAAssetCustomVersion is at version=0 since 4.27 at least
			if ( CustomVersion->Key == FDNAAssetCustomVersion_GUID )
				ReplaceWithSafeCustomVersion = false;
			//First appeared in 5.3 and still at version=0
			if ( CustomVersion->Key == FInstancedStructCustomVersion_GUID && DowngraderSettings->TargetVersion >= EEngineVersion::EV_5_3 )
				ReplaceWithSafeCustomVersion = false;
			if (ReplaceWithSafeCustomVersion)
			{
				//Assign a harmless GUID at latest version for 4.26->5.0
				CustomVersion->Key = FParticleSystemCustomVersion_GUID;
				CustomVersion->Version = FParticleSystemCustomVersion::FixLegacySpawningBugs;
			}
		}
	}

	int32* LegacyFileVersion = (int32*)(Data + 4);
	if ( CurrentVersion >= EEngineVersion::EV_5_6 && 
		 DowngraderSettings->TargetVersion < EEngineVersion::EV_5_6 && DowngraderSettings->TargetVersion > EEngineVersion::EV_4_27 )
	{
		*LegacyFileVersion = -8;
	}
	else if (DowngraderSettings->TargetVersion <= EEngineVersion::EV_4_27)
	{
		*LegacyFileVersion = -7;
	}

	int Deleted = 0;
	//File cuts
	for( int i = 0; i < Modifications.Num(); i++ )
	{
		PackageModification Modification = Modifications[ i ];
		uint8* DataCopy = new uint8[ PackageSummarySize ];
		memcpy( DataCopy, Data, PackageSummarySize );

		int ActualOffset = Modification.Offset - Deleted;
		int CopySize = PackageSummarySize - ( ActualOffset + Modification.Size );
		memcpy( Data + ActualOffset, DataCopy + ActualOffset + Modification.Size, CopySize );

		Deleted += Modification.Size;
		delete[] DataCopy;
	}
	bool Status = true;
	if( !WriteAssetAfterUnload )
	{
		do
		{
			uint32 WriteFlags = FILEWRITE_EvenIfReadOnly;
			Status = FFileHelper::SaveArrayToFile( PackageFileData, *FilePath, &IFileManager::Get(), WriteFlags );
			if( !Status )
			{
				UE_LOG( LogTemp, Warning, TEXT( "Saving %s -> Sleep for 1s Size = %lld" ), *FilePath, PackageFileData.Num() );
				FPlatformProcess::Sleep( 1 );
			}
		} while( !Status );
	}
	
	AssetObject->RemoveFromRoot();

	return Status;
#else
	return false;
#endif
}
template<class TYPE>
void AssignVariable( void* Destination, int MemberOffset, const TYPE& Source )
{
	*(TYPE*)( (uint8*)Destination + MemberOffset ) = Source;
}
template<class TYPE>
void AssignVariable( void* Destination, FString ClassName, FString MemberName, const TYPE& Source )
{
	int MemberOffset = GetMemberOffset( ClassName, MemberName );
	RemovePropertyFlag( ClassName, MemberName, CPF_Deprecated );
	if( MemberOffset == -1 )
	{
		ensureMsgf( false, TEXT( "AssignVariable failed ! with %s->%s" ), *ClassName, *MemberName );
	}
	*(TYPE*)( (uint8*)Destination + MemberOffset ) = Source;
}
template<class TYPE>
void AssignVariable( void* Destination, int DestMemberOffset, const void* Source, int SourceOffset )
{
	*(TYPE*)( (uint8*)Destination + DestMemberOffset ) = *(TYPE*)( (uint8*)Source + SourceOffset );
}
template<class TYPE>
TYPE& GetVariable( void* Pointer, int MemberOffset )
{
	TYPE* PointerToMember = (TYPE*)( (uint8*)Pointer + MemberOffset );
	return *PointerToMember;
}
template<class TYPE>
TYPE& GetVariable( void* Pointer, FString ClassName, FString MemberName )
{
	int MemberOffset = GetMemberOffset( ClassName, MemberName );
	RemovePropertyFlag( ClassName, MemberName, CPF_Deprecated );
	if( MemberOffset == -1 )
	{
		ensureMsgf( false, TEXT( "AssignVariable failed ! with %s->%s" ), *ClassName, *MemberName );
	}
	TYPE* PointerToMember = (TYPE*)( (uint8*)Pointer + MemberOffset );
	return *PointerToMember;
}
bool CompressTextureSourceWithPNG(UTexture* Texture,int32 Quality)
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
	if ( ! Texture->Source.IsValid() )
	{
		return false;
	}

	// we only support 1 layer currently
	if (Texture->Source.GetNumLayers() != 1)
	{
		return false;
	}
	
	if (Texture->Source.GetNumBlocks() != 1 )
	{
		// JPEG does not support UDIM/blocks ; fix me?
		return false;
	}
	
	if (Texture->Source.GetSourceCompression() == ETextureSourceCompressionFormat::TSCF_PNG)
	{
		// already TSCF_PNG
		return false;
	}
	
	if ( Texture->Source.GetNumSlices() != 1 )
	{
		// 1 mip, 1 slice only
		return false;
	}

	ETextureSourceFormat Format = Texture->Source.GetFormat(0);
	if ( Format != TSF_G8 && Format != TSF_BGRA8 )
	{
		// must be 8 bit
		return false;
	}

	// we do kill existing mips to match the behavior of the other conversions in here

	// okay, looks good, do it!

	FImage Image;
	if ( ! Texture->Source.GetMipImage(Image,0) )
	{
		UE_LOG(LogTexture,Error,TEXT("CompressTextureSourceWithPNG: Texture GetMipImage failed [%s]"),
			*Texture->GetFullName());
		return false;
	}

	// JPEG it :

	TArray64<uint8> PNGData;
	if ( ! FImageUtils::CompressImage( PNGData,TEXT(".png"),Image,Quality) )
	{
		UE_LOG(LogTexture,Error,TEXT("CompressTextureSourceWithPNG: Texture CompressImage failed [%s]"),
			*Texture->GetFullName());
		return false;
	}

	Texture->PreEditChange(nullptr);

	// Format stays BGRA8 or G8
	const int32 NumMips = 1;
	//Flip image because on load from 5.1+ it gets flipped back
	//if ( Format == TSF_BGRA8 )
		//FImageCore::TransposeImageRGBABGRA( Image );

	Texture->Source.InitWithCompressedSourceData(Image.SizeX,Image.SizeY,NumMips,Format,
		PNGData,
		ETextureSourceCompressionFormat::TSCF_PNG );

	Texture->PostEditChange();
#endif
	return true;
}
void FixTexture( UTexture* Texture, bool CompressWithPNG = true )
{
	if( !Texture )
		return;

	RemovePropertyFlag( TEXT( "TextureSource" ), TEXT( "bPNGCompressed" ), CPF_Deprecated );

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
	bool Modified = false;
	
	TEnumAsByte<enum ETextureSourceCompressionFormat>& Format = GetVariable< TEnumAsByte<enum ETextureSourceCompressionFormat> >( &Texture->Source, TEXT( "TextureSource" ), TEXT( "CompressionFormat" ) );

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5
	if (DowngraderSettings->TargetVersion < EEngineVersion::EV_5_5 )
	{
		//Convert to PNG
		if (Format == TSCF_UEJPEG ||
			Format == TSCF_UEDELTA)
		{
			Texture->Source.RemoveCompression();
			//This kills mipmaps !!!
			//if ( CompressWithPNG )
				//CompressTextureSourceWithPNG( Texture, 0 );
			Modified = true;
		}
	}
#endif

	//From 5.1 onwards there's this TransposeImageRGBABGRA in FTextureSource::TryDecompressData that flips channels
		//After 5.1 bPNGCompressed is DEPRECATED anyway
	if( Format == TSCF_PNG && DowngraderSettings->TargetVersion < EEngineVersion::EV_5_1 )
	{
	#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5
		//Remove PNG format because it will appear as BGRA in 4.27
		Texture->Source.RemoveCompression();
	#endif

		//bool& Value = GetVariable< bool >( &Texture->Source, TEXT( "TextureSource" ), TEXT( "bPNGCompressed" ) );
		//Value = 1;
		Modified = true;
	}
	if( !Texture->OodleTextureSdkVersion.IsNone() && Texture->OodleTextureSdkVersion.ToString().Len() > 0)
	{
		Texture->OodleTextureSdkVersion = FName();
		Modified = true;
	}

	if( DowngraderSettings->TargetVersion <= EEngineVersion::EV_4_27 )
	{
		if( Texture->Source.GetSizeX() != FMath::RoundUpToPowerOfTwo( Texture->Source.GetSizeX() ) ||
			Texture->Source.GetSizeY() != FMath::RoundUpToPowerOfTwo( Texture->Source.GetSizeY() ) )
		{
			ETextureSourceFormat SourceFormat = Texture->Source.GetFormat();
			//4.27 errors out if you don't do this
			//if( Texture->PowerOfTwoMode == ETexturePowerOfTwoSetting::None &&
			//	SourceFormat != TSF_BGRE8)//TC_Custom_HDRI_grey from DemonicVillage throws an assert if PadToPowerOfTwo
			//{
			//	Texture->PowerOfTwoMode = ETexturePowerOfTwoSetting::PadToPowerOfTwo;
			//	Modified = true;
			//}
			if ( Texture->MipGenSettings != TextureMipGenSettings::TMGS_NoMipmaps )
			{
				Texture->MipGenSettings = TextureMipGenSettings::TMGS_NoMipmaps;
				Modified = true;
			}
			if (Texture->CompressionSettings != TextureCompressionSettings::TC_EditorIcon)
			{
				Texture->CompressionSettings = TextureCompressionSettings::TC_EditorIcon;
				Modified = true;
			}
		}
	}
	if( DowngraderSettings->TargetVersion < EEngineVersion::EV_5_4 )//5.4 added 3 more modes
	{
		if( Texture->PowerOfTwoMode > ETexturePowerOfTwoSetting::PadToSquarePowerOfTwo )
			Texture->PowerOfTwoMode = ETexturePowerOfTwoSetting::PadToPowerOfTwo;
	}
	UVolumeTexture* VolumeTexture = Cast<UVolumeTexture>( Texture );
	if( VolumeTexture && DowngraderSettings->TargetVersion <= EEngineVersion::EV_5_0 )
	{
		//Temporary fix, feels like SliceIndex code is bugged in 5.0 IMO
		if( Texture->MipGenSettings != TMGS_Unfiltered )
		{
			Texture->MipGenSettings = TMGS_Unfiltered;
			Modified = true;
		}
	}

	if( Modified )
	{
		Texture->PostEditChange();
		Texture->MarkPackageDirty();
	}
#endif
}

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
const FNiagaraEmitterHandle* GetEmitterHandleForEmitter( UNiagaraSystem& System, const FVersionedNiagaraEmitter& Emitter )
{
	return System.GetEmitterHandles().FindByPredicate(
		[&Emitter]( const FNiagaraEmitterHandle& EmitterHandle ) { return EmitterHandle.GetInstance() == Emitter; } );
}
#endif
int GetModuleVersion( const FConfigSection* ModuleVersions, FString ModuleName )
{
	const FConfigValue* Value = ModuleVersions->Find( FName( ModuleName ) );
	if (!Value)
		return 0;
	FString SavedValue = Value->GetSavedValue();
	int32 Version = FCString::Atoi( *SavedValue );
	return Version;
}

#if (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 27) || ENGINE_MAJOR_VERSION == 5
FGuid GetVersionGUID( TArray<FVersionedNiagaraScriptData>& VersionData, int SelectedVersion )
{
	for (int i = 0; i < VersionData.Num(); i++)
	{
		FVersionedNiagaraScriptData& Data = VersionData[i];
		int DataVersion = Data.Version.MajorVersion * 10 + Data.Version.MinorVersion;
		if (DataVersion == SelectedVersion)
		{
			return Data.Version.VersionGuid;
		}
	}

	return FGuid();
}
int GetVersionUsed( TArray<FVersionedNiagaraScriptData>& VersionData, FGuid Selection )
{
	for (int i = 0; i < VersionData.Num(); i++)
	{
		FVersionedNiagaraScriptData& Data = VersionData[i];
		int VersionValue = Data.Version.MajorVersion * 10 + Data.Version.MinorVersion;
		if (Data.Version.VersionGuid == Selection)
		{
			return VersionValue;
		}
	}

	return 0;
}
#endif

struct FVersionedNiagaraEmitter;
class UNiagaraGraph;
class UNiagaraGraphNode;
void FixNiagaraDuplicatePins( UEdGraphNode* Node )
{
	TArray<UEdGraphPin*>& Pins = (TArray<UEdGraphPin*>&)Node->GetAllPins();
	for (int i = 0; i < Pins.Num(); i++)
	{
		UEdGraphPin* Pin = (UEdGraphPin*)Pins[i];
		//This prevents reallocated pins in modules like Drag to duplicate and throw an error
			//See NiagaraNode.cpp line 291 in 5_0
		if (Pin->AutogeneratedDefaultValue.Compare( Pin->DefaultValue ) != 0)
			Pin->AutogeneratedDefaultValue = Pin->DefaultValue;
	}
}
void InvalidatePinPersistentIDs( UEdGraphNode* Node )
{
	TArray<UEdGraphPin*>& Pins = (TArray<UEdGraphPin*>&)Node->GetAllPins();
	for (int i = 0; i < Pins.Num(); i++)
	{
		UEdGraphPin* Pin = (UEdGraphPin*)Pins[i];
		//This prevents variables from InitializeParticle to not be recognized in 4.27
		Pin->PersistentGuid.Invalidate();
	}
}
void FixNiagaraGraph( UNiagaraGraph* Graph, FVersionedNiagaraEmitter* VersionedEmitter )
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
	if( !Graph )
		return;

	const FConfigSection* ModuleVersions = GetNiagaraVersionsFor( DowngraderSettings->TargetVersion );

	UNiagaraEmitter* Emitter = nullptr;
	UNiagaraSystem* OwnerSystem = nullptr;
	if( VersionedEmitter )
	{
		Emitter = VersionedEmitter->Emitter;
		OwnerSystem = Emitter->GetTypedOuter<UNiagaraSystem>();
	}
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 6
	if( DowngraderSettings->TargetVersion < EEngineVersion::EV_5_5 )
	{
		UHierarchyRoot* HierarchyRoot = Graph->GetScriptParameterHierarchyRoot();
		if ( HierarchyRoot )
		{
			//Must do this, otherwise it will crash in UNiagaraGraph::MigrateParameterScriptDataToHierarchyRoot because the serialized Parameters have Variable = nullptr
			TArray<TObjectPtr<UHierarchyElement>>& MutableChildren = HierarchyRoot->GetChildrenMutable();
			MutableChildren.Empty();
		}
	}
#endif
	bool RemovedSomething = false;
	do
	{
		RemovedSomething = false;
		for( int n = 0; n < Graph->Nodes.Num(); n++ )
		{
			UEdGraphNode* Node = Graph->Nodes[ n ];
			FString NodeClassName = Node->GetClass()->GetName();
			if (NodeClassName.Contains( TEXT( "NiagaraNodeStaticSwitch" ) ))
			{
				NodeClassName = NodeClassName;
			}			
			UNiagaraNodeFunctionCall* FunctionCallNode = Cast< UNiagaraNodeFunctionCall>( Node );
			if( FunctionCallNode )
			{
				UNiagaraScript* FunctionScript = FunctionCallNode->FunctionScript.Get();
				if (FunctionScript)
				{
					if (FunctionScript->GetName().Compare( TEXT( "SolveForcesAndVelocity" ) ) == 0)
					{
						FunctionScript = FunctionScript;
					}
					
					if (DowngraderSettings->TargetVersion <= EEngineVersion::EV_4_27)
					{
						if (FunctionScript->GetName().Compare( TEXT( "InitializeParticle" ) ) == 0 )
						{
							InvalidatePinPersistentIDs( Node );
						}
					}
					if (DowngraderSettings->TargetVersion <= EEngineVersion::EV_5_0)
					{
						if (FunctionScript->GetName().Compare( TEXT( "Drag" ) ) == 0 ||
							FunctionScript->GetName().Compare( TEXT( "SolveForcesAndVelocity" ) ) == 0)
						{
							FixNiagaraDuplicatePins( Node );
						}
					}
					TArray<FVersionedNiagaraScriptData>& VersionData = GetVariable< TArray<FVersionedNiagaraScriptData> >( FunctionScript, TEXT("NiagaraScript"), TEXT("VersionData"));
					if (VersionData.Num() > 1 && FunctionCallNode->SelectedScriptVersion.IsValid())
					{
						int VersionUsed = GetVersionUsed( VersionData, FunctionCallNode->SelectedScriptVersion );

						int MaxModuleVersion = GetModuleVersion( ModuleVersions, FunctionScript->GetName() );
						if (VersionUsed > MaxModuleVersion)
						{
							FGuid VersionGUID = GetVersionGUID( VersionData, MaxModuleVersion );
							FunctionCallNode->SelectedScriptVersion = VersionGUID;
						}
					}
				}
				UNiagaraScriptSource* ScriptSource = FunctionCallNode->GetFunctionScriptSource();
				if( ScriptSource )
				{
					if( FunctionCallNode->GetName().Contains( TEXT( "Ripple_Position" ) ) )
					{
						FunctionCallNode = FunctionCallNode;
					}
					if( FunctionCallNode->FunctionScript->GetName().Contains( TEXT( "Ripple_Position" ) ) )
					{
						FunctionCallNode = FunctionCallNode;
					}
					
					UNiagaraGraph* FunctionGraph = CastChecked<UNiagaraGraph>( ScriptSource->NodeGraph );
					FixNiagaraGraph( FunctionGraph, nullptr );
					if( DowngraderSettings->TargetVersion <= EEngineVersion::EV_4_27 )
					{
						UPackage* FunctionPackage = ScriptSource->GetPackage();
						if( ScriptSource->GetName().Compare( TEXT( "ShapeLocation" ) ) == 0 &&
							FunctionPackage && FunctionPackage->GetName().StartsWith( TEXT( "/Niagara" ) ) )
						{
							TArray<TWeakObjectPtr<UNiagaraNodeInput>> RemovedNodes;
							if( VersionedEmitter && OwnerSystem )
							{
								const FNiagaraEmitterHandle* EmitterHandleId = GetEmitterHandleForEmitter( *OwnerSystem, *VersionedEmitter );
								if( EmitterHandleId )
								{
								#if 0 //def DOWNGRADER_CUSTOM_ENGINE
									FNiagaraStackGraphUtilities::RemoveModuleFromStack( *OwnerSystem, EmitterHandleId->GetId(), *FunctionCallNode, RemovedNodes );
									RemovedSomething = true;
									//Destroy it by detaching it from the package
									FunctionCallNode->Rename( nullptr, GetTransientPackage() );
									break;
								#endif
								}
							}
						}
					}
				}				
			}
			UNiagaraNodeInput* NodeInput = Cast< UNiagaraNodeInput>( Node );
			if (NodeInput)
			{
				FixNiagaraVariable( &NodeInput->Input );
				if (DowngraderSettings->TargetVersion <= EEngineVersion::EV_4_27)
				{
					//Not sure it's needed
					//NodeInput->ExposureOptions.bExposed = false;
				}
			}
			UNiagaraNodeOutput * NodeOutput = Cast< UNiagaraNodeOutput>( Node );
			if( NodeOutput )
			{
				for( int i = 0; i < NodeOutput->Outputs.Num(); i++ )
				{
					FixNiagaraVariable( &NodeOutput->Outputs[ i ] );
				}
			}
			if( DowngraderSettings->TargetVersion <= EEngineVersion::EV_4_27 )
			{
				//UNiagaraNodeParameterMapForIndex* MapForIndex = Cast<UNiagaraNodeParameterMapForIndex>( Node );
				if( NodeClassName.Contains( TEXT( "NiagaraNodeParameterMapForIndex" ) ))//Exists from 5_0
				{
					RemoveGraphNode( Node );
					RemovedSomething = true;
					break;
				}
			}
			TArray<UEdGraphPin*>& Pins = ( TArray<UEdGraphPin*>& )Node->GetAllPins();
			for( int i = 0; i < Pins.Num(); i++ )
			{
				UEdGraphPin* Pin = (UEdGraphPin*)Pins[ i ];
				if( DowngraderSettings->TargetVersion <= EEngineVersion::EV_4_27 )
				{
					if( Pin->GetName().Contains( TEXT( "Orientation Method" ) ) && FunctionCallNode &&
						FunctionCallNode->FunctionScript->GetName().Contains(TEXT("UpdateMeshOrientation")))
					{
						Pins.RemoveAt( i );
						i--;
						continue;
					}
					//if( Pin->PinType.PinCategory == UEdGraphSchema_Niagara::PinCategoryType )
					FixGraphPinType( Pin->PinType );

					//Prevent pins from not being recognized if this is non-zero. This is true in at least 4.27
					//Pin->PersistentGuid.Invalidate();
					if ( Pin->AutogeneratedDefaultValue.Len() > 0 )
						Pin->AutogeneratedDefaultValue.Empty();//prevent default values from putting null into DefaultValue and looking invalid in 4.27
				}
			}
		}
	} while( RemovedSomething );
	for( int s = 0; s < Graph->SubGraphs.Num(); s++ )
	{
		UNiagaraGraph* Subgraph = Cast<UNiagaraGraph>( Graph->SubGraphs[ s ] );
		FixNiagaraGraph( Subgraph, VersionedEmitter );
	}
#endif
}
FNiagaraTypeDefinition Vector2For427;
FNiagaraTypeDefinition Vector3For427;
FNiagaraTypeDefinition Vector4For427;
FNiagaraTypeDefinition FloatFor427;
bool InitializedNiagaraTypesFor427 = false;
void FixNiagaraVariable( FNiagaraVariableBase* Variable )
{
	if( !InitializedNiagaraTypesFor427 )
	{
		const auto& RegisteredTypes = FNiagaraTypeRegistry::GetRegisteredTypes();
		for( int i = 0; i < RegisteredTypes.Num(); i++ )
		{
			if( RegisteredTypes[ i ].GetName().Compare( TEXT( "Vector2D" ) ) == 0 )
			{
				Vector2For427 = RegisteredTypes[ i ];
			}
			if( RegisteredTypes[ i ].GetName().Compare( TEXT( "Vector" ) ) == 0 )
			{
				Vector3For427 = RegisteredTypes[ i ];
			}
			if( RegisteredTypes[ i ].GetName().Compare( TEXT( "Vector4" ) ) == 0 )
			{
				Vector4For427 = RegisteredTypes[ i ];
			}
			if( RegisteredTypes[ i ].GetName().Compare( TEXT( "NiagaraFloat" ) ) == 0 )
			{
				FloatFor427 = RegisteredTypes[ i ];
			}
		}

		InitializedNiagaraTypesFor427 = true;
	}

	if( DowngraderSettings->TargetVersion <= EEngineVersion::EV_4_27 )
	{
		FNiagaraTypeDefinition& Type = (FNiagaraTypeDefinition&)Variable->GetType();
		if( Type == FNiagaraTypeDefinition::GetVec2Def() )
		{
			Variable->SetType( Vector2For427 );
		}
	#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 0
		if( Type == FNiagaraTypeDefinition::GetVec3Def() ||
			Type == FNiagaraTypeDefinition::GetPositionDef() )
		{
			Variable->SetType( Vector3For427 );
		}
	#endif
		if( Type == FNiagaraTypeDefinition::GetVec4Def() )
		{
			Variable->SetType( Vector4For427 );
		}
	#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4
		if( Type == FNiagaraTypeHelper::GetDoubleDef() )
		{
			Variable->SetType( FloatFor427 );
		}
	#endif
	}
}
void FixNiagaraScript( UNiagaraScript* Script )
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
	UNiagaraScriptSourceBase* ScriptSourceBase = Script->GetLatestSource();
	UNiagaraScriptSource* ScriptSource = Cast< UNiagaraScriptSource >( ScriptSourceBase);
	if( ScriptSource )
	{
		if( ScriptSource->GetName().Contains( TEXT( "Ripple_Position" ) ) )
		{
			ScriptSource = ScriptSource;
		}
		UNiagaraGraph* FunctionGraph = CastChecked<UNiagaraGraph>( ScriptSource->NodeGraph );
		FixNiagaraGraph( FunctionGraph, nullptr );
	}

	TArray<FNiagaraVariableWithOffset>& SortedParameterOffsets = GetVariable< TArray<FNiagaraVariableWithOffset> >( &Script->RapidIterationParameters, TEXT( "NiagaraParameterStore" ), TEXT( "SortedParameterOffsets" ) );

	for( int i = 0; i < SortedParameterOffsets.Num(); i++ )
	{
		FixNiagaraVariable( &SortedParameterOffsets[ i ] );
	}

	//if( DowngraderSettings->TargetVersion < EEngineVersion::EV_5_6 )
	//{
	//	//If this isn't true, UNiagaraHierarchyScriptParameter::ParameterScriptVariable is nullptr because MigrateParameterDataToHierarchyRoot doesn't initialize it anymore
	//	Script->bMigrateParameterDataToHierarchyRoot = true;
	//}
#endif
}
void FixEmitterRendererProperties( UNiagaraEmitter* Emitter )
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
	TArray<TObjectPtr<UNiagaraRendererProperties>>& RendererProperties = GetVariable< TArray<TObjectPtr<UNiagaraRendererProperties>>>( Emitter, TEXT( "NiagaraEmitter" ), TEXT( "RendererProperties" ) );
	bool RemovedOne = false;
	do
	{
		RemovedOne = false;
		for( int i = 0; i < RendererProperties.Num(); i++ )
		{
			UNiagaraRendererProperties* Prop = RendererProperties[ i ];
		#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
			UNiagaraVolumeRendererProperties* VolumeRendererProperties = Cast< UNiagaraVolumeRendererProperties >( Prop );
			if( DowngraderSettings->TargetVersion < EEngineVersion::EV_5_3 )
			{
				if( VolumeRendererProperties )
				{
					RendererProperties.RemoveAt( i );
					RemovedOne = true;
				}
			}
		#endif
		}
	} while( RemovedOne );
#endif
}
struct FVersionedNiagaraEmitter;
void FixNiagaraEmitter(UNiagaraEmitter* Emitter, FVersionedNiagaraEmitter* VersionedNiagaraEmitter )
{
	if( !Emitter )
		return;

	#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3

	FixTexture( Emitter->ThumbnailImage );	
	
	RemovePropertyFlag( TEXT( "NiagaraEmitter" ), TEXT( "SimTarget" ), CPF_Deprecated );
	RemovePropertyFlag( TEXT( "NiagaraEmitter" ), TEXT( "Platforms" ), CPF_Deprecated );
	RemovePropertyFlag( TEXT( "NiagaraEmitter" ), TEXT( "RandomSeed" ), CPF_Deprecated );
	RemovePropertyFlag( TEXT( "NiagaraEmitter" ), TEXT( "bLocalSpace" ), CPF_Deprecated );
	RemovePropertyFlag( TEXT( "NiagaraEmitter" ), TEXT( "FixedBounds" ), CPF_Deprecated );
	RemovePropertyFlag( TEXT( "NiagaraEmitter" ), TEXT( "GraphSource" ), CPF_Deprecated );
	RemovePropertyFlag( TEXT( "NiagaraEmitter" ), TEXT( "bDeterminism" ), CPF_Deprecated );
	RemovePropertyFlag( TEXT( "NiagaraEmitter" ), TEXT( "AllocationMode" ), CPF_Deprecated );
	RemovePropertyFlag( TEXT( "NiagaraEmitter" ), TEXT( "SpawnScriptProps" ), CPF_Deprecated );
	RemovePropertyFlag( TEXT( "NiagaraEmitter" ), TEXT( "SimulationStages" ), CPF_Deprecated );
	RemovePropertyFlag( TEXT( "NiagaraEmitter" ), TEXT( "EditorParameters" ), CPF_Deprecated );
	RemovePropertyFlag( TEXT( "NiagaraEmitter" ), TEXT( "GPUComputeScript" ), CPF_Deprecated );
	RemovePropertyFlag( TEXT( "NiagaraEmitter" ), TEXT( "UpdateScriptProps" ), CPF_Deprecated );
	RemovePropertyFlag( TEXT( "NiagaraEmitter" ), TEXT( "PreAllocationCount" ), CPF_Deprecated );
	RemovePropertyFlag( TEXT( "NiagaraEmitter" ), TEXT( "RendererProperties" ), CPF_Deprecated );
	RemovePropertyFlag( TEXT( "NiagaraEmitter" ), TEXT( "ScalabilityOverrides" ), CPF_Deprecated );
	RemovePropertyFlag( TEXT( "NiagaraEmitter" ), TEXT( "AttributesToPreserve" ), CPF_Deprecated );
	RemovePropertyFlag( TEXT( "NiagaraEmitter" ), TEXT( "bInterpolatedSpawning" ), CPF_Deprecated );
	RemovePropertyFlag( TEXT( "NiagaraEmitter" ), TEXT( "bRequiresPersistentIDs" ), CPF_Deprecated );
	RemovePropertyFlag( TEXT( "NiagaraEmitter" ), TEXT( "EventHandlerScriptProps" ), CPF_Deprecated );
	RemovePropertyFlag( TEXT( "NiagaraEmitter" ), TEXT( "SharedEventGeneratorIds" ), CPF_Deprecated );
	RemovePropertyFlag( TEXT( "NiagaraEmitter" ), TEXT( "EmitterSpawnScriptProps" ), CPF_Deprecated );
	RemovePropertyFlag( TEXT( "NiagaraEmitter" ), TEXT( "EmitterUpdateScriptProps" ), CPF_Deprecated );
	RemovePropertyFlag( TEXT( "NiagaraEmitter" ), TEXT( "MaxGPUParticlesSpawnPerFrame" ), CPF_Deprecated );
	RemovePropertyFlag( TEXT( "NiagaraEmitter" ), TEXT( "ScratchPadScripts" ), CPF_Deprecated );
	RemovePropertyFlag( TEXT( "NiagaraEmitter" ), TEXT( "Parent" ), CPF_Deprecated );
	RemovePropertyFlag( TEXT( "NiagaraEmitter" ), TEXT( "ParentAtLastMerge" ), CPF_Deprecated );

	RemovePropertyFlag( TEXT( "NiagaraEmitterHandle" ), TEXT( "Source" ), CPF_Deprecated );
	RemovePropertyFlag( TEXT( "NiagaraEmitterHandle" ), TEXT( "LastMergedSource" ), CPF_Deprecated );
	RemovePropertyFlag( TEXT( "NiagaraEmitterHandle" ), TEXT( "Instance" ), CPF_Deprecated );

	int EditorDataOffset = GetMemberOffset( TEXT( "NiagaraEmitter" ), TEXT( "EditorData" ) );
	int SimulationStagesOffset = GetMemberOffset( TEXT( "NiagaraEmitter" ), TEXT( "SimulationStages" ) );
	int EditorParametersOffset = GetMemberOffset( TEXT( "NiagaraEmitter" ), TEXT( "EditorParameters" ) );
	int GPUComputeScriptOffset = GetMemberOffset( TEXT( "NiagaraEmitter" ), TEXT( "GPUComputeScript" ) );
	int RendererPropertiesOffset = GetMemberOffset( TEXT( "NiagaraEmitter" ), TEXT( "RendererProperties" ) );
	int EventHandlerScriptPropsOffset = GetMemberOffset( TEXT( "NiagaraEmitter" ), TEXT( "EventHandlerScriptProps" ) );
	int SharedEventGeneratorIdsOffset = GetMemberOffset( TEXT( "NiagaraEmitter" ), TEXT( "SharedEventGeneratorIds" ) );
	int ParentOffset = GetMemberOffset( TEXT( "NiagaraEmitter" ), TEXT( "Parent" ) );
	int ParentAtLastMergeOffset = GetMemberOffset( TEXT( "NiagaraEmitter" ), TEXT( "ParentAtLastMerge" ) );

	
	int VersionedEditorData = GetMemberOffset( TEXT( "VersionedNiagaraEmitterData" ), TEXT( "EditorData" ) );
	int VersionedSimulationStages = GetMemberOffset( TEXT( "VersionedNiagaraEmitterData" ), TEXT( "SimulationStages" ) );
	int VersionedEditorParameters = GetMemberOffset( TEXT( "VersionedNiagaraEmitterData" ), TEXT( "EditorParameters" ) );
	int VersionedGPUComputeScript = GetMemberOffset( TEXT( "VersionedNiagaraEmitterData" ), TEXT( "GPUComputeScript" ) );
	int VersionedRendererProperties = GetMemberOffset( TEXT( "VersionedNiagaraEmitterData" ), TEXT( "RendererProperties" ) );
	int VersionedDataSharedEventGeneratorIds = GetMemberOffset( TEXT( "VersionedNiagaraEmitterData" ), TEXT( "SharedEventGeneratorIds" ) );
	int VersionedEventHandlerScriptProps = GetMemberOffset( TEXT( "VersionedNiagaraEmitterData" ), TEXT( "EventHandlerScriptProps" ) );
	//int VersionedParent = GetMemberOffset( TEXT( "VersionedNiagaraEmitterData" ), TEXT( "Parent" ) );
	//int VersionedParentAtLastMerge = GetMemberOffset( TEXT( "VersionedNiagaraEmitterData" ), TEXT( "ParentAtLastMerge" ) );

	Emitter->ForEachVersionData(
		[&]( const FVersionedNiagaraEmitterData& Data )
	{
		UNiagaraScriptSource* GraphSource = Cast<UNiagaraScriptSource>( Data.GraphSource );
		if( GraphSource )
		{
			FixNiagaraGraph( GraphSource->NodeGraph, VersionedNiagaraEmitter );
		}
		
		Emitter->SimTarget_DEPRECATED = Data.SimTarget;

		Emitter->Platforms_DEPRECATED = Data.Platforms;
		Emitter->RandomSeed_DEPRECATED = Data.RandomSeed;
		//Emitter->EditorData_DEPRECATED = Data.EditorData;
		
		AssignVariable< TObjectPtr<UNiagaraEditorDataBase>>( Emitter, EditorDataOffset, &Data, VersionedEditorData );
		Emitter->bLocalSpace_DEPRECATED = Data.bLocalSpace;
		Emitter->FixedBounds_DEPRECATED = Data.FixedBounds;
		Emitter->GraphSource_DEPRECATED = Data.GraphSource;
		Emitter->bDeterminism_DEPRECATED = Data.bDeterminism;
		Emitter->AllocationMode_DEPRECATED = Data.AllocationMode;
		if( DowngraderSettings->TargetVersion <= EEngineVersion::EV_4_27 )
		{
			if( Emitter->AllocationMode_DEPRECATED == EParticleAllocationMode::FixedCount )
			{
				Emitter->AllocationMode_DEPRECATED = EParticleAllocationMode::AutomaticEstimate;
			}
		}
		Emitter->SpawnScriptProps_DEPRECATED = Data.SpawnScriptProps;
		AssignVariable< TArray<TObjectPtr<UNiagaraSimulationStageBase>>>( Emitter, SimulationStagesOffset, &Data, VersionedSimulationStages );		
		AssignVariable< TObjectPtr<UNiagaraEditorParametersAdapterBase>>( Emitter, EditorParametersOffset, &Data, VersionedEditorParameters );
		AssignVariable< TObjectPtr<UNiagaraScript>>( Emitter, GPUComputeScriptOffset, &Data, VersionedGPUComputeScript );
		Emitter->UpdateScriptProps_DEPRECATED = Data.UpdateScriptProps;
		Emitter->PreAllocationCount_DEPRECATED = Data.PreAllocationCount;
		AssignVariable< TArray<TObjectPtr<UNiagaraRendererProperties>>>( Emitter, RendererPropertiesOffset, &Data, VersionedRendererProperties );
		Emitter->ScalabilityOverrides_DEPRECATED = Data.ScalabilityOverrides;
		Emitter->AttributesToPreserve_DEPRECATED = Data.AttributesToPreserve;
		#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 6
			Emitter->bInterpolatedSpawning_DEPRECATED = Data.bInterpolatedSpawning;
		#endif
		Emitter->bRequiresPersistentIDs_DEPRECATED = Data.bRequiresPersistentIDs;
		AssignVariable< TArray<FNiagaraEventScriptProperties>>( Emitter, EventHandlerScriptPropsOffset, &Data, VersionedEventHandlerScriptProps );
		AssignVariable< TArray<FName>>( Emitter, SharedEventGeneratorIdsOffset, &Data, VersionedDataSharedEventGeneratorIds );
		Emitter->EmitterSpawnScriptProps_DEPRECATED = Data.EmitterSpawnScriptProps;
		Emitter->EmitterUpdateScriptProps_DEPRECATED = Data.EmitterUpdateScriptProps;
		Emitter->MaxGPUParticlesSpawnPerFrame_DEPRECATED = Data.MaxGPUParticlesSpawnPerFrame;
		Emitter->RendererBindings_DEPRECATED = Data.RendererBindings;
		//GlobalSpawnCountScaleOverrides has an implementation !

		if( Emitter->LibraryVisibility == ENiagaraScriptLibraryVisibility::Library )
		{
			Emitter->bExposeToLibrary_DEPRECATED = true;
		}

		FixEmitterRendererProperties( Emitter );
		
		for( int i = 0; i < Data.ScratchPads.Get()->Scripts.Num(); i++ )
		{
			UNiagaraScript* Script = Data.ScratchPads.Get()->Scripts[ i ];
			UNiagaraScriptSourceBase* SourceBase = Script->Source_DEPRECATED.Get();
			UNiagaraScriptSource* Source = Cast<UNiagaraScriptSource>( SourceBase );
			if( Source )
			{				
				FixNiagaraGraph( Source->NodeGraph, VersionedNiagaraEmitter );
			}
			Emitter->ScratchPadScripts_DEPRECATED.Add( Script );
		}

		//Emitter->Parent_DEPRECATED = Data.VersionedParent.Emitter;
		FixNiagaraEmitter( Data.VersionedParent.Emitter, (FVersionedNiagaraEmitter*)&Data.VersionedParent );
		AssignVariable( Emitter, ParentOffset, Data.VersionedParent.Emitter );		

		//Emitter->ParentAtLastMerge_DEPRECATED = Data.VersionedParentAtLastMerge.Emitter;
		FixNiagaraEmitter( Data.VersionedParentAtLastMerge.Emitter, (FVersionedNiagaraEmitter*)&Data.VersionedParentAtLastMerge );
		AssignVariable( Emitter, ParentAtLastMergeOffset, Data.VersionedParentAtLastMerge.Emitter );		

		FixNiagaraScript( Emitter->SpawnScriptProps_DEPRECATED.Script );
		FixNiagaraScript( Emitter->UpdateScriptProps_DEPRECATED.Script );
		FixNiagaraScript( Emitter->EmitterSpawnScriptProps_DEPRECATED.Script );
		FixNiagaraScript( Emitter->EmitterUpdateScriptProps_DEPRECATED.Script );
	}
	);

	//TObjectPtr<UNiagaraEmitter>& Parent = GetVariable< TObjectPtr<UNiagaraEmitter>>( Emitter, ParentOffset );
	//if( Parent )
	//{
	//	FixNiagaraEmitter( Parent, &Data.VersionedParent );
	//}
	//TObjectPtr<UNiagaraEmitter>& ParentAtLastMerge = GetVariable< TObjectPtr<UNiagaraEmitter>>( Emitter, ParentAtLastMergeOffset );
	//if( ParentAtLastMerge )
	//{
	//	FixNiagaraEmitter( ParentAtLastMerge, nullptr );
	//}
	#endif
}
void FixParticleSystem( UParticleSystem* ParticleSystem )
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
	FixTexture( ParticleSystem->ThumbnailImage );
#endif
}
void FixNiagaraSystem( UNiagaraSystem* NiagaraSystem )
{
	#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3

	FixTexture( NiagaraSystem->ThumbnailImage );

	FNiagaraUserRedirectionParameterStore& ExposedParameters = GetVariable< FNiagaraUserRedirectionParameterStore >( NiagaraSystem, TEXT( "NiagaraSystem" ), TEXT( "ExposedParameters" ) );
	TArray<FNiagaraVariableWithOffset>& SortedParameterOffsets = GetVariable< TArray<FNiagaraVariableWithOffset> >( &ExposedParameters, TEXT( "NiagaraUserRedirectionParameterStore" ), TEXT( "SortedParameterOffsets" ) );

	for (int i = 0; i < SortedParameterOffsets.Num(); i++)
	{
		FixNiagaraVariable( &SortedParameterOffsets[i] );
	}

	TMap<FNiagaraVariable, FNiagaraVariable>& UserParameterRedirects = GetVariable< TMap<FNiagaraVariable, FNiagaraVariable> >( &ExposedParameters, TEXT( "NiagaraUserRedirectionParameterStore" ), TEXT( "UserParameterRedirects" ) );
	for (auto it = UserParameterRedirects.CreateIterator(); it; ++it)
	{
		FNiagaraVariable& Key = it.Key();
		FNiagaraVariable& Value = it.Value();

		FixNiagaraVariable( &Key );
		FixNiagaraVariable( &Value );
	}
	//LocalModules & "DynamicPins" are here
	for( int i = 0; i < NiagaraSystem->ScratchPadScripts.Num(); i++ )
	{
		UNiagaraScript* Script = NiagaraSystem->ScratchPadScripts[i];
		FixNiagaraScript( Script );
	}

	TArray<FNiagaraEmitterHandle>& EmitterHandles = NiagaraSystem->GetEmitterHandles();
	for( int i = 0; i < EmitterHandles.Num(); i++ )
	{
		FNiagaraEmitterHandle& EmitterHandle = EmitterHandles[ i ];
		FVersionedNiagaraEmitterData* EmitterData = EmitterHandle.GetEmitterData();
		int VersionedInstanceOffset = 88;
		FVersionedNiagaraEmitter* VersionedInstance = (FVersionedNiagaraEmitter*)( ( (uint8*)&EmitterHandle ) + VersionedInstanceOffset );
		int SourceOffset = 48;
		int InstanceOffset = 72;
		TObjectPtr<UNiagaraEmitter>* Source = ( TObjectPtr<UNiagaraEmitter>* ) ( ( (uint8*)&EmitterHandle ) + SourceOffset );
		TObjectPtr<UNiagaraEmitter>* Instance = ( TObjectPtr<UNiagaraEmitter>* ) ( ( (uint8*)&EmitterHandle ) + InstanceOffset );
		*Source = VersionedInstance->Emitter;
		*Instance = VersionedInstance->Emitter;

		FixNiagaraEmitter( VersionedInstance->Emitter, VersionedInstance );
	}
	#endif
}
void AddGraphs( UEdGraph* Graph, TArray< UEdGraph*>& AllGraphs )
{
	if( !Graph )
		return;

	AllGraphs.Add( Graph );

	for( int s = 0; s < Graph->SubGraphs.Num(); s++ )
	{
		AddGraphs( Graph->SubGraphs[ s ], AllGraphs );
	}
}
void AddGraphNodes( UEdGraph* Graph, TArray< UEdGraphNode*>& AllNodes )
{
	if( !Graph )
		return;

	for( int n = 0; n < Graph->Nodes.Num(); n++ )
	{
		AllNodes.Add( Graph->Nodes[ n ] );
	}
	for( int s = 0; s < Graph->SubGraphs.Num(); s++ )
	{
		AddGraphNodes( Graph->SubGraphs[s], AllNodes );
	}
}
TArray< UEdGraphNode*> GetAllBlueprintNodes( UBlueprint* Blueprint )
{
	TArray< UEdGraphNode*> AllNodes;
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 0
	for( int i = 0; i < Blueprint->FunctionGraphs.Num(); i++ )
	{
		UEdGraph* Graph = Blueprint->FunctionGraphs[ i ].Get();
		AddGraphNodes( Graph, AllNodes );
	}
	for( int i = 0; i < Blueprint->UbergraphPages.Num(); i++ )
	{
		UEdGraph* Graph = Blueprint->UbergraphPages[ i ].Get();
		AddGraphNodes( Graph, AllNodes );
	}
	for( int i = 0; i < Blueprint->EventGraphs.Num(); i++ )
	{
		UEdGraph* Graph = Blueprint->EventGraphs[ i ].Get();
		AddGraphNodes( Graph, AllNodes );
	}
	for( int32 i = 0; i < Blueprint->MacroGraphs.Num(); ++i )
	{
		UEdGraph* Graph = Blueprint->MacroGraphs[ i ];
		AddGraphNodes( Graph, AllNodes );
	}
	for( int32 i = 0; i < Blueprint->DelegateSignatureGraphs.Num(); ++i )
	{
		UEdGraph* Graph = Blueprint->DelegateSignatureGraphs[ i ];
		AddGraphNodes( Graph, AllNodes );
	}
	for( int32 BPIdx = 0; BPIdx < Blueprint->ImplementedInterfaces.Num(); BPIdx++ )
	{
		const FBPInterfaceDescription& InterfaceDesc = Blueprint->ImplementedInterfaces[ BPIdx ];
		for( int32 GraphIdx = 0; GraphIdx < InterfaceDesc.Graphs.Num(); GraphIdx++ )
		{
			UEdGraph* Graph = InterfaceDesc.Graphs[ GraphIdx ];
			AddGraphNodes( Graph, AllNodes );
		}
	}
#endif
	return AllNodes;
}
TArray< UEdGraph* > GetAllGraphs( UBlueprint* Blueprint )
{
	TArray< UEdGraph*> AllGraphs;
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 0
	for( int i = 0; i < Blueprint->FunctionGraphs.Num(); i++ )
	{
		UEdGraph* Graph = Blueprint->FunctionGraphs[ i ].Get();
		AddGraphs( Graph, AllGraphs );
	}
	for( int i = 0; i < Blueprint->UbergraphPages.Num(); i++ )
	{
		UEdGraph* Graph = Blueprint->UbergraphPages[ i ].Get();
		AddGraphs( Graph, AllGraphs );
	}
	for( int i = 0; i < Blueprint->EventGraphs.Num(); i++ )
	{
		UEdGraph* Graph = Blueprint->EventGraphs[ i ].Get();
		AddGraphs( Graph, AllGraphs );
	}
	for( int32 i = 0; i < Blueprint->MacroGraphs.Num(); ++i )
	{
		UEdGraph* Graph = Blueprint->MacroGraphs[ i ];
		AddGraphs( Graph, AllGraphs );
	}
	for( int32 i = 0; i < Blueprint->DelegateSignatureGraphs.Num(); ++i )
	{
		UEdGraph* Graph = Blueprint->DelegateSignatureGraphs[ i ];
		AddGraphs( Graph, AllGraphs );
	}
	for( int32 BPIdx = 0; BPIdx < Blueprint->ImplementedInterfaces.Num(); BPIdx++ )
	{
		const FBPInterfaceDescription& InterfaceDesc = Blueprint->ImplementedInterfaces[ BPIdx ];
		for( int32 GraphIdx = 0; GraphIdx < InterfaceDesc.Graphs.Num(); GraphIdx++ )
		{
			UEdGraph* Graph = InterfaceDesc.Graphs[ GraphIdx ];
			AddGraphs( Graph, AllGraphs );
		}
	}
#endif
	return AllGraphs;
}
void BreakAllPinsExceptExec( UEdGraphNode* Node )
{
	const TArray<UEdGraphPin*>& AllPins = Node->GetAllPins();

	TSet<UEdGraphNode*> NodeList;
	NodeList.Add( Node );

	// Iterate over each pin and break all links
	for( int i = 0; i < AllPins.Num(); i++ )
	{
		UEdGraphPin* Pin = AllPins[ i ];

		if( !Pin->GetName().Contains( TEXT( "exe" ) ) && !Pin->GetName().Contains( TEXT( "then" ) ) )
		{
			// Save all the connected nodes to be notified below
			for( UEdGraphPin* Connection : Pin->LinkedTo )
			{
				NodeList.Add( Connection->GetOwningNode() );
			}

			Pin->BreakAllPinLinks();
		}
	}

	// Send a notification to all nodes that lost a connection
	for( UEdGraphNode* LocalNode : NodeList )
	{
		LocalNode->NodeConnectionListChanged();
	}
}

bool HasEngineModifications()
{
	#ifdef DOWNGRADER_CUSTOM_ENGINE
		return true;
	#else
		return false;
	#endif
}
class UAnimSequence_Downgrader : public UAnimSequence
{
public:
	TArray<struct FRawAnimSequenceTrack>& GetRawAnimationData() { return RawAnimationData; }
	TArray<FName>& GetAnimationTrackNames() { return AnimationTrackNames; }
	int32& GetNumFrames() { return NumFrames; };
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 0
	int32& GetNumberOfKeys() { return NumberOfKeys; };	
	FFrameRate& GetSamplingFrameRate() { return SamplingFrameRate; }
#endif
	float& GetSequenceLength() { return SequenceLength; }
	#ifdef DOWNGRADER_CUSTOM_ENGINE
	TArray<struct FTrackToSkeletonMap>& GetTrackToSkeletonMapTable() { return TrackToSkeletonMapTable; }
	#endif
};
class IAnimationDataModel;
void FixFloatCurves( const TArray<FFloatCurve>& FloatCurves )
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
	//Fix FFloatCurve
	RemovePropertyFlag( TEXT( "FloatCurve" ), TEXT( "Name" ), CPF_Deprecated );
	RemovePropertyFlag( TEXT( "FloatCurve" ), TEXT( "LastObservedName" ), CPF_Deprecated );
	for( int32 Index = 0; Index < FloatCurves.Num(); ++Index )
	{
		FFloatCurve& Curve = (FFloatCurve&)FloatCurves[ Index ];
		FName& CurveName = GetVariable< FName>( &Curve, TEXT( "FloatCurve" ), TEXT( "CurveName" ) );
		Curve.Name_DEPRECATED = FSmartName( CurveName, 0 );
		Curve.LastObservedName_DEPRECATED = CurveName;
	}
#endif
}
void FixFloatCurves( IAnimationDataModel* Model )
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
	const TArray<FFloatCurve>& FloatCurves = Model->GetFloatCurves();
	FixFloatCurves( FloatCurves );
#endif
}
void FixAnimSequenceBase( UAnimSequenceBase* AnimationSequenceBase )
{
	if (DowngraderSettings->TargetVersion <= EEngineVersion::EV_5_3)
	{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
		TObjectPtr<UAnimDataModel>& DataModel = GetVariable< TObjectPtr<UAnimDataModel>>( AnimationSequenceBase, TEXT( "AnimSequenceBase" ), TEXT( "DataModel" ) );
		TScriptInterface<IAnimationDataModel>& DataModelInterface = GetVariable< TScriptInterface<IAnimationDataModel>>( AnimationSequenceBase, TEXT( "AnimSequenceBase" ), TEXT( "DataModelInterface" ) );

		if (!DataModel.Get())
		{
			DataModel = NewObject<UAnimDataModel>( AnimationSequenceBase, FName( TEXT( "AnimationDataModel" ) ) );
		}

		UAnimDataModel* AnimDataModel = DataModel.Get();
		TArray<FBoneAnimationTrack>& DM_BoneAnimationTracks = GetVariable< TArray<FBoneAnimationTrack> >( AnimDataModel, TEXT( "AnimDataModel" ), TEXT( "BoneAnimationTracks" ) );
		FFrameRate& DM_FrameRate = GetVariable< FFrameRate >( AnimDataModel, TEXT( "AnimDataModel" ), TEXT( "FrameRate" ) );
		float& PlayLength = GetVariable< float >( AnimDataModel, TEXT( "AnimDataModel" ), TEXT( "PlayLength" ) );
		int32& NumberOfFrames = GetVariable< int32 >( AnimDataModel, TEXT( "AnimDataModel" ), TEXT( "NumberOfFrames" ) );
		int32& NumberOfKeys = GetVariable< int32 >( AnimDataModel, TEXT( "AnimDataModel" ), TEXT( "NumberOfKeys" ) );
		FAnimationCurveData& CurveData = GetVariable< FAnimationCurveData >( AnimDataModel, TEXT( "AnimDataModel" ), TEXT( "CurveData" ) );
		TArray<FAnimatedBoneAttribute>& AnimatedBoneAttributes = GetVariable< TArray<FAnimatedBoneAttribute> >( AnimDataModel, TEXT( "AnimDataModel" ), TEXT( "AnimatedBoneAttributes" ) );

		DM_BoneAnimationTracks.Reset( 0 );

		TArray<FName> AnimationTrackNames;
		if (DataModelInterface.GetInterface())
			DataModelInterface->GetBoneTrackNames( AnimationTrackNames );

		for (int i = 0; i < AnimationTrackNames.Num(); i++)
		{
			TArray<FTransform> OutTransforms;
			DataModelInterface->GetBoneTrackTransforms( AnimationTrackNames[i], OutTransforms );
			int BoneTreeIndex = DataModelInterface->GetBoneTrackIndexByName( AnimationTrackNames[i] );

			FRawAnimSequenceTrack NewTrack;

			for (int t = 0; t < OutTransforms.Num(); t++)
			{
				FTransform Trans = OutTransforms[t];
				FVector Location = Trans.GetLocation();
				FQuat Quat = Trans.GetRotation();
				FVector Scale = Trans.GetScale3D();
				NewTrack.PosKeys.Add( FVector3f( Location.X, Location.Y, Location.Z ) );
				NewTrack.RotKeys.Add( FQuat4f( Quat.X, Quat.Y, Quat.Z, Quat.W ) );
				NewTrack.ScaleKeys.Add( FVector3f( Scale.X, Scale.Y, Scale.Z ) );
			}

			FBoneAnimationTrack NewBoneAnimationTrack;
			NewBoneAnimationTrack.BoneTreeIndex = BoneTreeIndex;
			NewBoneAnimationTrack.Name = AnimationTrackNames[i];
			NewBoneAnimationTrack.InternalTrackData = NewTrack;
			DM_BoneAnimationTracks.Add( NewBoneAnimationTrack );
		}

		DM_FrameRate = DataModelInterface->GetFrameRate();
		PlayLength = DataModelInterface->GetPlayLength();
		NumberOfFrames = DataModelInterface->GetNumberOfFrames();
		NumberOfKeys = DataModelInterface->GetNumberOfKeys();
		CurveData = DataModelInterface->GetCurveData();
		AnimatedBoneAttributes = DataModelInterface->GetAttributes();

		FixFloatCurves( AnimDataModel );

		FRawCurveTracks& RawCurveData = GetVariable< FRawCurveTracks>( AnimationSequenceBase, TEXT( "AnimSequenceBase" ), TEXT( "RawCurveData" ) );
		FixFloatCurves( RawCurveData.FloatCurves );

		DataModelInterface = nullptr;
#endif
	}
}
void FixAnimSequence( UAnimSequence* AnimationSequence )
{
	#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3

	if( DowngraderSettings->TargetVersion <= EEngineVersion::EV_5_0 )
	{
		FPerPlatformFrameRate& PlatformTargetFrameRate = GetVariable< FPerPlatformFrameRate>( AnimationSequence, TEXT( "AnimSequence" ), TEXT( "PlatformTargetFrameRate" ) );
		FFrameRate& TargetFrameRate = GetVariable< FFrameRate>( AnimationSequence, TEXT( "AnimSequence" ), TEXT( "TargetFrameRate" ) );
		//In cases like 60000
		if (TargetFrameRate.Numerator > 1000)
		{
			TargetFrameRate.Numerator/= 1000;
			TargetFrameRate.Denominator = 1;
		}
		PlatformTargetFrameRate = TargetFrameRate;
	}
	if( DowngraderSettings->TargetVersion <= EEngineVersion::EV_4_27 )
	{
		#if defined DOWNGRADER_CUSTOM_ENGINE
		//AddPropertyFlag( TEXT( "AnimSequence" ), TEXT( "TrackToSkeletonMapTable" ), CPF_Deprecated );
		//RemovePropertyFlag( TEXT( "AnimSequence" ), TEXT( "RawAnimationData" ), CPF_Deprecated );//Property doesn't exist

		IAnimationDataController& Controller = AnimationSequence->GetController();
		IAnimationDataModel* Model = (IAnimationDataModel*)Controller.GetModel();

		UAnimSequence_Downgrader* AnimationSequence2 = (UAnimSequence_Downgrader*)AnimationSequence;

		TArray<struct FRawAnimSequenceTrack>& RawAnimationData = AnimationSequence2->GetRawAnimationData();
		TArray<FName>& AnimationTrackNames = AnimationSequence2->GetAnimationTrackNames();
		TArray<struct FTrackToSkeletonMap>& TrackToSkeletonMapTable = AnimationSequence2->GetTrackToSkeletonMapTable();
		RawAnimationData.Empty( 0 );
		TrackToSkeletonMapTable.Empty( 0 );
		if( AnimationTrackNames.Num() == 0 )
		{
			Model->GetBoneTrackNames( AnimationTrackNames );
		}
		int32 MaxNumberOfTrackKeys = 0;
		for( int i = 0; i < AnimationTrackNames.Num(); i++ )
		{
			TArray<FTransform> OutTransforms;
			Model->GetBoneTrackTransforms( AnimationTrackNames[ i ], OutTransforms );
			int BoneTreeIndex = Model->GetBoneTrackIndexByName( AnimationTrackNames[ i ] );
			FRawAnimSequenceTrack NewTrack;

			//4.27 errors out in CompressRawAnimSequenceTrack if keys.num() is 0
			if (OutTransforms.Num() == 0)
			{
				FTransform DefaultTransform;
				OutTransforms.Add( DefaultTransform );
			}
			for( int t = 0; t < OutTransforms.Num(); t++ )
			{
				FTransform Trans = OutTransforms[ t ];
				FVector Location = Trans.GetLocation();
				FQuat Quat = Trans.GetRotation();
				FVector Scale = Trans.GetScale3D();
				NewTrack.PosKeys.Add( FVector3f( Location.X, Location.Y, Location.Z ) );
				NewTrack.RotKeys.Add( FQuat4f( Quat.X, Quat.Y, Quat.Z, Quat.W ) );
				NewTrack.ScaleKeys.Add( FVector3f( Scale.X, Scale.Y, Scale.Z ) );
			}

			MaxNumberOfTrackKeys = FMath::Max( MaxNumberOfTrackKeys, NewTrack.PosKeys.Num() );
			RawAnimationData.Add( NewTrack );

			FTrackToSkeletonMap NewTrackToSkeletonMap;
			NewTrackToSkeletonMap.BoneTreeIndex = i;// BoneTreeIndex;
			TrackToSkeletonMapTable.Add( NewTrackToSkeletonMap );
		}

		//Fix SamplingFrameRate
		const int32 NumberOfFrames = FMath::Max( AnimationSequence2->GetNumberOfKeys() - 1, 1 );
		const double DecimalFrameRate = (double)NumberOfFrames / ( (double)AnimationSequence2->GetSequenceLength() > 0.0 ? (double)AnimationSequence2->GetSequenceLength() : 1.0 );
		const double Denominator = 1000000.0;
		AnimationSequence2->GetSamplingFrameRate() = FFrameRate( DecimalFrameRate * Denominator, Denominator );

		AnimationSequence2->GetNumberOfKeys() = MaxNumberOfTrackKeys;
		AnimationSequence2->GetNumFrames() = MaxNumberOfTrackKeys;

		FixFloatCurves( Model );
		#endif
	}
	else if( DowngraderSettings->TargetVersion == EEngineVersion::EV_5_0 ||
			 DowngraderSettings->TargetVersion == EEngineVersion::EV_5_1 )
	{
		FixAnimSequenceBase( AnimationSequence );
	}
	else if( DowngraderSettings->TargetVersion == EEngineVersion::EV_5_2 )
	{
		IAnimationDataModel* Model = AnimationSequence->GetDataModelInterface().GetInterface();
		FixFloatCurves( Model );
	}

	FRawCurveTracks& RawCurveData = GetVariable< FRawCurveTracks>( AnimationSequence, TEXT( "AnimSequenceBase" ), TEXT( "RawCurveData" ) );
	FixFloatCurves( RawCurveData.FloatCurves );

	if (DowngraderSettings->TargetVersion < EEngineVersion::EV_5_3)
	{
		UAnimBoneCompressionSettings* BoneCompressionSettings = Cast< UAnimBoneCompressionSettings>( StaticLoadObject( UAnimBoneCompressionSettings::StaticClass(), nullptr, TEXT( "/Engine/Animation/DefaultAnimBoneCompressionSettings" ) ) );
		UAnimCurveCompressionSettings* CurveCompressionSettings = Cast< UAnimCurveCompressionSettings>( StaticLoadObject( UAnimCurveCompressionSettings::StaticClass(), nullptr, TEXT( "/Engine/Animation/DefaultAnimCurveCompressionSettings" ) ));
		UVariableFrameStrippingSettings* VariableFrameStrippingSettings = Cast< UVariableFrameStrippingSettings>( StaticLoadObject( UVariableFrameStrippingSettings::StaticClass(), nullptr, TEXT( "/Engine/Animation/DefaultVariableFrameStrippingSettings" ) ));

		//It's possible it's using the ACL plugin which is available from 5.3
		if (AnimationSequence->BoneCompressionSettings != BoneCompressionSettings)
			AnimationSequence->BoneCompressionSettings = BoneCompressionSettings;
		if (AnimationSequence->CurveCompressionSettings != CurveCompressionSettings)
			AnimationSequence->CurveCompressionSettings = CurveCompressionSettings;
		if (AnimationSequence->VariableFrameStrippingSettings != VariableFrameStrippingSettings)
			AnimationSequence->VariableFrameStrippingSettings = VariableFrameStrippingSettings;
	}
	#endif
}
class UBlendSpace;
void FixBlendSpace( UBlendSpace* BlendSpace )
{
	#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
	if ( DowngraderSettings->TargetVersion <= EEngineVersion::EV_4_27 )
	{
		BlendSpace->bInterpolateUsingGrid = true;
		BlendSpace->ResampleData();
	}
	#endif
}
void FixPoseAsset( UPoseAsset* PoseAsset )
{
	#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
	if( DowngraderSettings->TargetVersion <= EEngineVersion::EV_5_2 )
	{
		FPoseDataContainer& PoseContainer = GetVariable< FPoseDataContainer >( PoseAsset, TEXT( "PoseAsset" ), TEXT( "PoseContainer" ) );
		TArray<FSmartName>& PoseNames = GetVariable< TArray<FSmartName> >( &PoseContainer, TEXT( "PoseDataContainer" ), TEXT( "PoseNames" ) );
		TArray<FName>& PoseFNames = GetVariable< TArray<FName> >( &PoseContainer, TEXT( "PoseDataContainer" ), TEXT( "PoseFNames" ) );

		PoseNames.Reset( 0 );
		for( int i = 0; i < PoseFNames.Num(); i++ )
		{
			PoseNames.Add( FSmartName( PoseFNames[ i ], 0 ) );
		}
		
		PoseFNames.Reset( 0 );
	}
	#endif
}
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
class UPoseWatch2
	: public UPoseWatch
{
public:
	FColor GetColor() 
	{
	#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
		return Color_DEPRECATED;
	#else
		return Color;
	#endif
	}
	FName GetIconName() 
	{
		return IconName_DEPRECATED;
	}
	TArray<TObjectPtr<UPoseWatchElement>>& GetElements() { return Elements; }
};
#endif
class ULevelSequence;
void FixLevelSequence( ULevelSequence* LevelSequence )
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
	RemovePropertyFlag( TEXT("LevelSequence"), TEXT("ObjectReferences"), CPF_Deprecated );
	//LevelSequence->GetBindingReferences();
	const UMovieScene* MovieScene = LevelSequence->GetMovieScene();
#endif
}
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
class UWidgetComponent_Downgrader : public UWidgetComponent
{
public:
	EWindowVisibility& GetWindowVisibility() { return WindowVisibility; }
};
#endif
void RemoveGraphNode( UEdGraphNode* NodeToRemove )
{
	UEdGraph* OwnerGraph = NodeToRemove->GetGraph();
	OwnerGraph->Nodes.Remove( NodeToRemove );

	int RemovedPins = 0;
	int RemovedLinks = 0;
	for (int i = 0; i < OwnerGraph->Nodes.Num(); i++)
	{
		auto Node = OwnerGraph->Nodes[i];
		bool RemovedConnection = false;
		for (int p = 0; p < Node->Pins.Num(); p++)
		{
			auto Pin = Node->Pins[p];
			if (Pin->GetOwningNode() == NodeToRemove)
			{
				Node->Pins.RemoveAt( p );
				RemovedPins++;
				p--;
				RemovedConnection = true;
			}
			for (int l = 0; l < Pin->LinkedTo.Num(); l++)
			{
				if (Pin->LinkedTo[l]->GetOwningNode() == NodeToRemove)
				{
					Pin->LinkedTo.RemoveAt( l );
					RemovedLinks++;
					l--;
					RemovedConnection = true;
				}
			}
		}

		if (RemovedConnection)
		{
			Node->NodeConnectionListChanged();
		}
	}
}
void RemoveGraphNode2( UEdGraphNode* NodeToRemove )
{
	UEdGraph* OwnerGraph = NodeToRemove->GetGraph();

	// Step 1: Break all links to/from this node
	NodeToRemove->BreakAllNodeLinks();

	// Step 2: Remove node from the graph
	OwnerGraph->RemoveNode( NodeToRemove );

	// Step 3: Mark graph as modified for undo/redo support
	OwnerGraph->Modify();

	// Step 4: Mark node for destruction (GC will clean it later)
	NodeToRemove->ConditionalBeginDestroy();
}
void FixGeneratedPinLinksFor427( UEdGraphNode* Node )
{
	if( Node->Pins.Num() <= 3 )
		return;

	UEdGraph* OwnerGraph = Node->GetGraph();
	UEdGraphPin* Pin = Node->Pins[ 3 ];
	UEdGraphPin* OutputPin = Node->Pins[ 2 ];
	OutputPin->SubPins.Reset( 0 );
	int LinksRemoved = 0;

	UEdGraphNode* OwnerNode = Pin->GetOwningNode();

	//for( int p = 0; p < OwnerNode->Pins.Num(); p++ )
	//{
	//	for( int s = 0; s < OwnerNode->Pins[ p ]->SubPins.Num(); s++ )
	//	{
	//		if( OwnerNode->Pins[ p ]->SubPins[ s ] == Pin )
	//		{
	//			OwnerNode->Pins[ p ]->SubPins.RemoveAt( s );
	//			s--;
	//			LinksRemoved--;
	//		}
	//	}
	//}

	for( int l = 0; l < Pin->LinkedTo.Num(); l++ )
	{
		UEdGraphPin* TargetPin = Pin->LinkedTo[ l ];
		OutputPin->LinkedTo.Add( TargetPin );

		for( int l2 = 0; l2 < TargetPin->LinkedTo.Num(); l2++ )
		{
			if( TargetPin->LinkedTo[ l2 ] == Pin )
			{
				TargetPin->LinkedTo[ l2 ] = OutputPin;
			}
		}		
	}

	OwnerNode->Pins.Remove( Pin );
}
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
struct FMemberReference_Downgrader : public FMemberReference
{
	void SetMemberName( FString Str )
	{
		MemberName = FName( Str );
	}
	void SetMemberParent( TObjectPtr<UObject>& Param )
	{
		MemberParent = Param;
	}
	TObjectPtr<UObject>& GetMemberParent()
	{
		return MemberParent;
	}
};
#endif
class ALandscapeProxy;
void RemoveNaniteComponents( ALandscapeProxy * Proxy)//replacement for ALandscapeProxy::RemoveNaniteComponents that I can't link with
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
	for (ULandscapeNaniteComponent* NaniteComponent : Proxy->NaniteComponents)
	{
		if (NaniteComponent)
		{
			// Don't call modify when detaching the nanite component, this is non-transactional "derived data", regenerated any time the source landscape data changes. This prevents needlessly dirtying the package :
			//NaniteComponent->SetEnabled( false );
			NaniteComponent->DetachFromComponent( FDetachmentTransformRules( EDetachmentRule::KeepRelative, /*bInCallModify = */false ) );
			NaniteComponent->DestroyComponent();

			NaniteComponent->Rename( nullptr, GetTransientPackage() );
		}		
	}

	Proxy->NaniteComponents.Empty();
#endif
}
void FixTexturesFromMaterialInstances( TArray<UMaterialInterface*> Materials )
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
	for( int m = 0; m < Materials.Num(); m++ )
	{
		UMaterialInstance* MI = Cast<UMaterialInstance>( Materials[m] );
		if( MI )
		{
			for( int t = 0; t < MI->TextureParameterValues.Num(); t++ )
			{
				UTexture* Tex = MI->TextureParameterValues[t].ParameterValue;
				if( Tex )
					//These are Weightmaps actually
					FixTexture( Tex, false );
			}
		}
	}
#endif
}
class UGeometryCollection;
void FixGeometryCollection( UGeometryCollection* Collection );

bool FixComponents( UObject* Object )
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
	UWidgetTree* WidgetTree = Cast<UWidgetTree>( Object );
	if( WidgetTree )
	{
		TArray<UWidget*> AllWidgets;
		WidgetTree->GetAllWidgets( AllWidgets );
		for( int i = 0; i < AllWidgets.Num(); i++ )
		{
			UWidget* Widget = AllWidgets[ i ];
			UMultiLineEditableTextBox* MultiLineEditableTextBox = Cast< UMultiLineEditableTextBox>( Widget );
			if( MultiLineEditableTextBox )
			{
				bool& bIsFontDeprecationDone = GetVariable<bool>( MultiLineEditableTextBox, TEXT( "MultiLineEditableTextBox" ), TEXT( "bIsFontDeprecationDone" ) );

				if( !bIsFontDeprecationDone && DowngraderSettings->TargetVersion <= EEngineVersion::EV_4_27 )
				{
					bIsFontDeprecationDone = true;
				}
			}
		}
	}
	UWidgetComponent* WidgetComponent = Cast<UWidgetComponent>( Object );
	if( WidgetComponent )
	{
		UWidgetComponent_Downgrader* WidgetComponent2 = (UWidgetComponent_Downgrader*)WidgetComponent;
		WidgetComponent2->GetWindowVisibility() = EWindowVisibility::Visible;
		return true;
	}
	UDirectionalLightComponent* DirectionalLightComponent = Cast< UDirectionalLightComponent >( Object );
	if( DirectionalLightComponent )
	{
		if( DowngraderSettings->TargetVersion <= EEngineVersion::EV_4_27 )
		{
			DirectionalLightComponent->bAtmosphereSunLight = DirectionalLightComponent->bUsedAsAtmosphereSunLight_DEPRECATED;
			return true;
		}
	}
	UNiagaraComponent* NiagaraComponent = Cast< UNiagaraComponent >( Object );
	if( NiagaraComponent )
	{
		bool RemovedOne = false;
		do
		{
			RemovedOne = false;
			TMap<FNiagaraVariableBase, FNiagaraVariant>& InstanceParameterOverrides =
				GetVariable<TMap<FNiagaraVariableBase, FNiagaraVariant>>( NiagaraComponent, TEXT( "NiagaraComponent" ), TEXT( "InstanceParameterOverrides" ) );
			for( auto it = InstanceParameterOverrides.CreateIterator(); it; ++it )
			{
				FNiagaraVariant& Variant = it.Value();				
				if( Variant.GetMode() == ENiagaraVariantMode::DataInterface )
				{
					UNiagaraDataInterface* Interface = Variant.GetDataInterface();
					if( Interface )
					{
						UClass* InterfaceClass = Interface->GetClass();
						//UNiagaraDataInterfaceVirtualTexture* InterfaceVirtualTexture = Cast< UNiagaraDataInterfaceVirtualTexture >( Object );
						if( InterfaceClass->GetName().Contains( TEXT( "NiagaraDataInterfaceVirtualTexture" ) ) )
						{
							if( DowngraderSettings->TargetVersion < EEngineVersion::EV_5_1 )//introduced in 5.1
							{
								InstanceParameterOverrides.Remove( it.Key() );
								RemovedOne = true;
								break;
							}
						}
					}
				}
			}
		} while( RemovedOne );
	}
	#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5
	UNavMovementComponent* NavMovementComponent = Cast< UNavMovementComponent >( Object );
	if( NavMovementComponent )
	{
		if( DowngraderSettings->TargetVersion <= EEngineVersion::EV_5_4 )
		{
			
			FNavMovementProperties& NavMovementProperties = GetVariable < FNavMovementProperties >( NavMovementComponent, TEXT( "NavMovementComponent" ), TEXT( "NavMovementProperties" ) );

			float& FixedPathBrakingDistance				= GetVariable < float >( NavMovementComponent, TEXT( "NavMovementComponent" ), TEXT( "FixedPathBrakingDistance" ) );
			bool& bUpdateNavAgentWithOwnersCollision	= GetVariable < bool >( NavMovementComponent, TEXT( "NavMovementComponent" ), TEXT( "bUpdateNavAgentWithOwnersCollision" ) );
			bool& bUseAccelerationForPaths				= GetVariable < bool >( NavMovementComponent, TEXT( "NavMovementComponent" ), TEXT( "bUseAccelerationForPaths" ) );
			bool& bUseFixedBrakingDistanceForPaths		= GetVariable < bool >( NavMovementComponent, TEXT( "NavMovementComponent" ), TEXT( "bUseFixedBrakingDistanceForPaths" ) );
			int bStopMovementAbortPaths_Offset = 332;
			//bool& bStopMovementAbortPaths				= GetVariable < bool >( NavMovementComponent, TEXT( "NavMovementComponent" ), TEXT( "bStopMovementAbortPaths" ) );

			NavMovementProperties.FixedPathBrakingDistance = FixedPathBrakingDistance;
			NavMovementProperties.bUpdateNavAgentWithOwnersCollision = bUpdateNavAgentWithOwnersCollision;
			NavMovementProperties.bUseAccelerationForPaths = bUseAccelerationForPaths;
			NavMovementProperties.bUseFixedBrakingDistanceForPaths = bUseFixedBrakingDistanceForPaths;
			//NavMovementProperties.bStopMovementAbortPaths = bStopMovementAbortPaths;
			int NavMovementProperties_bStopMovementAbortPaths_Offset = 343;
			AssignVariable< bool>( NavMovementComponent, NavMovementProperties_bStopMovementAbortPaths_Offset, NavMovementComponent, bStopMovementAbortPaths_Offset );
		}
	}
	#endif
	UGeometryCollectionComponent* GeometryCollectionComponent = Cast< UGeometryCollectionComponent >( Object );
	if( GeometryCollectionComponent )
	{
		UGeometryCollection* GeometryCollection = (UGeometryCollection*)GeometryCollectionComponent->GetRestCollection();
		FixGeometryCollection( GeometryCollection );
	}
#endif
	return false;
}
bool FixActor( AActor* Actor )
{
	TArray<UActorComponent*> Components;
	Actor->GetComponents( Components );

	bool Modified = false;
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
	ALevelInstance* LevelInstance = Cast< ALevelInstance>( Actor );
	if( LevelInstance )
	{
		TSoftObjectPtr<UWorld> WorldAsset = LevelInstance->GetWorldAsset();
		//UWorld* ActualWorld = WorldAsset.LoadSynchronous();
		auto SoftObjectPath = WorldAsset.ToSoftObjectPath();
		FTopLevelAssetPath AssetPath = SoftObjectPath.GetAssetPath();
		FName LevelAssetName = AssetPath.GetAssetName();
		FString Str = AssetPath.ToString();
		int SlashOffset = Str.Find( TEXT( "/" ), ESearchCase::IgnoreCase, ESearchDir::FromEnd );
		int HasDot = Str.Find( TEXT( "." ) );
		if( SlashOffset != INDEX_NONE && HasDot == INDEX_NONE )
		{
			SlashOffset += 1;
			FString Name = Str.Mid( SlashOffset, Str.Len() - SlashOffset );
			Str += TEXT( "." );
			Str += Name;
			//if ( ActualWorld )
				//Str += ActualWorld->GetName();

			FSoftObjectPath NewPath;
			NewPath.SetPath( Str );

			WorldAsset = TSoftObjectPtr<UWorld>( NewPath );
			LevelInstance->SetWorldAsset( WorldAsset );
			Modified = true;
		}
	}
	ALandscape* Landscape = Cast< ALandscape >( Actor );
	ALandscapeProxy* LandscapeProxy = Cast< ALandscapeProxy >( Actor );
	if( Landscape )
	{
		if( DowngraderSettings->TargetVersion < EEngineVersion::EV_5_5 )
		{
		#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5
			RemovePropertyFlag( TEXT( "Landscape" ), TEXT( "LandscapeLayers" ), CPF_Deprecated );

			TArrayView<const FLandscapeLayer> Layers = Landscape->GetLayers();
			for( int i = 0; i < Layers.Num(); i++ )
			{
				FLandscapeLayer Layer = Layers[ i ];
				if ( Layer.EditLayer != nullptr )
				{
					Landscape->EditingLayer = Layer.EditLayer->GetGuid();

					Layer.Guid_DEPRECATED = Layer.EditLayer->GetGuid();
					Layer.Name_DEPRECATED = Layer.EditLayer->GetName();
					Layer.bVisible_DEPRECATED = Layer.EditLayer->IsVisible();
					Layer.bLocked_DEPRECATED = Layer.EditLayer->IsLocked();

					float& HeightmapAlpha = GetVariable < float >( Layer.EditLayer, TEXT( "LandscapeEditLayerBase" ), TEXT( "HeightmapAlpha" ) );
					Layer.HeightmapAlpha_DEPRECATED = HeightmapAlpha;

					float& WeightmapAlpha = GetVariable < float >( Layer.EditLayer, TEXT( "LandscapeEditLayerBase" ), TEXT( "WeightmapAlpha" ) );
					Layer.WeightmapAlpha_DEPRECATED = WeightmapAlpha;

					const ULandscapeEditLayerBase* EditLayerBase = Layer.EditLayer;
					#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 6
					const TMap<TObjectPtr<ULandscapeLayerInfoObject>, bool>& WeightmapLayerAllocationBlend = EditLayerBase->GetWeightmapLayerAllocationBlend();
					for( auto it = WeightmapLayerAllocationBlend.CreateConstIterator(); it; ++it )
					{
						ULandscapeLayerInfoObject* LayerInfoObject = it.Key();
						if( LayerInfoObject )
						{
							FixTexture( LayerInfoObject->SplineFalloffModulationTexture, false );
						}
					}
					#endif
				}

				Layer.EditLayer = nullptr;
				Landscape->LandscapeLayers_DEPRECATED.Add( Layer );
			}
		#endif
		}

		#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 7
		if( DowngraderSettings->TargetVersion < EEngineVersion::EV_5_7 )
		{
			Landscape->bCanHaveLayersContent_DEPRECATED = true;
		}
		#endif
	}
	if( LandscapeProxy )
	{
		for( int32 ComponentIndex = 0; ComponentIndex < LandscapeProxy->LandscapeComponents.Num(); ComponentIndex++ )
		{
			ULandscapeComponent* Component = LandscapeProxy->LandscapeComponents[ ComponentIndex ];

			UTexture2D* HeightMap = Component->GetHeightmap();
			FixTexture( HeightMap, false );
			TArray<TObjectPtr<UTexture2D>>& WeightmapTextures = Component->GetWeightmapTextures();
			for( int i = 0; i < WeightmapTextures.Num(); i++ )
			{
				UTexture2D* Weightmap = WeightmapTextures[ i ].Get();
				FixTexture( Weightmap, false );
			}
			
			TArray<TObjectPtr<UTexture2D>>& MobileWeightmapTextures = GetVariable < TArray<TObjectPtr<UTexture2D>> >( Component, TEXT( "LandscapeComponent" ), TEXT( "MobileWeightmapTextures" ) );
			for( int i = 0; i < MobileWeightmapTextures.Num(); i++ )
			{
				UTexture2D* Weightmap = MobileWeightmapTextures[ i ];
				FixTexture( Weightmap, false );
			}

			auto LayerLambda = [&]( const FGuid& GUID, FLandscapeLayerComponentData& LayerData )
			{
				FixTexture( LayerData.HeightmapData.Texture, false );
				for( int i = 0; i < LayerData.WeightmapData.Textures.Num(); i++ )
				{
					UTexture2D* Weightmap = LayerData.WeightmapData.Textures[i];
					int NumMips = Weightmap->Source.GetNumMips();
					if( NumMips == 1 && DowngraderSettings->TargetVersion < EEngineVersion::EV_5_3 )
					{
						uint8* Mip0Ptr = Weightmap->Source.LockMip( 0 );
						int64 MipSize = Weightmap->Source.CalcMipSize( 0 );
						uint8* Mip0Backup = new uint8[ MipSize ];
						FMemory::Memcpy( Mip0Backup, Mip0Ptr, MipSize );
						Weightmap->Source.UnlockMip( 0 );

						Weightmap->Source.Init2DWithMipChain( Weightmap->Source.GetSizeX(), Weightmap->Source.GetSizeY(), Weightmap->Source.GetFormat() );
						Mip0Ptr = Weightmap->Source.LockMip( 0 );
						FMemory::Memcpy( Mip0Ptr, Mip0Backup, MipSize );
						Weightmap->Source.UnlockMip( 0 );
						delete[] Mip0Backup;

						if ( Weightmap->MipGenSettings != TMGS_LeaveExistingMips )
							Weightmap->MipGenSettings = TMGS_LeaveExistingMips;
					}
					FixTexture( Weightmap, false );
				}
			};
			Component->ForEachLayer( LayerLambda );
			Component->UpdateCachedBounds(/* bInApproximateBounds= */ false );

			TArray<UMaterialInterface*> OutMaterials;
			Component->GetUsedMaterials( OutMaterials );
			FixTexturesFromMaterialInstances( OutMaterials );
		}
		for (int32 ComponentIndex = 0; ComponentIndex < LandscapeProxy->NaniteComponents.Num(); ComponentIndex++)
		{
			ULandscapeNaniteComponent* NaniteComponent = LandscapeProxy->NaniteComponents[ComponentIndex];
			TArray<UMaterialInterface*> OutMaterials;
			NaniteComponent->GetUsedMaterials( OutMaterials );
			FixTexturesFromMaterialInstances( OutMaterials );
		}
		for( int32 ComponentIndex = 0; ComponentIndex < LandscapeProxy->CollisionComponents.Num(); ComponentIndex++ )
		{
			ULandscapeHeightfieldCollisionComponent* CollisionComponent = LandscapeProxy->CollisionComponents[ComponentIndex];
		}
		if (DowngraderSettings->TargetVersion < EEngineVersion::EV_5_3)
		{
			bool& bEnableNanite = GetVariable< bool >( LandscapeProxy, TEXT( "LandscapeProxy" ), TEXT( "bEnableNanite" ) );
			bEnableNanite = false;
			if (LandscapeProxy->NaniteComponents.Num() > 0)
			{
				RemoveNaniteComponents( LandscapeProxy );
			}
		}
	}
	AWorldPartitionHLOD* WorldPartitionHLOD = Cast< AWorldPartitionHLOD >( Actor );
	if( WorldPartitionHLOD )
	{
		if( DowngraderSettings->TargetVersion < EEngineVersion::EV_5_3 )
		{
		#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5
			const IWorldPartitionCell* Cell = WorldPartitionHLOD->GetLevel()->GetWorldPartitionRuntimeCell();
			UWorldPartition* WP = WorldPartitionHLOD->GetLevel()->GetWorldPartition();

			TObjectPtr<UWorldPartitionHLODSourceActorsFromCell>& SourceActors = GetVariable < TObjectPtr<UWorldPartitionHLODSourceActorsFromCell> >( WorldPartitionHLOD, TEXT( "WorldPartitionHLOD" ), TEXT( "SourceActors" ) );

			TArray<FWorldPartitionRuntimeCellObjectMapping>& HLODSubActors = GetVariable < TArray<FWorldPartitionRuntimeCellObjectMapping> >( WorldPartitionHLOD, TEXT( "WorldPartitionHLOD" ), TEXT( "HLODSubActors" ) );
			TObjectPtr<const UHLODLayer>& SubActorsHLODLayer = GetVariable < TObjectPtr<const UHLODLayer> >( WorldPartitionHLOD, TEXT( "WorldPartitionHLOD" ), TEXT( "SubActorsHLODLayer" ) );

			UWorldPartitionHLODSourceActorsFromCell* SourceActorsObj = SourceActors.Get();
			if( SourceActorsObj )
			{
				TArray<FWorldPartitionRuntimeCellObjectMapping>& Actors = GetVariable < TArray<FWorldPartitionRuntimeCellObjectMapping> >( SourceActorsObj, TEXT( "WorldPartitionHLODSourceActorsFromCell" ), TEXT( "Actors" ) );
				HLODSubActors.Reset( 0 );
				for( int i = 0; i < Actors.Num(); i++ )
				{
					HLODSubActors.Add( Actors[ i ] );
				}

				SubActorsHLODLayer = SourceActorsObj->GetHLODLayer();
			}
			SourceActors = nullptr;
			//FString GridName = InGridName.ToString().ToLower();
			//FGridCellCoord CellGlobalCoord = InCellGlobalCoord;
			//uint32 DataLayerID = InDataLayerID.GetHash();
			//uint32 ContentBundleID = InContentBundleID.IsValid() ? GetTypeHash( InContentBundleID ) : 0;
			//
			//FArchiveMD5 ArMD5;
			//ArMD5 << GridName << CellGlobalCoord << DataLayerID << ContentBundleID;
			//auto CellGuid = ArMD5.GetGuidFromHash();
			//WorldPartitionHLOD->SetSourceCellGuid( CellGuid );
		#endif
		}
	}
	AWorldPartitionMiniMap* WorldPartitionMiniMap = Cast< AWorldPartitionMiniMap >( Actor );
	if( WorldPartitionMiniMap )
	{
		FixTexture( WorldPartitionMiniMap->MiniMapTexture, false );
	}
	APostProcessVolume* PostProcessVolume = Cast< APostProcessVolume>( Actor );
	if (PostProcessVolume)
	{
		if (DowngraderSettings->TargetVersion < EEngineVersion::EV_5_0)
		{
			//This fixes completely white levels in 4.27
			if (PostProcessVolume->Settings.AutoExposureMaxBrightness - PostProcessVolume->Settings.AutoExposureMinBrightness < 0.1f)
			{
				PostProcessVolume->Settings.AutoExposureMinBrightness = 0.0;
				PostProcessVolume->Settings.AutoExposureMaxBrightness = 1.0;
			}
		}
	}
	for( int i = 0; i < Components.Num(); i++ )
	{
		bool FixResult = FixComponents( Components[ i ] );
		Modified = Modified || FixResult;
	}
#endif
	return Modified;
}
template<class TYPE>
bool RemoveNodeIfTypeIs( UEdGraphNode* Node, bool& Modified )
{
	TYPE* CastedNode = Cast< TYPE >( Node );
	if( CastedNode )
	{
		RemoveGraphNode( CastedNode );
		Modified = true;
		return true;
	}

	return false;
}
bool FixGraphPinType( FEdGraphPinType& PinType )
{
	bool Modified = false;
	#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 0
	if( PinType.PinCategory == UEdGraphSchema_K2::PC_Real &&
		PinType.PinSubCategory == UEdGraphSchema_K2::PC_Double )
	{
		PinType.PinCategory = UEdGraphSchema_K2::PC_Float;
		PinType.PinSubCategory = NAME_None;
		PinType.bSerializeAsSinglePrecisionFloat = true;
		Modified = true;
	}
	if( PinType.PinCategory == UEdGraphSchema_K2::PC_Real &&
		PinType.PinSubCategory == UEdGraphSchema_K2::PC_Float )
	{
		PinType.PinCategory = UEdGraphSchema_K2::PC_Float;
		PinType.PinSubCategory = NAME_None;
		Modified = true;
	}
	if( PinType.PinValueType.TerminalCategory == UEdGraphSchema_K2::PC_Real &&
		PinType.PinValueType.TerminalSubCategory == UEdGraphSchema_K2::PC_Double )
	{
		PinType.PinValueType.TerminalCategory = UEdGraphSchema_K2::PC_Float;
		PinType.PinValueType.TerminalSubCategory = NAME_None;
		PinType.bSerializeAsSinglePrecisionFloat = true;
		Modified = true;
	}
	if( PinType.PinValueType.TerminalCategory == UEdGraphSchema_K2::PC_Real &&
		PinType.PinValueType.TerminalSubCategory == UEdGraphSchema_K2::PC_Float )
	{
		PinType.PinValueType.TerminalCategory = UEdGraphSchema_K2::PC_Float;
		PinType.PinValueType.TerminalSubCategory = NAME_None;
		Modified = true;
	}
	#endif

	UObject* Obj = PinType.PinSubCategoryObject.Get();

	if( Obj )
	{
		if( Obj->GetName().Contains( TEXT( "Vector2f" ) ) )
		{
			UScriptStruct* VectorStruct = TBaseStructure<FVector2D>::Get();
			PinType.PinSubCategoryObject = VectorStruct;
			Modified = true;
		}
		if( Obj->GetName().Contains( TEXT( "Vector3f" ) ) ||
			Obj->GetName().Contains( TEXT( "NiagaraPosition" ) ))
		{
			UScriptStruct* VectorStruct = TBaseStructure<FVector>::Get();
			PinType.PinSubCategoryObject = VectorStruct;
			Modified = true;
		}
		if( Obj->GetName().Contains( TEXT( "Vector4f" ) ) )
		{
			UScriptStruct* VectorStruct = TBaseStructure<FVector4>::Get();
			PinType.PinSubCategoryObject = VectorStruct;
			Modified = true;
		}
	}

	return Modified;
}
bool ChangeDoublePropertyValueToFloat( FName VarName, UClass* BlueprintGeneratedClass )
{
	if (!BlueprintGeneratedClass)
		return false;
	FProperty* Property = FindFProperty<FProperty>( BlueprintGeneratedClass, VarName );
	if (!Property)
	{
		return false;
	}
	UObject* GeneratedCDO = BlueprintGeneratedClass->GetDefaultObject( false );
	void* ValuePtr = Property->ContainerPtrToValuePtr<void>( GeneratedCDO );
	if (!ValuePtr)
		return false;
	if (FDoubleProperty* DoubleProperty = CastField<FDoubleProperty>( Property ))
	{
		void* Value = DoubleProperty->GetPropertyValuePtr( ValuePtr );
		double* DoubleVal = (double*)Value;
		float* FloatVal = (float*)Value;
		*FloatVal = (float)(*DoubleVal);
		return true;
	}	
	return false;
}
UEdGraphPin* GetPin( UEdGraphNode* Node, FName PinName )
{
	for( int p = 0; p < Node->Pins.Num(); p++ )
	{
		UEdGraphPin* Pin = Node->Pins[ p ];
		if( Pin->PinName == PinName )
		{
			return Pin;
		}
	}

	return nullptr;
}
void AddOldNodeLinks( UEdGraphNode* OldNode, UEdGraphNode* NewNode )
{
	for (int p = 0; p < OldNode->Pins.Num(); p++)
	{
		UEdGraphPin* PinNew = nullptr;
		UEdGraphPin* PinOld = OldNode->Pins[p];
		for (int p2 = 0; p2 < NewNode->Pins.Num(); p2++)
		{
			UEdGraphPin* P = NewNode->Pins[p2];
			if (PinOld->PinName == P->PinName)
			{
				PinNew = P;
				PinNew->DefaultObject = PinOld->DefaultObject;
				PinNew->DefaultTextValue = PinOld->DefaultTextValue;
				PinNew->DefaultValue = PinOld->DefaultValue;
				//Maybe ?
				PinNew->PinId = PinOld->PinId;
				PinNew->PersistentGuid = PinOld->PersistentGuid;
				break;
			}
		}
		if (!PinNew)
			continue;

		for (int i = 0; i < PinOld->LinkedTo.Num(); i++)
		{
			UEdGraphPin* PinLink = PinOld->LinkedTo[i];
			UEdGraphNode* OwningNode = PinLink->GetOwningNode();
			if (OwningNode == OldNode)
			{
				PinLink = PinLink;
			}
			PinNew->LinkedTo.Add( PinLink );
		}
	}
}
UEdGraphPin* GetPin( UEdGraphNode* Node, FString PinName )
{
	for (int p2 = 0; p2 < Node->Pins.Num(); p2++)
	{
		UEdGraphPin* P = Node->Pins[p2];
		if (P->PinName == FName( PinName ) )
		{
			return P;
		}
	}
	return nullptr;
}
void AddLinkFromOldNode( UEdGraphNode* OldNode, UEdGraphNode* NewNode, FString OldPinName, FString NewPinName )
{
	UEdGraphPin* PinNew = GetPin( NewNode, NewPinName );
	UEdGraphPin* PinOld = GetPin( OldNode, OldPinName );
		
	if (!PinNew || !PinOld)
		return;

	for (int i = 0; i < PinOld->LinkedTo.Num(); i++)
	{
		UEdGraphPin* PinLink = PinOld->LinkedTo[i];
		PinNew->LinkedTo.Add( PinLink );
	}
}
bool ReplaceOldNodeWithNew( UEdGraphNode* OldNode, UEdGraphNode* NewNode )
{
	const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();

	bool bSuccess = false;
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3

	if( Schema && OldNode && NewNode )
	{
		TMap<FName, FName> OldToNewPinMap;
		for( UEdGraphPin* Pin : OldNode->Pins )
		{
			if( Pin->ParentPin != nullptr )
			{
				// ReplaceOldNodeWithNew() will take care of mapping split pins (as long as the parents are properly mapped)
				continue;
			}
			else if( Pin->PinName == UEdGraphSchema_K2::PN_Self )
			{
				// there's no analogous pin, signal that we're expecting this
				OldToNewPinMap.Add( Pin->PinName, NAME_None );
			}
			else
			{
				// The input pins follow the same naming scheme
				OldToNewPinMap.Add( Pin->PinName, Pin->PinName );
			}
		}

		bSuccess = Schema->ReplaceOldNodeWithNew( OldNode, NewNode, OldToNewPinMap );
		// reconstructing the node will clean up any
		// incorrect default values that may have been copied over
		NewNode->ReconstructNode();
	}

	AddOldNodeLinks( OldNode, NewNode );
#endif
	return bSuccess;
}
void FixOuterGraphReferencesForNewNode( UEdGraphNode* OldNode, UEdGraphNode* NewNode, FString OutputPinOverride = TEXT("") )
{
	UObject* NodeOuter = NewNode->GetOuter();
	UEdGraph* OuterGraph = Cast<UEdGraph>( NodeOuter );

	for (int p = 0; p < NewNode->Pins.Num(); p++)
	{
		auto& Pin = NewNode->Pins[p];
		FixGraphPinType( Pin->PinType );
	}
	int NodeIndex = OuterGraph->Nodes.IndexOfByKey( OldNode );
	OuterGraph->Nodes[NodeIndex] = NewNode;

	for (int n = 0; n < OuterGraph->Nodes.Num(); n++)
	{
		for (int p = 0; p < OuterGraph->Nodes[n]->Pins.Num(); p++)
		{
			UEdGraphPin* Pin = OuterGraph->Nodes[n]->Pins[p];

			for (int l = 0; l < Pin->LinkedTo.Num(); l++)
			{
				UEdGraphPin* OldPin = Pin->LinkedTo[l];				
				if (OldPin->GetOwningNode() == OldNode )
				{
					UEdGraphPin* NewPin = GetPin( NewNode, FName( OldPin->PinName ) );
					if (!NewPin || NewPin->Direction != OldPin->Direction)
					{
						NewPin = GetPin( NewNode, FName( OutputPinOverride ) );
					}
					//The direction check makes sure we match output pins with output pins
					if (NewPin && NewPin->Direction == OldPin->Direction )
					{
						NewPin->DefaultObject = OldPin->DefaultObject;
						NewPin->DefaultTextValue = OldPin->DefaultTextValue;
						NewPin->DefaultValue = OldPin->DefaultValue;
						Pin->LinkedTo[l] = NewPin;
					}
					else
					{
						Pin->LinkedTo.RemoveAt( l );
						l--;
					}
				}
			}
		}
	}
}
class UK2Node_CallFunction;
bool ReplaceFunctionReference( UK2Node_CallFunction* FunctionNode, FString What, FString Replace )
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
	if( FunctionNode->FunctionReference.GetMemberName().ToString().Contains( What ) )
	{
		( (FMemberReference_Downgrader*)&FunctionNode->FunctionReference )->SetMemberName( Replace );
		return true;
	}
#endif
	return false;
}
int MetaDataOffset = 320;
void FixPackage( UPackage* Package )
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 6
	if (DowngraderSettings->TargetVersion < EEngineVersion::EV_5_6)
	{
		UDEPRECATED_MetaData*& DeprecatedMetaData = GetVariable< UDEPRECATED_MetaData* >( Package, MetaDataOffset );
		UClass* Class = UDEPRECATED_MetaData::StaticClass();
		Class->ClassFlags &= ~CLASS_Deprecated;
		FString ClassName = Class->GetName();
		if (DeprecatedMetaData != nullptr)
		{
			UE_LOG( LogTemp, Warning, TEXT( "DeprecatedMetaData exists !" ) );
		}
		
		DeprecatedMetaData = NewObject<UDEPRECATED_MetaData>( Package, NAME_PackageMetaData, RF_Standalone | RF_LoadCompleted );

		FMetaData& NewMetadata = Package->GetMetaData();
		//DeprecatedMetaData->ObjectMetaDataMap = NewMetadata.ObjectMetaDataMap;
		for (auto it = NewMetadata.ObjectMetaDataMap.CreateIterator(); it; ++it)
		{
			auto Key = it.Key();
			auto Value = it.Value();
			FWeakObjectPtr Ptr = Key.TryLoad();
			DeprecatedMetaData->ObjectMetaDataMap.Add( Ptr, Value );
		}
		DeprecatedMetaData->RootMetaDataMap = NewMetadata.RootMetaDataMap;		
	}
#endif
}
void FixUserDefinedStruct( UUserDefinedStruct* UserDefinedStruct )
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
	bool Modified = false;
	if( DowngraderSettings->TargetVersion <= EEngineVersion::EV_4_27 )
	{
		TArray<FStructVariableDescription>& VariableDescriptions = FStructureEditorUtils::GetVarDesc( UserDefinedStruct );
		//First force FDoubleProperty to become FFloatProperty !
		for( int i = 0; i < VariableDescriptions.Num(); i++ )
		{
			FStructVariableDescription& VarDesc = VariableDescriptions[ i ];

			if( VarDesc.Category == UEdGraphSchema_K2::PC_Real &&
				VarDesc.SubCategory == UEdGraphSchema_K2::PC_Double )
			{
				VarDesc.SubCategory = UEdGraphSchema_K2::PC_Float;
				Modified = true;
			}
		}

		if( Modified )
		{
			FStructureEditorUtils::CompileStructure( UserDefinedStruct );
		}

		VariableDescriptions = FStructureEditorUtils::GetVarDesc( UserDefinedStruct );
		for( int i = 0; i < VariableDescriptions.Num(); i++ )
		{
			FStructVariableDescription& VarDesc = VariableDescriptions[ i ];

			if( VarDesc.Category == UEdGraphSchema_K2::PC_Real &&
				VarDesc.SubCategory == UEdGraphSchema_K2::PC_Double )
			{
				VarDesc.Category = UEdGraphSchema_K2::PC_Float;
				VarDesc.SubCategory = NAME_None;
				Modified = true;
			}
			if( VarDesc.Category == UEdGraphSchema_K2::PC_Real &&
				VarDesc.SubCategory == UEdGraphSchema_K2::PC_Float )
			{
				VarDesc.Category = UEdGraphSchema_K2::PC_Float;
				VarDesc.SubCategory = NAME_None;
				Modified = true;
			}
		}
	}
#endif
}
void FinishAllCompilation()
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 0
	FAssetCompilingManager::Get().FinishAllCompilation();
#endif
}
void CollectAnimSequencesSafe( const UObject* Object, TSet<const UAnimSequence*>& OutSequences, TSet<const UObject*>& Visited )
{
	if (!Object || Visited.Contains( Object ))
	{
		return; // Eliminate already visited objects to prevent cycles
	}
	Visited.Add( Object );

	for (TFieldIterator<FProperty> PropIt( Object->GetClass() ); PropIt; ++PropIt)
	{
		FProperty* Property = *PropIt;
		const void* ValuePtr = Property->ContainerPtrToValuePtr<void>( Object );

		if (FObjectProperty* ObjProp = CastField<FObjectProperty>( Property ))
		{
			UObject* RefObj = ObjProp->GetObjectPropertyValue( ValuePtr );
			if (const UAnimSequence* AnimSeq = Cast<UAnimSequence>( RefObj ))
			{
				OutSequences.Add( AnimSeq );
			}
			else
			{
				CollectAnimSequencesSafe( RefObj, OutSequences, Visited );
			}
		}
		else if (FArrayProperty* ArrayProp = CastField<FArrayProperty>( Property ))
		{
			FScriptArrayHelper ArrayHelper( ArrayProp, ValuePtr );
			FProperty* InnerProp = ArrayProp->Inner;

			for (int32 i = 0; i < ArrayHelper.Num(); ++i)
			{
				void* ElemPtr = ArrayHelper.GetRawPtr( i );
				if (FObjectProperty* InnerObjProp = CastField<FObjectProperty>( InnerProp ))
				{
					UObject* RefObj = InnerObjProp->GetObjectPropertyValue( ElemPtr );
					if (const UAnimSequence* AnimSeq = Cast<UAnimSequence>( RefObj ))
					{
						OutSequences.Add( AnimSeq );
					}
					else
					{
						CollectAnimSequencesSafe( RefObj, OutSequences, Visited );
					}
				}
			}
		}
		// Optionally handle FMapProperty, FSetProperty, FStructProperty similarly.
	}
}
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
class UK2Node_MacroInstance_Downgrader : public UK2Node_MacroInstance
{
public:
	TArray<UEdGraphPin*>& GetWildcardPins()
	{
		return WildcardPins;
	}
};
#endif
void FixBlueprint( UBlueprint* Blueprint )
{	
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3

	bool Modified = false;
	TArray< UEdGraphNode*> AllNodes = GetAllBlueprintNodes( Blueprint );
	for( int i = 0; i < AllNodes.Num(); i++ )
	{
		UEdGraphNode* Node = AllNodes[ i ];
		UClass* NodeClass = Node->GetClass();
		
		#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 6
		if (DowngraderSettings->TargetVersion < EEngineVersion::EV_5_6)
		{
			UK2Node_MacroInstance* MacroInstance = Cast< UK2Node_MacroInstance >( Node );
			if (MacroInstance)
			{
				TArray<UEdGraphPin*>& WildcardPins = ((UK2Node_MacroInstance_Downgrader*)MacroInstance)->GetWildcardPins();
				if (WildcardPins.Num() > 0)
				{
					MacroInstance->ResolvedWildcardType = WildcardPins[0]->PinType;
				}
			}
			UK2Node_VariableGet* VariableGet = Cast< UK2Node_VariableGet>( Node );
			if ( VariableGet )
			{
				bool& bIsPureGet_DEPRECATED = GetVariable< bool >( Node, TEXT( "K2Node_VariableGet" ), TEXT( "bIsPureGet" ) );
				EGetNodeVariation& CurrentVariation = GetVariable< EGetNodeVariation >( Node, TEXT( "K2Node_VariableGet" ), TEXT( "CurrentVariation" ) );

				if ( CurrentVariation == EGetNodeVariation::ValidatedObject )
				{
					bIsPureGet_DEPRECATED = false;
				}
			}
		}
		#endif
		#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4
		if( DowngraderSettings->TargetVersion < EEngineVersion::EV_5_4 )
		{
			RemoveNodeIfTypeIs< UAnimGraphNode_Steering >( Node, Modified );

			//UAnimGraphNode_RigLogic* NodeControlRig = Cast<UAnimGraphNode_RigLogic>( Node );
			if( NodeClass->GetName().Contains( TEXT("AnimGraphNode_RigLogic")))
			{
				for( int p = 0; p < Node->Pins.Num(); p++ )
				{
					UEdGraphPin* Pin = Node->Pins[p];
					if( Pin->GetName().Contains( TEXT( "LODThreshold" ) ) )
					{
						//Disconnect it
						Pin->LinkedTo.Reset();
					}
				}
			}
		}
		#endif
		if( DowngraderSettings->TargetVersion < EEngineVersion::EV_5_1 )
		{
			RemoveNodeIfTypeIs< UAnimGraphNode_FootPlacement >( Node, Modified );
			RemoveNodeIfTypeIs< UAnimGraphNode_OffsetRootBone >( Node, Modified );
		}
		if( DowngraderSettings->TargetVersion <= EEngineVersion::EV_4_27 )
		{			
			RemoveNodeIfTypeIs< UAnimGraphNode_OrientationWarping >( Node, Modified );
			RemoveNodeIfTypeIs< UAnimGraphNode_SlopeWarping >( Node, Modified );
			RemoveNodeIfTypeIs< UAnimGraphNode_StrideWarping >( Node, Modified );
			
			for( int p = 0; p < Node->Pins.Num(); p++ )
			{
				UEdGraphPin* Pin = Node->Pins[ p ];
				if( Pin->GetName().Contains( TEXT( "Double" ) ) )
				{
					FString NewName = Pin->GetName().Replace( TEXT( "Double" ), TEXT( "Float" ) );
					Pin->PinName = FName( NewName );
				}
				Modified = FixGraphPinType( Pin->PinType );				
			}

			UK2Node_EditablePinBase* EditablePinBase = Cast< UK2Node_EditablePinBase >( Node );
			if( EditablePinBase )
			{
				for( int p = 0; p < EditablePinBase->UserDefinedPins.Num(); p++ )
				{
					auto& Pin = EditablePinBase->UserDefinedPins[ p ];
					if( Pin.IsValid() )
					{
						Modified = FixGraphPinType( Pin->PinType );
					}
				}
			}
			UK2Node_FunctionEntry* FunctionEntryNode = Cast< UK2Node_FunctionEntry >( Node );
			if( FunctionEntryNode )
			{
				for( int p = 0; p < FunctionEntryNode->LocalVariables.Num(); p++ )
				{
					auto& LocalVariable = FunctionEntryNode->LocalVariables[ p ];
					Modified = FixGraphPinType( LocalVariable.VarType );
				}
			}
			UK2Node_FunctionResult* FunctionResult = Cast< UK2Node_FunctionResult >( Node );
			if( FunctionResult )
			{
				for( int p = 0; p < FunctionResult->UserDefinedPins.Num(); p++ )
				{
					auto& Pin = FunctionResult->UserDefinedPins[ p ];
					if( Pin.IsValid() )
					{
						Modified = FixGraphPinType( Pin->PinType );
					}
				}
			}
			UK2Node_CallFunction* FunctionNode = Cast< UK2Node_CallFunction>( Node );
			if( FunctionNode )
			{
				//Functions that doesn't exist in 4.27 in UKismetArrayLibrary
				if( FunctionNode->FunctionReference.GetMemberName().ToString().Contains( TEXT( "Array_IsEmpty" ) ) ||
					FunctionNode->FunctionReference.GetMemberName().ToString().Contains( TEXT( "Array_IsNotEmpty" ) ))
				{
					RemoveGraphNode( FunctionNode );
				}
				if( FunctionNode->FunctionReference.GetMemberName().ToString().Contains( TEXT( "Add_DoubleDouble" ) ) )
				{
					( (FMemberReference_Downgrader*)&FunctionNode->FunctionReference )->SetMemberName( TEXT( "Add_FloatFloat" ) );
					Modified = true;
				}
				if( FunctionNode->FunctionReference.GetMemberName().ToString().Contains( TEXT( "Subtract_DoubleDouble" ) ) )
				{
					( (FMemberReference_Downgrader*)&FunctionNode->FunctionReference )->SetMemberName( TEXT( "Subtract_FloatFloat" ) );
				}
				if( FunctionNode->FunctionReference.GetMemberName().ToString().Contains( TEXT( "Multiply_DoubleDouble" ) ) )
				{
					( (FMemberReference_Downgrader*)&FunctionNode->FunctionReference )->SetMemberName( TEXT( "Multiply_FloatFloat" ) );
					Modified = true;
				}
				if( FunctionNode->FunctionReference.GetMemberName().ToString().Contains( TEXT( "Divide_DoubleDouble" ) ) )
				{
					( (FMemberReference_Downgrader*)&FunctionNode->FunctionReference )->SetMemberName( TEXT( "Divide_FloatFloat" ) );
				}
				if( FunctionNode->FunctionReference.GetMemberName().ToString().Contains( TEXT( "Less_DoubleDouble" ) ) )
				{
					( (FMemberReference_Downgrader*)&FunctionNode->FunctionReference )->SetMemberName( TEXT( "Less_FloatFloat" ) );
				}
				if( FunctionNode->FunctionReference.GetMemberName().ToString().Contains( TEXT( "Greater_DoubleDouble" ) ) )
				{
					( (FMemberReference_Downgrader*)&FunctionNode->FunctionReference )->SetMemberName( TEXT( "Greater_FloatFloat" ) );
				}
				if( FunctionNode->FunctionReference.GetMemberName().ToString().Contains( TEXT( "LessEqual_DoubleDouble" ) ) )
				{
					( (FMemberReference_Downgrader*)&FunctionNode->FunctionReference )->SetMemberName( TEXT( "LessEqual_FloatFloat" ) );
				}
				if( FunctionNode->FunctionReference.GetMemberName().ToString().Contains( TEXT( "GreaterEqual_DoubleDouble" ) ) )
				{
					( (FMemberReference_Downgrader*)&FunctionNode->FunctionReference )->SetMemberName( TEXT( "GreaterEqual_FloatFloat" ) );
				}
				if( FunctionNode->FunctionReference.GetMemberName().ToString().Contains( TEXT( "EqualEqual_DoubleDouble" ) ) )
				{
					( (FMemberReference_Downgrader*)&FunctionNode->FunctionReference )->SetMemberName( TEXT( "EqualEqual_FloatFloat" ) );
				}
				if( FunctionNode->FunctionReference.GetMemberName().ToString().Contains( TEXT( "NotEqual_DoubleDouble" ) ) )
				{
					( (FMemberReference_Downgrader*)&FunctionNode->FunctionReference )->SetMemberName( TEXT( "NotEqual_FloatFloat" ) );
				}
				Modified = ReplaceFunctionReference( FunctionNode, TEXT( "Conv_DoubleToString" ), TEXT( "Conv_FloatToString" ) ) || Modified;
				Modified = ReplaceFunctionReference( FunctionNode, TEXT( "Conv_StringToDouble" ), TEXT( "Conv_StringToFloat" ) ) || Modified;
				Modified = ReplaceFunctionReference( FunctionNode, TEXT( "BuildString_Double" ), TEXT( "BuildString_Float" ) ) || Modified;
				
				//Modified = ReplaceFunctionReference( FunctionNode, TEXT( "GetSkeletalMeshAsset" ), TEXT( "BuildString_Float" ) ) || Modified;

				UClass* FunctionFromClass = FunctionNode->FunctionReference.GetMemberParentClass();
				if ( FunctionFromClass )
				{
					if( FunctionFromClass->GetOuter()->GetName().Contains( TEXT( "/Script/GeometryScriptingCore" ) ) ||
						FunctionFromClass->GetOuter()->GetName().Contains( TEXT( "/Script/GeometryFramework" ) ) ||
						FunctionFromClass->GetOuter()->GetName().Contains( TEXT( "/Script/GeometryScriptingEditor" ) )
						)
					{
						RemoveGraphNode( FunctionNode );
					}
				}

				#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5
				if (FunctionNode->FunctionReference.GetMemberName().ToString().Contains( TEXT( "GetSkeletalMeshAsset" ) ))
				{
					UObject* NodeOuter = Node->GetOuter();
					UK2Node_VariableGet* NodeFor427 = NewObject<UK2Node_VariableGet>( NodeOuter );

					FMemberReference_Downgrader* MemberReference = (FMemberReference_Downgrader*)&NodeFor427->VariableReference;

					MemberReference->SetMemberName( TEXT( "SkeletalMesh" ) );
					TObjectPtr<UObject> Class = USkinnedMeshComponent::StaticClass();
					MemberReference->SetMemberParent( Class );
					NodeFor427->NodePosX = Node->NodePosX;
					NodeFor427->NodePosY = Node->NodePosY;

					NodeFor427->CreateNewGuid();
					//NodeFor427->SetFlags( RF_Transactional );
					NodeFor427->AllocateDefaultPins();
					NodeFor427->PostPlacedNewNode();
					
					bool& bSelfContext = GetVariable< bool >( &NodeFor427->VariableReference, TEXT("MemberReference"), TEXT( "bSelfContext" ) );
					bSelfContext = false;
					AddOldNodeLinks( Node, NodeFor427 );
					AddLinkFromOldNode( Node, NodeFor427, TEXT( "ReturnValue" ), TEXT( "SkeletalMesh" ) );
					FixOuterGraphReferencesForNewNode( Node, NodeFor427, TEXT("SkeletalMesh") );

					//Remove from graph
					Node->Rename( nullptr, GetTransientPackage() );
					Modified = true;
				}
				#endif
			}
			//UK2Node_PromotableOperator doesn't exist in 4.27
			UK2Node_PromotableOperator* PromotableOperator = Cast< UK2Node_PromotableOperator>( Node );
			if( PromotableOperator )
			{
				UObject* NodeOuter = Node->GetOuter();
				UEdGraph* OuterGraph = Cast<UEdGraph>( NodeOuter );
				UK2Node_CallFunction* OpNodeFor427 = nullptr;
				//if( PromotableOperator->GetOperationName().ToString().Contains( "EqualEqual" ) )
				{
					OpNodeFor427 = NewObject<UK2Node_CallFunction>( NodeOuter );
				}
				//else
					//OpNodeFor427 = NewObject<UK2Node_CommutativeAssociativeBinaryOperator>( NodeOuter );

				FMemberReference_Downgrader* FunctionReferenceSource = (FMemberReference_Downgrader*)&PromotableOperator->FunctionReference;
				FMemberReference_Downgrader* FunctionReferenceTarget = (FMemberReference_Downgrader*)&OpNodeFor427->FunctionReference;
				FunctionReferenceTarget->SetMemberName( PromotableOperator->FunctionReference.GetMemberName().ToString() );
				FunctionReferenceTarget->SetMemberParent( FunctionReferenceSource->GetMemberParent() );
				OpNodeFor427->bIsPureFunc = 1;
				ReplaceOldNodeWithNew( Node, OpNodeFor427 );
				FunctionReferenceTarget->SetMemberName( PromotableOperator->FunctionReference.GetMemberName().ToString() );

				for( int s = 0; s < PromotableOperator->Pins.Num(); s++ )
				{
					auto& SourcePin = PromotableOperator->Pins[ s ];
					
					FName SourcePinName = SourcePin->GetFName();
					UEdGraphPin* TargetPin = GetPin( OpNodeFor427, SourcePinName );
					if( !TargetPin && SourcePinName != FName( TEXT( "self" )))// &&
						//SourcePinName != FName( TEXT( "ReturnValue_X" )) &&
						//SourcePinName != FName( TEXT( "ReturnValue_Y" ))&&
						//SourcePinName != FName( TEXT( "ReturnValue_Z" )))
					{
						UEdGraphPin* NewPin = FWildcardNodeUtils::CreateWildcardPin( OpNodeFor427, SourcePinName, EGPD_Input );

						NewPin->PinType = SourcePin->PinType;
						NewPin->PinId = SourcePin->PinId;
						NewPin->Direction = SourcePin->Direction;
						#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 6
							NewPin->bIsDiffing = SourcePin->bIsDiffing;
						#endif
						NewPin->AutogeneratedDefaultValue = SourcePin->AutogeneratedDefaultValue;

						//Add original links
						for( int l = 0; l < SourcePin->LinkedTo.Num(); l++ )
						{
							NewPin->LinkedTo.Add( SourcePin->LinkedTo[ l ] );
						}
					}
				}
				
				FixOuterGraphReferencesForNewNode( Node, OpNodeFor427 );

				//Remove from graph
				RemoveGraphNode( Node );//->Rename( nullptr, GetTransientPackage() );
				Modified = true;
				continue;
			}			
			UAnimGraphNode_Base* AnimNode = Cast< UAnimGraphNode_Base>( Node );
			if( AnimNode )
			{
				UAnimGraphNode_StateMachine* StateMachine = Cast< UAnimGraphNode_StateMachine>( Node );
				if( StateMachine )
				{
					UEdGraph* StateMachineGraph = StateMachine->EditorStateMachineGraph.Get();
					for( int n = 0; n < StateMachineGraph->Nodes.Num(); n++ )
					{
						UEdGraphNode* StateMachineNode = StateMachineGraph->Nodes[ n ];
						UAnimStateAliasNode* StateAliasNode = Cast< UAnimStateAliasNode >( StateMachineNode );
						if( StateAliasNode )
						{
							UEdGraph* Graph = StateAliasNode->GetGraph();
							RemoveGraphNode( StateAliasNode );
						}
					}
				}
			}
			//UK2Node_PropertyAccess* PropertyAccess = Cast< UK2Node_PropertyAccess>( Node );
			if( NodeClass->GetName().Contains(TEXT("PropertyAccess")))
			{
				static int ResolvedPinTypeOffset = 232;
				//FEdGraphPinType& PinType = (FEdGraphPinType&)PropertyAccess->GetResolvedPinType();
				FEdGraphPinType& ResolvedPinType = GetVariable< FEdGraphPinType >( Node, ResolvedPinTypeOffset );
				Modified = FixGraphPinType( ResolvedPinType );
			}
			UK2Node_MacroInstance* MacroInstance = Cast< UK2Node_MacroInstance >( Node );
			if( MacroInstance )
			{
				Modified = FixGraphPinType( MacroInstance->ResolvedWildcardType );
			}
			UK2Node_GetArrayItem* GetArrayItem = Cast< UK2Node_GetArrayItem >( Node );
			if( GetArrayItem )
			{
				FixGeneratedPinLinksFor427( GetArrayItem );
			}
			UK2Node_GenericCreateObject* GenericCreateObject = Cast< UK2Node_GenericCreateObject >( Node );
			if( GenericCreateObject )
			{
				if( GenericCreateObject->Pins.Num() > 4 && GenericCreateObject->Pins[ 4 ]->PinName.ToString().Contains( "self" ) )
				{
					GenericCreateObject->Pins[ 4 ]->PinName = FName( TEXT( "Outer" ) );
					Modified = true;
				}
			}
		}
	}

	TArray< UEdGraph* > AllGraphs = GetAllGraphs( Blueprint );
	for( int i = 0; i < AllGraphs.Num(); i++ )
	{
		UEdGraph* Graph = AllGraphs[ i ];
	#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4
		UAnimationBlendStackGraph* AnimationBlendStackGraph = Cast<UAnimationBlendStackGraph>( Graph );
		if( AnimationBlendStackGraph )
		{
			//FObjectDuplicationParameters DuplicationParameters( Graph, Graph->GetOuter() );
			//DuplicationParameters.DestClass = UAnimationGraph::StaticClass();
			//UObject* DupGraph = StaticDuplicateObjectEx( DuplicationParameters );
			UEdGraph* ParentGraph = UEdGraph::GetOuterGraph( Graph );
			if( ParentGraph )
			{
				ParentGraph->SubGraphs.Remove( Graph );
			}
			//UClass* Replacement = UAnimationGraph::StaticClass();
			//static int Offset = 16;
			//TObjectPtr<UClass>& ClassPrivate = GetVariable< TObjectPtr<UClass> >( AnimationBlendStackGraph, Offset );
			//ClassPrivate = Replacement;
			//Graph->Schema = UAnimationGraphSchema::StaticClass();
		}
	#endif
	}
	
	for (int i = 0; i < Blueprint->Extensions.Num(); i++)
	{
		UBlueprintExtension* Extension = Blueprint->Extensions[i];
	}
	if( DowngraderSettings->TargetVersion <= EEngineVersion::EV_4_27 )
	{
		//Extensions are now forwarded to base class in AddRedirects
		//Blueprint->RemoveAllExtension( []( UBlueprintExtension* InExtension )
		//{
		//	return true;
		//} );

		if( Blueprint->ParentClass )
		{
			if( Blueprint->ParentClass->GetName().Contains( TEXT( "PackedLevelActor" ) ) ||
				Blueprint->ParentClass->GetName().Contains( TEXT( "DynamicMeshActor" ) )
				)
			{
				Blueprint->ParentClass = AActor::StaticClass();
				Modified = true;
			}
		}
	}
	bool RemoveCachedVMCode = true;
	if( RemoveCachedVMCode && Blueprint->GeneratedClass )
	{
		TArray<UObject*> ClassSubObjects;
		UClass* BlueprintClass = Blueprint->GeneratedClass;
		UBlueprintGeneratedClass* BlueprintGeneratedClass = Cast< UBlueprintGeneratedClass>( BlueprintClass );
		if( BlueprintGeneratedClass )
		{
			//Only needed for 4.27, putting it here in case I move it
			if( DowngraderSettings->TargetVersion <= EEngineVersion::EV_4_27 )
			{
				BlueprintGeneratedClass->bIsSparseClassDataSerializable = 0;
			}
		}
		GetObjectsWithOuter( BlueprintClass, ClassSubObjects, false );


		{
			// Save subobjects, that won't be regenerated.
			//FSubobjectCollection SubObjectsToSave;
			//SaveSubObjectsFromCleanAndSanitizeClass( SubObjectsToSave, ClassToClean );
			//
			//ClassSubObjects.RemoveAllSwap( SubObjectsToSave );
		}

		//UClass* InheritableComponentHandlerClass = UInheritableComponentHandler::StaticClass();

		UObject* CDO = BlueprintGeneratedClass->GetDefaultObject();
		AActor* CDOActor = Cast<AActor>( CDO );
		if( CDOActor )
		{
			TArray<UActorComponent*> Components;
			CDOActor->GetComponents( Components );
			for( int i = 0; i < Components.Num(); i++ )
			{
				UDynamicMeshComponent* DynamicMeshComponent = Cast<UDynamicMeshComponent>( Components[ i ] );
				if( DynamicMeshComponent )
				{
					if( DowngraderSettings->TargetVersion <= EEngineVersion::EV_4_27 )
					{
						DynamicMeshComponent->DestroyComponent();
					}
				}

				ClassSubObjects.Add( Components[ i ] );
			}
		}
		for( int i = 0; i < ClassSubObjects.Num(); i++)
		{
			UObject* CurrSubObj = ClassSubObjects[ i ];
			//if( Cast<UInheritableComponentHandler>( CurrSubObj ) || CurrSubObj->IsInA( InheritableComponentHandlerClass ) || CurrSubObj->HasAnyFlags( RF_InheritableComponentTemplate ) )
			//{
			//	continue;
			//}
			FixComponents( CurrSubObj );
			if( UFunction* Function = Cast<UFunction>( CurrSubObj ) )
			{
				Function->Script.Empty();
				Function->ScriptAndPropertyObjectReferences.Empty();
				Function->DestroyChildPropertiesAndResetPropertyLinks();
				Function->StaticLink(/*bRelinkExistingProperties =*/ true );
			}
		}
		UActorComponent* ActorComponent = Cast<UActorComponent>( CDO );
		if ( ActorComponent )
		{
			FixComponents( ActorComponent );
		}
		//I wonder why is this here ? It interfeers when I need to reparent the class because of missing classes in 4.27
		//Modified = false;
	}
	UAnimBlueprint* AnimBlueprint = Cast< UAnimBlueprint>( Blueprint );
	if ( AnimBlueprint )
	{
		if( DowngraderSettings->TargetVersion <= EEngineVersion::EV_5_0 )
		{
			//Crashes if I don't clear them because it tries to allocate
			AnimBlueprint->PoseWatches.Reset( 0 );
		}
		UAnimBlueprintGeneratedClass* AnimGeneratedClass = Cast<UAnimBlueprintGeneratedClass>( Blueprint->GeneratedClass );
		if (AnimGeneratedClass)
		{
		}
	}
	if( Modified )
	{
		Blueprint->PostEditChange();
		Blueprint->MarkPackageDirty();

		FCompilerResultsLog LogResults;
		LogResults.SetSourcePath( Blueprint->GetPathName() );
		LogResults.BeginEvent( TEXT( "Compile" ) );
		LogResults.bLogDetailedResults = true;
		LogResults.EventDisplayThresholdMs = true;
		EBlueprintCompileOptions CompileOptions = EBlueprintCompileOptions::None;

		FKismetEditorUtilities::CompileBlueprint( Blueprint, CompileOptions, &LogResults );
		FinishAllCompilation();
	}
	
	//Change type and data here because compile above reallocates pins and allocates the wrong classes for 5.7+
	if (DowngraderSettings->TargetVersion <= EEngineVersion::EV_4_27)
	{
		for (int i = 0; i < Blueprint->NewVariables.Num(); i++)
		{
			FBPVariableDescription& Var = Blueprint->NewVariables[i];
			FEdGraphPinType NewPinType = Var.VarType;
			Modified = FixGraphPinType( NewPinType );
			if (Modified)
			{
				Var.VarType = NewPinType;
				ChangeDoublePropertyValueToFloat( Var.VarName, Blueprint->GeneratedClass );
			}
		}
	}
	if (AnimBlueprint)
	{
		TSet<const UAnimSequence*> OutSequences;
		TSet<const UObject*> Visited;
		CollectAnimSequencesSafe( AnimBlueprint, OutSequences, Visited );

		UAnimBlueprintGeneratedClass* AnimGeneratedClass = Cast<UAnimBlueprintGeneratedClass>( Blueprint->GeneratedClass );
		if (AnimGeneratedClass)
		{
			int MutablePropertiesOffset = 2184;
			TArray<FProperty*>& MutableProperties = GetVariable< TArray<FProperty*>>( AnimGeneratedClass, MutablePropertiesOffset );
			for (int i = 0; i < MutableProperties.Num(); i++)
			{
				FProperty* Property = MutableProperties[i];

				UObject* GeneratedCDO = AnimGeneratedClass->GetDefaultObject( false );
				//FAnimBlueprintMutableData * MutableData = AnimGeneratedClass->GetMutableNodeData(GeneratedCDO);

				//void* ValuePtr = Property->ContainerPtrToValuePtr<void>( GeneratedCDO );
				//if (ValuePtr != nullptr)
				//{
				//	if (FObjectProperty* ObjectProperty = CastField<FObjectProperty>( Property ))
				//	{
				//		void* ObjectValue = ObjectProperty->GetPropertyValuePtr( ValuePtr );
				//		if (ObjectValue)
				//		{
				//
				//		}
				//	}
				//}
			}
			//if (MutableProperties.Num() > 0)
			//{
			//	MutableProperties.Reset( 0 );
			//}
		}
	}
	
	/*UEditorUtilityWidgetBlueprint* EUWB = Cast< UEditorUtilityWidgetBlueprint>(Blueprint);
	if( EUWB )
	{
		TArray<UWidget*> AllWidgets;
		EUWB->WidgetTree.Get()->GetAllWidgets( AllWidgets );
		for( int i = 0; i < AllWidgets.Num(); i++ )
		{
			UWidget* Widget = AllWidgets[ i ];
			UEditableTextBox* EditableTextBox = Cast< UEditableTextBox>( Widget );
			if( EditableTextBox )
			{
				FTextBlockStyle& TextStyle = EditableTextBox->WidgetStyle.TextStyle;
				TextStyle.SetFont( EditableTextBox->WidgetStyle.Font_DEPRECATED );
			}
		}
	}*/
#endif
}
void FixRigVMNode( UControlRigBlueprint* ControlRigBlueprint, URigVMNode* Node )
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
	int ReferencedNodePtrOffset = GetMemberOffset( TEXT( "RigVMFunctionReferenceNode" ), TEXT( "ReferencedNodePtr" ) );
	int ReferencedFunctionHeaderOffset = GetMemberOffset( TEXT( "RigVMFunctionReferenceNode" ), TEXT( "ReferencedFunctionHeader" ) );

	if (!ControlRigBlueprint->GetPackage()->GetName().StartsWith( "/Temp/" ))
	{
		if (URigVMFunctionReferenceNode* FunctionReferenceNode = Cast<URigVMFunctionReferenceNode>( Node ))
		{
			TSoftObjectPtr<URigVMLibraryNode>& ReferencedNodePtr = GetVariable< TSoftObjectPtr<URigVMLibraryNode>>( FunctionReferenceNode, ReferencedNodePtrOffset );
			FRigVMGraphFunctionHeader& ReferencedFunctionHeader = GetVariable< FRigVMGraphFunctionHeader>( FunctionReferenceNode, ReferencedFunctionHeaderOffset );
			//FunctionReferenceNode->ReferencedNodePtr_DEPRECATED = FunctionReferenceNode->ReferencedFunctionHeader.LibraryPointer.LibraryNode;
			//AssignVariable( FunctionReferenceNode, ReferencedNodePtrOffset, ReferencedFunctionHeader.LibraryPointer.LibraryNode );

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5
			if (!ReferencedFunctionHeader.LibraryPointer.LibraryNode_DEPRECATED.IsValid())
				ReferencedFunctionHeader.LibraryPointer.LibraryNode_DEPRECATED = ReferencedFunctionHeader.LibraryPointer.GetLibraryNodePath();

			if (!ReferencedNodePtr.Get())
				ReferencedNodePtr = ReferencedFunctionHeader.LibraryPointer.LibraryNode_DEPRECATED;
#else
			ReferencedNodePtr = ReferencedFunctionHeader.LibraryPointer.LibraryNode;
#endif
		}
	}
	URigVMAggregateNode* AggregateNode = Cast<URigVMAggregateNode>( Node );
	if (AggregateNode)
	{
		//URigVMEdGraph* EdGraph = Cast<URigVMEdGraph>( GraphToValidate );
		//FRigVMAssetInterfacePtr Blueprint = FRigVMBlueprintUtils::FindAssetForGraph( EdGraph );
		//UScriptStruct* StructTemplate = nullptr;
		//FName MethodName = FName( TEXT("Execute") );
		//FVector2D Location = AggregateNode->GetPosition() + FVector2D( 50, 0 );
		//URigVMEdGraphNode * NewNode = URigVMEdGraphUnitNodeSpawner::SpawnNode( EdGraph, Blueprint, StructTemplate, MethodName, Location );
		URigVMUnitNode* NewNode = NewObject<URigVMUnitNode>();
	}
	URigVMCollapseNode* CollapseNode = Cast<URigVMCollapseNode>( Node );
	if (CollapseNode)
	{
		const TArray<URigVMNode*>& InnerNodes = CollapseNode->GetContainedNodes();
		for (int i = 0; i < InnerNodes.Num(); i++)
		{
			URigVMNode* InnerNode = InnerNodes[i];
			FixRigVMNode( ControlRigBlueprint, InnerNode );
		}
	}
	if (URigVMUnitNode* StructNode = Cast<URigVMUnitNode>( Node ))
	{
		TObjectPtr<UScriptStruct>& ScriptStructDeprecated = GetVariable< TObjectPtr<UScriptStruct>>( StructNode, TEXT( "RigVMUnitNode" ), TEXT( "ScriptStruct" ) );
		FName& MethodNameDeprecated = GetVariable< FName>( StructNode, TEXT( "RigVMUnitNode" ), TEXT( "MethodName" ) );

		UScriptStruct* ScriptStruct = StructNode->GetScriptStruct();
		ScriptStructDeprecated = ScriptStruct;

		FName MethodName = StructNode->GetMethodName();
		MethodNameDeprecated = MethodName;
	}

	const TArray<URigVMPin*>& Pins = Node->GetPins();
	for (int p = 0; p < Pins.Num(); p++)
	{
		URigVMPin* Pin = (URigVMPin*)Pins[p];
		const TArray<URigVMInjectionInfo*> InjectedNodes = Pin->GetInjectedNodes();
		for (int j = 0; j < InjectedNodes.Num(); j++)
		{
			URigVMInjectionInfo* Info = InjectedNodes[j];
			Info->UnitNode_DEPRECATED = Cast< URigVMUnitNode >( Info->Node );
			if (!Info->UnitNode_DEPRECATED)
			{
				UE_LOG( LogTemp, Warning, TEXT( "UnitNode_DEPRECATED cast failed !" ) );
			}
		}

		if (DowngraderSettings->TargetVersion < EEngineVersion::EV_5_0)
		{
			FString BaseCPPType = Pin->IsArray() ? Pin->GetArrayElementCppType() : Pin->GetCPPType();
			if (BaseCPPType.Compare( TEXT( "double" ) ) == 0)
			{
				FString& CPPType = GetVariable< FString>( Pin, TEXT( "RigVMPin" ), TEXT( "CPPType" ) );
				CPPType = TEXT( "float" );
				UObject* Check = Pin->GetCPPTypeObject();
			}
		}

		if (DowngraderSettings->TargetVersion < EEngineVersion::EV_5_6)
		{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 6
			FName DisplayName = Pin->GetDisplayName();
			if (Pin->GetName().Equals( TEXT( "ExecutePin" ) ))
			{
				Pin->Rename( TEXT( "ExecuteContext" ), Pin->GetOuter() );
				static int CachedPinPathOffset = 392;
				TRigVMModelCachedValue<URigVMPin, FString>& CachedPinPath = GetVariable< TRigVMModelCachedValue<URigVMPin, FString> >( Pin, CachedPinPathOffset );
				CachedPinPath.ResetCachedValue();
			}
			const TArray<URigVMLink*>& Links = Pin->GetLinks();
			for (int l = 0; l < Links.Num(); l++)
			{
				URigVMLink* Link = (URigVMLink*)Links[l];
				FString& SourcePinPath = GetVariable< FString>( Link, TEXT( "RigVMLink" ), TEXT( "SourcePinPath" ) );
				FString& TargetPinPath = GetVariable< FString>( Link, TEXT( "RigVMLink" ), TEXT( "TargetPinPath" ) );

				if (SourcePinPath.Contains( "ExecutePin" ))
				{
					SourcePinPath = SourcePinPath.Replace( TEXT( "ExecutePin" ), TEXT( "ExecuteContext" ) );
				}
				if (TargetPinPath.Contains( "ExecutePin" ))
				{
					TargetPinPath = TargetPinPath.Replace( TEXT( "ExecutePin" ), TEXT( "ExecuteContext" ) );
				}
			}
#endif
		}
	}	
#endif
}
void FixControlRigBlueprint( UControlRigBlueprint* ControlRigBlueprint )
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
	RemovePropertyFlag( TEXT( "RigVMFunctionReferenceNode" ), TEXT( "ReferencedNodePtr" ), CPF_Deprecated );
	RemovePropertyFlag( TEXT( "RigVMUnitNode" ), TEXT( "ScriptStruct" ), CPF_Deprecated );
	RemovePropertyFlag( TEXT( "RigVMUnitNode" ), TEXT( "MethodName" ), CPF_Deprecated );
	AddPropertyFlag( TEXT( "ControlRig" ), TEXT( "DataSourceRegistry" ), CPF_Deprecated );

	TArray<URigVMGraph*> GraphsToValidate = ControlRigBlueprint->GetAllModels();
	//TArray<UEdGraph*> EdGraphs;
	//A duplicate graph, weird...
	//ControlRigBlueprint->GetAllEdGraphs( EdGraphs );
	for( int32 GraphIndex = 0; GraphIndex < GraphsToValidate.Num(); GraphIndex++ )
	{
		URigVMGraph* GraphToValidate = GraphsToValidate[ GraphIndex ];
		if( GraphToValidate == nullptr )
		{
			continue;
		}

		URigVMController* Controller = ControlRigBlueprint->GetController( GraphToValidate );
		if (Controller)
		{
		}
		const TArray<URigVMNode*>& Nodes = GraphToValidate->GetNodes();
		for( int i=0; i<Nodes.Num(); i++ )
		{
			URigVMNode* Node = Nodes[i];
			FixRigVMNode( ControlRigBlueprint, Node );

			if (DowngraderSettings->TargetVersion <= EEngineVersion::EV_4_27)
			{
				if (Node->GetNodeTitle().Contains( TEXT( "Full Body IK" ) ))
				{
					//Destroy
					bool Removed = Controller->RemoveNode( Node );
					if (!Removed)
					{
						UE_LOG( LogTemp, Log, TEXT( "[Downgrader] Controller->RemoveNode '%s' failed !" ), *Node->GetName() );
					}
					//RemoveRigGraphNode( Node );
				}
			}
		}
	}
	
	for (int i = 0; i < ControlRigBlueprint->ShapeLibraries.Num(); i++)
	{
		TSoftObjectPtr<UControlRigShapeLibrary> Library = ControlRigBlueprint->ShapeLibraries[i];
		auto Path = Library.ToSoftObjectPath();
		auto Str = Path.GetAssetPath().GetAssetName().ToString();

		if (DowngraderSettings->TargetVersion <= EEngineVersion::EV_4_27)
		{
			if (Str.Contains( TEXT( "DefaultGizmoLibrary" ) ))
			{
				ControlRigBlueprint->ShapeLibraries[i].Reset();
			}
		}
	}
	if( DowngraderSettings->TargetVersion <= EEngineVersion::EV_4_27 )
	{
		//Hierarchy is deprecated in 4.27 and now in 5.6 it reappeared as not deprecated and with a new typename
		AddPropertyFlag( TEXT( "ControlRigBlueprint" ), TEXT( "Hierarchy" ), CPF_Deprecated );

		for( int i = 0; i < ControlRigBlueprint->FunctionGraphs.Num(); i++ )
		{
			UEdGraph* Graph = ControlRigBlueprint->FunctionGraphs[ i ];
			//UEdGraphSchema_K2* FunctionGraphSchema = Cast<UEdGraphSchema_K2>( Graph->Schema );
			UClass* TheClass = Graph->Schema.Get();
			if( TheClass != UEdGraphSchema_K2::StaticClass() )
			{
				Graph->Schema = UEdGraphSchema_K2::StaticClass();
			}
		}
		#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4
		if (ControlRigBlueprint->GetHierarchy())
		{
			#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 6
			TArray<FRigBone>& Bones = GetVariable< TArray<FRigBone>>( &ControlRigBlueprint->HierarchyContainer_DEPRECATED.BoneHierarchy, TEXT( "RigBoneHierarchy" ), TEXT( "Bones" ) );
			Bones.Reset( 0 );
			URigHierarchy* Hierarchy = ControlRigBlueprint->GetHierarchy();
			auto Elements = Hierarchy->GetFilteredElements<FRigBaseElement>( [Hierarchy]( const FRigBaseElement* Element )
				{
					return true;
				} );
			for (int i = 0; i < Elements.Num(); i++)
			{
				FRigBaseElement* Element = Elements[i];
				if (Element->GetKey().Type == ERigElementType::Bone)
				{
					FRigBoneElement* BoneElement = (FRigBoneElement*)Element;
					FName ParentName = NAME_None;
					if (BoneElement->ParentElement)
						ParentName = BoneElement->ParentElement->GetFName();
					ERigBoneType BoneType = BoneElement->BoneType;
					FRigCurrentAndInitialTransform& Transform = BoneElement->GetTransform();
					ControlRigBlueprint->HierarchyContainer_DEPRECATED.BoneHierarchy.Add(
						Element->GetFName(), ParentName, BoneType, Transform.Initial.Local.Get(),
						Transform.Current.Local.Get(), Transform.Initial.Global.Get() );
				}
			}
			#endif
		}
		#endif
	}
#endif
}
TArray<UMaterialExpression*> GetExpressions( UMaterial* Material )
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
	TArray<UMaterialExpression*> Expressions;
	if( !Material )
		return Expressions;
	TConstArrayView<TObjectPtr<UMaterialExpression>> ConstExpressions = Material->GetExpressions();	
	Expressions.Reserve( ConstExpressions.Num() );
	for( int i = 0; i < ConstExpressions.Num(); i++ )
	{
		Expressions.Add( ConstExpressions[ i ] );
	}
	return Expressions;
#else
	return Material->Expressions;
#endif
}
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3

void FixMaterialLayersFunctions( FMaterialLayersFunctions& MaterialLayersFunctions, FMaterialLayersFunctionsEditorOnlyData& SourceLayersFunctions )
{
	TArray<bool>& LayerStates = GetVariable< TArray<bool>>( &MaterialLayersFunctions, TEXT( "MaterialLayersFunctions" ), TEXT( "LayerStates" ) );
	TArray<FText>& LayerNames = GetVariable< TArray<FText>>( &MaterialLayersFunctions, TEXT( "MaterialLayersFunctions" ), TEXT( "LayerNames" ) );
	TArray<FGuid>& LayerGuids = GetVariable< TArray<FGuid>>( &MaterialLayersFunctions, TEXT( "MaterialLayersFunctions" ), TEXT( "LayerGuids" ) );
	TArray<bool>& RestrictToLayerRelatives = GetVariable< TArray<bool>>( &MaterialLayersFunctions, TEXT( "MaterialLayersFunctions" ), TEXT( "RestrictToLayerRelatives" ) );
	TArray<bool>& RestrictToBlendRelatives = GetVariable< TArray<bool>>( &MaterialLayersFunctions, TEXT( "MaterialLayersFunctions" ), TEXT( "RestrictToBlendRelatives" ) );
	TArray<EMaterialLayerLinkState>& LayerLinkStates = GetVariable< TArray<EMaterialLayerLinkState>>( &MaterialLayersFunctions, TEXT( "MaterialLayersFunctions" ), TEXT( "LayerLinkStates" ) );
	TArray<FGuid>& DeletedParentLayerGuids = GetVariable< TArray<FGuid>>( &MaterialLayersFunctions, TEXT( "MaterialLayersFunctions" ), TEXT( "DeletedParentLayerGuids" ) );

	for( int i = 0; i < SourceLayersFunctions.LayerStates.Num(); i++ )
	{
		LayerStates.Add( SourceLayersFunctions.LayerStates[ i ] );
	}
	for( int i = 0; i < SourceLayersFunctions.LayerNames.Num(); i++ )
	{
		LayerNames.Add( SourceLayersFunctions.LayerNames[ i ] );
	}
	for( int i = 0; i < SourceLayersFunctions.LayerGuids.Num(); i++ )
	{
		LayerGuids.Add( SourceLayersFunctions.LayerGuids[ i ] );
	}
	for( int i = 0; i < SourceLayersFunctions.RestrictToLayerRelatives.Num(); i++ )
	{
		RestrictToLayerRelatives.Add( SourceLayersFunctions.RestrictToLayerRelatives[ i ] );
	}
	for( int i = 0; i < SourceLayersFunctions.RestrictToBlendRelatives.Num(); i++ )
	{
		RestrictToBlendRelatives.Add( SourceLayersFunctions.RestrictToBlendRelatives[ i ] );
	}
	for( int i = 0; i < SourceLayersFunctions.LayerLinkStates.Num(); i++ )
	{
		LayerLinkStates.Add( SourceLayersFunctions.LayerLinkStates[ i ] );
	}
	for( int i = 0; i < SourceLayersFunctions.DeletedParentLayerGuids.Num(); i++ )
	{
		DeletedParentLayerGuids.Add( SourceLayersFunctions.DeletedParentLayerGuids[ i ] );
	}
}
void FixMaterialLayersFunctions( FMaterialLayersFunctions& MaterialLayersFunctions, FMaterialLayersFunctions& SourceLayersFunctions )
{
	TArray<bool>& LayerStates = GetVariable< TArray<bool>>( &MaterialLayersFunctions, TEXT( "MaterialLayersFunctions" ), TEXT( "LayerStates" ) );
	TArray<FText>& LayerNames = GetVariable< TArray<FText>>( &MaterialLayersFunctions, TEXT( "MaterialLayersFunctions" ), TEXT( "LayerNames" ) );
	TArray<FGuid>& LayerGuids = GetVariable< TArray<FGuid>>( &MaterialLayersFunctions, TEXT( "MaterialLayersFunctions" ), TEXT( "LayerGuids" ) );
	TArray<bool>& RestrictToLayerRelatives = GetVariable< TArray<bool>>( &MaterialLayersFunctions, TEXT( "MaterialLayersFunctions" ), TEXT( "RestrictToLayerRelatives" ) );
	TArray<bool>& RestrictToBlendRelatives = GetVariable< TArray<bool>>( &MaterialLayersFunctions, TEXT( "MaterialLayersFunctions" ), TEXT( "RestrictToBlendRelatives" ) );
	TArray<EMaterialLayerLinkState>& LayerLinkStates = GetVariable< TArray<EMaterialLayerLinkState>>( &MaterialLayersFunctions, TEXT( "MaterialLayersFunctions" ), TEXT( "LayerLinkStates" ) );
	TArray<FGuid>& DeletedParentLayerGuids = GetVariable< TArray<FGuid>>( &MaterialLayersFunctions, TEXT( "MaterialLayersFunctions" ), TEXT( "DeletedParentLayerGuids" ) );
	
	for( int i = 0; i < SourceLayersFunctions.EditorOnly.LayerStates.Num(); i++ )
	{
		LayerStates.Add( SourceLayersFunctions.EditorOnly.LayerStates[ i ] );
	}
	for( int i = 0; i < SourceLayersFunctions.EditorOnly.LayerNames.Num(); i++ )
	{
		LayerNames.Add( SourceLayersFunctions.EditorOnly.LayerNames[i]);
	}
	for( int i = 0; i < SourceLayersFunctions.EditorOnly.LayerGuids.Num(); i++ )
	{
		LayerGuids.Add( SourceLayersFunctions.EditorOnly.LayerGuids[ i ] );
	}
	for( int i = 0; i < SourceLayersFunctions.EditorOnly.RestrictToLayerRelatives.Num(); i++ )
	{
		RestrictToLayerRelatives.Add( SourceLayersFunctions.EditorOnly.RestrictToLayerRelatives[ i ] );
	}
	for( int i = 0; i < SourceLayersFunctions.EditorOnly.RestrictToBlendRelatives.Num(); i++ )
	{
		RestrictToBlendRelatives.Add( SourceLayersFunctions.EditorOnly.RestrictToBlendRelatives[ i ] );
	}
	for( int i = 0; i < SourceLayersFunctions.EditorOnly.LayerLinkStates.Num(); i++ )
	{
		LayerLinkStates.Add( SourceLayersFunctions.EditorOnly.LayerLinkStates[ i ] );
	}
	for( int i = 0; i < SourceLayersFunctions.EditorOnly.DeletedParentLayerGuids.Num(); i++ )
	{
		DeletedParentLayerGuids.Add( SourceLayersFunctions.EditorOnly.DeletedParentLayerGuids[ i ] );
	}
}
FExpressionInput* GetMaterialAttributes( UMaterial* Material )
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >=1
	return &Material->GetEditorOnlyData()->MaterialAttributes;
#else
	return &Material->OpacityMask;
#endif
}
FScalarMaterialInput* GetOpacity( UMaterial* Material )
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >=1
	return &Material->GetEditorOnlyData()->Opacity;
#else
	return &Material->Opacity;
#endif
}
FScalarMaterialInput* GetOpacityMask( UMaterial* Material )
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >=1
	return &Material->GetEditorOnlyData()->OpacityMask;
#else
	return &Material->OpacityMask;
#endif
}
FColorMaterialInput* GetBaseColor( UMaterial* Material )
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >=1
	return &Material->GetEditorOnlyData()->BaseColor;
#else
	return &Material->BaseColor;
#endif
}
FColorMaterialInput* GetEmissiveColor( UMaterial* Material )
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >=1
	return &Material->GetEditorOnlyData()->EmissiveColor;
#else
	return &Material->EmissiveColor;
#endif
}
FExpressionInput* GetNormal( UMaterial* Material )
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >=1
	return &Material->GetEditorOnlyData()->Normal;
#else
	return &Material->Normal;
#endif
}
FExpressionInput* GetMetallic( UMaterial* Material )
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >=1
	return &Material->GetEditorOnlyData()->Metallic;
#else
	return &Material->Metallic;
#endif
}
FExpressionInput* GetRoughness( UMaterial* Material )
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >=1
	return &Material->GetEditorOnlyData()->Roughness;
#else
	return &Material->Roughness;
#endif
}
FExpressionInput* GetSpecular( UMaterial* Material )
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >=1
	return &Material->GetEditorOnlyData()->Specular;
#else
	return &Material->Specular;
#endif
}
FExpressionInput* GetAnisotropy( UMaterial* Material )
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >=1
	return &Material->GetEditorOnlyData()->Anisotropy;
#elif (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION > 20) || ENGINE_MAJOR_VERSION >= 5
	return &Material->Anisotropy;
#else
	return nullptr;
#endif
}
FExpressionInput* GetTangent( UMaterial* Material )
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >=1
	return &Material->GetEditorOnlyData()->Tangent;
#elif (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION > 20) || ENGINE_MAJOR_VERSION >= 5
	return &Material->Tangent;
#else
	return nullptr;
#endif
}
FExpressionInput* GetWorldPositionOffset( UMaterial* Material )
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >=1
	return &Material->GetEditorOnlyData()->WorldPositionOffset;
#else
	return &Material->WorldPositionOffset;
#endif
}
FExpressionInput* GetSubsurfaceColor( UMaterial* Material )
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >=1
	return &Material->GetEditorOnlyData()->SubsurfaceColor;
#else
	return &Material->SubsurfaceColor;
#endif
}
FExpressionInput* GetClearCoat( UMaterial* Material )
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >=1
	return &Material->GetEditorOnlyData()->ClearCoat;
#else
	return &Material->ClearCoat;
#endif
}
FExpressionInput* GetClearCoatRoughness( UMaterial* Material )
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >=1
	return &Material->GetEditorOnlyData()->ClearCoatRoughness;
#else
	return &Material->ClearCoatRoughness;
#endif
}
FExpressionInput* GetAmbientOcclusion( UMaterial* Material )
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >=1
	return &Material->GetEditorOnlyData()->AmbientOcclusion;
#else
	return &Material->AmbientOcclusion;
#endif
}
FExpressionInput* GetRefraction( UMaterial* Material )
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >=1
	return &Material->GetEditorOnlyData()->Refraction;
#else
	return &Material->Refraction;
#endif
}
FExpressionInput* GetPixelDepthOffset( UMaterial* Material )
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >=1
	return &Material->GetEditorOnlyData()->PixelDepthOffset;
#else
	return &Material->PixelDepthOffset;
#endif
}
FExpressionInput* GetShadingModelFromMaterialExpression( UMaterial* Material )
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >=1
	return &Material->GetEditorOnlyData()->ShadingModelFromMaterialExpression;
#else
	#if (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 26) || ENGINE_MAJOR_VERSION >= 5
		return &Material->ShadingModelFromMaterialExpression;
	#else
		return nullptr;
	#endif
#endif
}
FExpressionInput* GetCustomizedUVs( UMaterial* Material, int i)
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >=1
	return &Material->GetEditorOnlyData()->CustomizedUVs[i];
#else
	return &Material->CustomizedUVs[i];
#endif
}
void GetOutputExpression( FExpressionInput* MaterialInput, UMaterialExpression* Source, int OutputIndex, TArray< FExpressionInput*>& Inputs )
{
	if( !MaterialInput )
		return;
	if( MaterialInput->Expression == Source && MaterialInput->OutputIndex == OutputIndex )
		Inputs.Add( MaterialInput );
}
TArray<FExpressionInput*> GetInputs( UMaterialExpression* Exp )
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
	return TArray<FExpressionInput*>{ Exp->GetInputsView() };
#else
	return Exp->GetInputs();
#endif
}

void GetOutputExpression( UMaterial* Material, UMaterialExpression* Source, int OutputIndex, TArray< FExpressionInput*>& Inputs )
{
	TArray<UMaterialExpression*> AllExpressions = GetExpressions( Material );

	for( int i = 0; i < AllExpressions.Num(); i++ )
	{
		UMaterialExpression* Exp = AllExpressions[ i ];

		TArray<FExpressionInput*> ExpInputs = GetInputs( Exp );
		for( int u = 0; u < ExpInputs.Num(); u++ )
		{
			UMaterialExpression* A = ExpInputs[ u ]->Expression;
			if( A == Source && ExpInputs[ u ]->OutputIndex == OutputIndex )
			{
				Inputs.Add( ExpInputs[ u ] );
			}
		}
	}

	GetOutputExpression( GetBaseColor( Material ), Source, OutputIndex, Inputs );
	GetOutputExpression( GetMetallic( Material ), Source, OutputIndex, Inputs );
	GetOutputExpression( GetSpecular( Material ), Source, OutputIndex, Inputs );
	GetOutputExpression( GetRoughness( Material ), Source, OutputIndex, Inputs );
	GetOutputExpression( GetAnisotropy( Material ), Source, OutputIndex, Inputs );
	GetOutputExpression( GetNormal( Material ), Source, OutputIndex, Inputs );
	GetOutputExpression( GetTangent( Material ), Source, OutputIndex, Inputs );
	GetOutputExpression( GetEmissiveColor( Material ), Source, OutputIndex, Inputs );
	GetOutputExpression( GetOpacity( Material ), Source, OutputIndex, Inputs );
	GetOutputExpression( GetOpacityMask( Material ), Source, OutputIndex, Inputs );

	GetOutputExpression( GetWorldPositionOffset( Material ), Source, OutputIndex, Inputs );
#if ENGINE_MAJOR_VERSION == 4
	GetOutputExpression( &Material->WorldDisplacement, Source, OutputIndex, Inputs );
	GetOutputExpression( &Material->TessellationMultiplier, Source, OutputIndex, Inputs );
#endif
	GetOutputExpression( GetSubsurfaceColor( Material ), Source, OutputIndex, Inputs );
	GetOutputExpression( GetClearCoat( Material ), Source, OutputIndex, Inputs );
	GetOutputExpression( GetClearCoatRoughness( Material ), Source, OutputIndex, Inputs );
	GetOutputExpression( GetAmbientOcclusion( Material ), Source, OutputIndex, Inputs );
	GetOutputExpression( GetRefraction( Material ), Source, OutputIndex, Inputs );

	GetOutputExpression( GetMaterialAttributes( Material ), Source, OutputIndex, Inputs );

	for( int i = 0; i < 8; i++ )
		GetOutputExpression( GetCustomizedUVs( Material, i ), Source, OutputIndex, Inputs );

	GetOutputExpression( GetPixelDepthOffset( Material ), Source, OutputIndex, Inputs );
	GetOutputExpression( GetShadingModelFromMaterialExpression( Material ), Source, OutputIndex, Inputs );
}
const TArray<UMaterialExpression*> GetFunctionExpressions( UMaterialFunctionInterface* MaterialFunction)
{
	#if ENGINE_MAJOR_VERSION >= 5
		TArray<UMaterialExpression*> Ret;

		#if ENGINE_MINOR_VERSION >= 1
			TConstArrayView<TObjectPtr<UMaterialExpression>> FunctionExpressions = MaterialFunction->GetExpressions();

			for( int i = 0; i < FunctionExpressions.Num(); i++ )
			{
				Ret.Add( FunctionExpressions[ i ] );
			}
		#else
			const TArray<TObjectPtr<UMaterialExpression>>* FunctionExpressions = MaterialFunction->GetFunctionExpressions();
		
			for( int i = 0; i < FunctionExpressions->Num(); i++ )
			{
				Ret.Add( ( *FunctionExpressions )[ i ] );
			}
		#endif
		return Ret;
	#else
		return *MaterialFunction->GetFunctionExpressions();
	#endif
}
void GetOutputExpression( UMaterialFunction* MaterialFunction, UMaterialExpression* Source, int OutputIndex, TArray< FExpressionInput*>& Inputs )
{
	const TArray<UMaterialExpression*> AllExpressions = GetFunctionExpressions( MaterialFunction );

	for( int i = 0; i < AllExpressions.Num(); i++ )
	{
		UMaterialExpression* Exp = AllExpressions[ i ];

		TArray<FExpressionInput*> ExpInputs = GetInputs( Exp );
		for( int u = 0; u < ExpInputs.Num(); u++ )
		{
			UMaterialExpression* A = ExpInputs[ u ]->Expression;
			if( A == Source && ExpInputs[ u ]->OutputIndex == OutputIndex )
			{
				Inputs.Add( ExpInputs[ u ] );
			}
		}
	}
}
void GetOutputExpression( UMaterial* Material, UMaterialFunction* MaterialFunction, UMaterialExpression* Source, int OutputIndex, TArray< FExpressionInput*>& Inputs )
{
	if( Material )
	{
		GetOutputExpression( Material, Source, OutputIndex, Inputs );
	}
	else if( MaterialFunction )
	{
		GetOutputExpression( MaterialFunction, Source, OutputIndex, Inputs );
	}
}
FString AddInputForSwitch( float Case, FString SwitchValue, FString Value, int Mode = 0 )
{
	FString Prefix = TEXT( "else" );
	if( Mode == 0 )
		Prefix = TEXT( "if" );
	if( Mode == 1 )
		Prefix = TEXT( "else if" );
	FString Ret;
	if( Mode == 1 || Mode == 0 )
	{
		Ret = FString::Printf(
			TEXT( "(%s - eps <= %f && %s + eps >= %f)\n"
				  "{\n"
				  "		return %s;\n"
				  "}\n" ),
			*SwitchValue, Case, *SwitchValue, Case, *Value );
	}
	else
	{
		Ret = FString::Printf(
			TEXT( "{\n"
				  "		return %s;\n"
				  "}\n" ),
			*Value );
	}

	return Prefix + Ret;
}
int FixRemovedOutputIndex( TArray<TObjectPtr< UMaterialExpression>>& Expressions, UMaterialExpression* TargetExpression, int TargetOutputIndex )
{
	int Fixes = 0;
	for (int i = 0; i < Expressions.Num(); i++)
	{
		UMaterialExpression* Exp = Expressions[i].Get();
		auto Inputs = GetInputs( Exp );
		for (int t = 0; t < Inputs.Num(); t++)
		{
			FExpressionInput* Input = Inputs[t];
			if (Input->Expression == TargetExpression && Input->OutputIndex >= TargetOutputIndex)
			{
				Input->OutputIndex--;
				Fixes++;
			}
		}
	}

	return Fixes;
}
void GetExpressionConnections( TArray<TObjectPtr< UMaterialExpression>>& Expressions, UMaterialExpression* TargetExpression, TArray< FExpressionInput*>& Connections,
							   int SpecificOutput )
{
	int Fixes = 0;
	for( int i = 0; i < Expressions.Num(); i++ )
	{
		UMaterialExpression* Exp = Expressions[ i ].Get();
		auto Inputs = GetInputs( Exp );
		for( int t = 0; t < Inputs.Num(); t++ )
		{
			FExpressionInput* Input = Inputs[ t ];
			if( Input->Expression == TargetExpression && Input->OutputIndex == SpecificOutput )
			{
				Connections.Add( Input );
			}
		}
	}
}
int GetNumComponentsFromVectorType( ECustomMaterialOutputType Type )
{
	switch (Type)
	{
		default:
		case CMOT_Float1: return 1;
		case CMOT_Float2: return 2;
		case CMOT_Float3: return 3;
		case CMOT_Float4: return 4;
	}
}
ECustomMaterialOutputType NumComponentsToVectorType( int Num )
{
	switch (Num)
	{
	default:
	case 1: return CMOT_Float1;
	case 2: return CMOT_Float2;
	case 3: return CMOT_Float3;
	case 4: return CMOT_Float4;
	}
}
class UMaterialExpressionReroute_UTG : public UMaterialExpressionReroute
{
public:
	virtual bool GetRerouteInput( FExpressionInput& OutInput ) const
	{
		return UMaterialExpressionReroute::GetRerouteInput( OutInput );
	}
};
class UMaterialExpressionNamedRerouteDeclaration_UTG : public UMaterialExpressionNamedRerouteDeclaration
{
public:
	virtual bool GetRerouteInput( FExpressionInput& OutInput ) const
	{
		return UMaterialExpressionNamedRerouteDeclaration::GetRerouteInput( OutInput );
	}
};
class UMaterialExpressionNamedRerouteUsage_UTG : public UMaterialExpressionNamedRerouteUsage
{
public:
	virtual bool GetRerouteInput( FExpressionInput& OutInput ) const
	{
		return UMaterialExpressionNamedRerouteUsage::GetRerouteInput( OutInput );
	}
};
ECustomMaterialOutputType EvaluateExpressionInputType( FExpressionInput& Input )
{
	UMaterialExpression* Expression = Input.Expression;
	if( !Expression )
		return ECustomMaterialOutputType::CMOT_Float1;

	UMaterialExpressionScalarParameter* ScalarParameter = Cast<UMaterialExpressionScalarParameter>( Expression );
	if( ScalarParameter )
		return ECustomMaterialOutputType::CMOT_Float1;
	UMaterialExpressionVectorParameter* VectorParameter = Cast<UMaterialExpressionVectorParameter>( Expression );
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 0
	UMaterialExpressionDoubleVectorParameter* DoubleVectorParameter = Cast<UMaterialExpressionDoubleVectorParameter>( Expression );
#endif
	UMaterialExpressionTextureSample* TextureSample = Cast<UMaterialExpressionTextureSample>( Expression );
	UMaterialExpressionTextureObject* TextureObject = Cast<UMaterialExpressionTextureObject>( Expression );
	UMaterialExpressionTextureObjectParameter* TextureObjectParameter = Cast<UMaterialExpressionTextureObjectParameter>( Expression );
	if( VectorParameter
	#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 0
		|| DoubleVectorParameter
	#endif
		|| ( TextureSample && !TextureObjectParameter ) )
	{
		if( Input.OutputIndex == 0 )
			return ECustomMaterialOutputType::CMOT_Float3;
		if( Input.OutputIndex == 5 )
			return ECustomMaterialOutputType::CMOT_Float4;
		else
			return ECustomMaterialOutputType::CMOT_Float1;
	}
	UMaterialExpressionDotProduct* DotProduct = Cast<UMaterialExpressionDotProduct>( Expression );
	UMaterialExpressionDistance* Distance = Cast<UMaterialExpressionDistance>( Expression );
	UMaterialExpressionConstant* Constant = Cast<UMaterialExpressionConstant>( Expression );
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 0
	UMaterialExpressionConstantDouble* ConstantDouble = Cast<UMaterialExpressionConstantDouble>( Expression );
#endif
	if( Constant ||
	#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 0
		ConstantDouble ||
	#endif
		DotProduct || Distance )
	{
		return ECustomMaterialOutputType::CMOT_Float1;
	}
	UMaterialExpressionConstant2Vector* Constant2Vector = Cast<UMaterialExpressionConstant2Vector>( Expression );
	UMaterialExpressionTextureCoordinate* Texcoords = Cast<UMaterialExpressionTextureCoordinate>( Expression );
	if( Constant2Vector || Texcoords )
	{
		return ECustomMaterialOutputType::CMOT_Float2;
	}
	UMaterialExpressionConstant3Vector* Constant3Vector = Cast<UMaterialExpressionConstant3Vector>( Expression );
	UMaterialExpressionNormalize* Normalize = Cast<UMaterialExpressionNormalize>( Expression );
	UMaterialExpressionCameraVectorWS* CameraVectorWS = Cast<UMaterialExpressionCameraVectorWS>( Expression );
	UMaterialExpressionCameraPositionWS* CameraPositionWS = Cast<UMaterialExpressionCameraPositionWS>( Expression );
	UMaterialExpressionWorldPosition* WorldPosition = Cast<UMaterialExpressionWorldPosition>( Expression );
	UMaterialExpressionPixelNormalWS* PixelNormalWS = Cast<UMaterialExpressionPixelNormalWS>( Expression );
	UMaterialExpressionVertexNormalWS* VertexNormalWS = Cast<UMaterialExpressionVertexNormalWS>( Expression );
	UMaterialExpressionPreSkinnedNormal* PreSkinnedNormal = Cast<UMaterialExpressionPreSkinnedNormal>( Expression );
	if( Constant3Vector || Normalize || CameraVectorWS || CameraPositionWS || WorldPosition || PixelNormalWS || VertexNormalWS || PreSkinnedNormal )
	{
		return ECustomMaterialOutputType::CMOT_Float3;
	}
	UMaterialExpressionConstant4Vector* Constant4Vector = Cast<UMaterialExpressionConstant4Vector>( Expression );
	if( Constant4Vector )
	{
		return ECustomMaterialOutputType::CMOT_Float4;
	}
	UMaterialExpressionAppendVector* AppendVector = Cast<UMaterialExpressionAppendVector>( Expression );
	if( AppendVector )
	{
		//GodotShaderVariableType TypeA = EvaluateExpressionInputType( AppendVector->A );
		//GodotShaderVariableType TypeB = EvaluateExpressionInputType( AppendVector->B );
		//
		//int ComponentsA = GetNumComponentsFromVectorType( TypeA );
		//int ComponentsB = GetNumComponentsFromVectorType( TypeB );
		//
		//int TotalComponents = FMath::Min( ComponentsA + ComponentsB, 4 );
		//GodotShaderVariableType Ret = NumComponentsToVectorType( TotalComponents );
		//return Ret;
	}
	UMaterialExpressionCustom* Custom = Cast<UMaterialExpressionCustom>( Expression );
	if (Custom)
	{
		if (Input.OutputIndex == 0)
		{
			return Custom->OutputType;
		}
		else if (Input.OutputIndex <= Custom->AdditionalOutputs.Num())
		{
			FCustomOutput& CustomOutput = Custom->AdditionalOutputs[Input.OutputIndex - 1];
			return CustomOutput.OutputType;
		}
	}
	UMaterialExpressionComponentMask* ComponentMask = Cast<UMaterialExpressionComponentMask>( Expression );
	if (ComponentMask)
	{
		//std::string swizzle;
		//int Components = GetComponentMaskSwizzleAndNumComponents( FVector4( ComponentMask->R, ComponentMask->G, ComponentMask->B, ComponentMask->A ), swizzle );
		//GodotShaderVariableType Type = NumComponentsToVectorType( Components );
		//return Type;
	}
	UMaterialExpressionMakeMaterialAttributes* MakeMaterialAttributes = Cast<UMaterialExpressionMakeMaterialAttributes>( Expression );
	UMaterialExpressionBreakMaterialAttributes* BreakMaterialAttributes = Cast<UMaterialExpressionBreakMaterialAttributes>( Expression );
	UMaterialExpressionSetMaterialAttributes* SetMaterialAttributes = Cast<UMaterialExpressionSetMaterialAttributes>( Expression );
	UMaterialExpressionGetMaterialAttributes* GetMaterialAttributes = Cast<UMaterialExpressionGetMaterialAttributes>( Expression );
	UMaterialExpressionBlendMaterialAttributes* BlendMaterialAttributes = Cast<UMaterialExpressionBlendMaterialAttributes>( Expression );
	if (MakeMaterialAttributes || SetMaterialAttributes || BlendMaterialAttributes)
	{
		return ECustomMaterialOutputType::CMOT_MaterialAttributes;
	}
	if (GetMaterialAttributes)
	{
		//if( Input.OutputIndex == 0 )
		//	return ECustomMaterialOutputType::CMOT_MaterialAttributes;
		//
		//GodotShaderVariableType Type = GetPropertyType( GetMaterialAttributes, Input.OutputIndex - 1 );
		//return Type;
	}
	if (BreakMaterialAttributes)
	{
		if (Input.OutputIndex >= 1 && Input.OutputIndex <= 4)
			return ECustomMaterialOutputType::CMOT_Float1;
		else
			return ECustomMaterialOutputType::CMOT_Float3;
	}
	UMaterialExpressionFunctionInput* FunctionInput = Cast<UMaterialExpressionFunctionInput>( Expression );
	if (FunctionInput)
	{
		switch (FunctionInput->InputType)
		{
		default://break;
		case FunctionInput_Scalar: return ECustomMaterialOutputType::CMOT_Float1;
		case FunctionInput_Vector2: return ECustomMaterialOutputType::CMOT_Float2;
		case FunctionInput_Vector3: return ECustomMaterialOutputType::CMOT_Float3;
		case FunctionInput_Vector4: return ECustomMaterialOutputType::CMOT_Float4;
			//case FunctionInput_MaterialAttributes:return GSVT_TRANSFORM;
		}
	}
	UClass* ExpressionClass = Expression->GetClass();
	if (ExpressionClass && ExpressionClass->GetName().Contains( TEXT( "DistanceToNearestSurface" ) ))
	{
		return ECustomMaterialOutputType::CMOT_Float1;
	}

	UMaterialExpressionNamedRerouteDeclaration* NamedRerouteDeclaration = Cast<UMaterialExpressionNamedRerouteDeclaration>( Expression );
	if (NamedRerouteDeclaration)
	{
		FExpressionInput OutRouting;
		((UMaterialExpressionNamedRerouteDeclaration_UTG*)NamedRerouteDeclaration)->GetRerouteInput( OutRouting );
		return EvaluateExpressionInputType( OutRouting );
	}
	UMaterialExpressionNamedRerouteUsage* NamedRerouteUsage = Cast<UMaterialExpressionNamedRerouteUsage>( Expression );
	if (NamedRerouteUsage)
	{
		FExpressionInput OutRouting;
		((UMaterialExpressionNamedRerouteUsage_UTG*)NamedRerouteUsage)->GetRerouteInput( OutRouting );
		return EvaluateExpressionInputType( OutRouting );
	}
	UMaterialExpressionReroute* Reroute = Cast<UMaterialExpressionReroute>( Expression );
	if (Reroute)
	{
		FExpressionInput OutRouting;
		((UMaterialExpressionReroute_UTG*)Reroute)->GetRerouteInput( OutRouting );
		return EvaluateExpressionInputType( OutRouting );
	}

	int NumComponents = 0;
	const TArray<FExpressionInput*> Inputs = GetInputs( Expression );
	for (int i = 0; i < Inputs.Num(); i++)
	{
		FExpressionInput* LocalInput = Inputs[i];
		ECustomMaterialOutputType Type = EvaluateExpressionInputType( *LocalInput );
		int Components = GetNumComponentsFromVectorType( Type );
		NumComponents = FMath::Max( Components, NumComponents );
	}

	ECustomMaterialOutputType Ret = NumComponentsToVectorType( NumComponents );

	return Ret;
}
void ReconnectNewNode( UMaterial* ParentMaterial, UMaterialFunction* ParentMaterialFunction, UMaterialExpression* OldExp, UMaterialExpression* NewExp, int NumOutputs )
{
	for (int i = 0; i < NumOutputs; i++)
	{
		TArray< FExpressionInput*> Inputs;
		int OutputIndex = i;
		GetOutputExpression( ParentMaterial, ParentMaterialFunction, OldExp, OutputIndex, Inputs );
		if (Inputs.Num() > 0)
		{
			for (int j = 0; j < Inputs.Num(); j++)
			{
				auto ExpInput = Inputs[j];
				ExpInput->Expression = NewExp;
				ExpInput->OutputIndex = OutputIndex;
			}
		}
	}
}
template< class EnumType>
bool FixEnum( TEnumAsByte<EnumType>& EnumValue, TArray< EEngineVersion > Versions, TArray<EnumType> MaxValues, EnumType DefaultValue )
{
	for( int i = 0; i < Versions.Num(); i++ )
	{
		EEngineVersion Version = Versions[i];
		EnumType MaxValue = MaxValues[i];

		if( DowngraderSettings->TargetVersion <= Version )
		{
			if( EnumValue > MaxValue )
			{
				EnumValue = DefaultValue;
				return true;
			}
		}
	}

	return false;
}
void FixMaterialExpressions( TArray<TObjectPtr< UMaterialExpression>>& Expressions, UMaterial* ParentMaterial, UMaterialFunction* ParentMaterialFunction )
{
	for( int i = 0; i < Expressions.Num(); i++ )
	{
		UMaterialExpression* Exp = Expressions[ i ].Get();
		UMaterialExpressionTransform* Transform = Cast< UMaterialExpressionTransform>( Exp );
		if( Transform )
		{
			if( DowngraderSettings->TargetVersion <= EEngineVersion::EV_4_27 )
			{
				if( Transform->TransformSourceType == EMaterialVectorCoordTransformSource::TRANSFORMSOURCE_Instance)
				{
					Transform->TransformSourceType = EMaterialVectorCoordTransformSource::TRANSFORMSOURCE_ParticleWorld;
				}
				if( Transform->TransformType == EMaterialVectorCoordTransform::TRANSFORM_Instance )
				{
					Transform->TransformType = EMaterialVectorCoordTransform::TRANSFORM_ParticleWorld;
				}
			}
		}
		UMaterialExpressionTransformPosition* TransformPosition = Cast< UMaterialExpressionTransformPosition>( Exp );
		if( TransformPosition )
		{
			if( DowngraderSettings->TargetVersion <= EEngineVersion::EV_4_27 )
			{
				if( TransformPosition->TransformSourceType == EMaterialPositionTransformSource::TRANSFORMPOSSOURCE_Instance )
				{
					TransformPosition->TransformSourceType = EMaterialPositionTransformSource::TRANSFORMPOSSOURCE_Particle;
				}
				if( TransformPosition->TransformType == EMaterialPositionTransformSource::TRANSFORMPOSSOURCE_Instance )
				{
					TransformPosition->TransformType = EMaterialPositionTransformSource::TRANSFORMPOSSOURCE_Particle;
				}
			}
		}
		UMaterialExpressionMaterialAttributeLayers* LayersExpression = Cast<UMaterialExpressionMaterialAttributeLayers>( Exp );
		if( LayersExpression )
		{
			FixMaterialLayersFunctions( LayersExpression->DefaultLayers, LayersExpression->DefaultLayers );
		}
		UMaterialExpressionSwitch* Switch = Cast<UMaterialExpressionSwitch>( Exp );
		if( Switch && DowngraderSettings->TargetVersion < EEngineVersion::EV_5_2 )
		{
			UMaterialExpression* NewExp = UMaterialEditingLibrary::CreateMaterialExpressionEx( ParentMaterial, ParentMaterialFunction, UMaterialExpressionCustom::StaticClass(), nullptr, Exp->MaterialExpressionEditorX - 10, Exp->MaterialExpressionEditorY - 10 );
			UMaterialExpressionCustom* CustomExp = Cast< UMaterialExpressionCustom>( NewExp );

			FString Code = TEXT( "float eps = 0.01f;\n" );
			CustomExp->Inputs.Empty();
			FString SwitchValueString = FString::Printf( TEXT("%f"), Switch->ConstSwitchValue );
			FString DefaultValueString = FString::Printf( TEXT( "%f" ), Switch->ConstDefault );

			if( Switch->SwitchValue.Expression )
			{
				FCustomInput SwitchValueInput;
				SwitchValueInput.Input = Switch->SwitchValue;
				SwitchValueInput.InputName = FName( TEXT("SwitchValue") );
				CustomExp->Inputs.Add( SwitchValueInput );

				SwitchValueString = TEXT( "SwitchValue" );
			}
			//else ConstSwitchValue

			if( Switch->Default.Expression )
			{
				FCustomInput DefaultInput;
				DefaultInput.Input = Switch->Default;
				DefaultInput.InputName = FName( "DefaultInput" );
				CustomExp->Inputs.Add( DefaultInput );

				DefaultValueString = TEXT( "DefaultInput" );
			}
			//else ConstDefault

			ECustomMaterialOutputType OutputType = CMOT_Float1;

			for( int Case = 0; Case < Switch->Inputs.Num(); Case++ )
			{
				FSwitchCustomInput& SwitchInput = Switch->Inputs[ Case ];

				FCustomInput Input;
				Input.Input = SwitchInput.Input;
				Input.InputName = SwitchInput.InputName;
				CustomExp->Inputs.Add( Input );

				OutputType = FMath::Max( OutputType, EvaluateExpressionInputType( SwitchInput.Input ) );

				Code += AddInputForSwitch( (float)Case, SwitchValueString, SwitchInput.InputName.ToString(), Case > 0 );
			}

			CustomExp->OutputType = OutputType;

			//Add default
			Code += AddInputForSwitch( (float)-1, TEXT("-1"), DefaultValueString, 2 );

			CustomExp->Code = Code;
			CustomExp->Description = "Switch_Downgrader";

			TArray< FExpressionInput*> Inputs;
			int OutputIndex = 0;
			GetOutputExpression( ParentMaterial, ParentMaterialFunction, Exp, OutputIndex, Inputs );
			if( Inputs.Num() > 0 )
			{				
				for( int j = 0; j < Inputs.Num(); j++ )
				{
					auto ExpInput = Inputs[ j ];
					ExpInput->Expression = NewExp;
					ExpInput->OutputIndex = OutputIndex;
				}
			}

			//if( ParentMaterial )
			//	ParentMaterial->GetEditorOnlyData()->ExpressionCollection.Expressions.Remove( Exp );
			//else if( ParentMaterialFunction )
			//	ParentMaterialFunction->GetEditorOnlyData()->ExpressionCollection.Expressions.Remove( Exp );

			//Destroy it
			Exp->Rename( nullptr, GetTransientPackage() );
		}
		UMaterialExpressionSetMaterialAttributes* SetMaterialAttributes = Cast< UMaterialExpressionSetMaterialAttributes>( Exp );
		if ( SetMaterialAttributes && DowngraderSettings->TargetVersion < EEngineVersion::EV_5_3 )
		{
			//SetMaterialAttributes->PreEditChange( nullptr );
			for (int a = 0; a < SetMaterialAttributes->AttributeSetTypes.Num(); a++)
			{
				FGuid InputGUID = SetMaterialAttributes->AttributeSetTypes[a];
				const FString& Str = FMaterialAttributeDefinitionMap::GetAttributeName( InputGUID );
				if (Str.Compare( TEXT( "Displacement" ) ) == 0)
				{
					SetMaterialAttributes->AttributeSetTypes.RemoveAt( a );
					SetMaterialAttributes->Inputs.RemoveAt( a + 1 );
					//if (SetMaterialAttributes->GraphNode)
					//	SetMaterialAttributes->GraphNode->RemovePinAt( a + 1, EGPD_Input );
					break;
				}
			}
			//SetMaterialAttributes->PostEditChange();
		}
		UMaterialExpressionGetMaterialAttributes* GetMaterialAttributes = Cast< UMaterialExpressionGetMaterialAttributes>( Exp );
		if ( GetMaterialAttributes && DowngraderSettings->TargetVersion < EEngineVersion::EV_5_3 )
		{
			//GetMaterialAttributes->PreEditChange( nullptr );
			for (int a = 0; a < GetMaterialAttributes->AttributeGetTypes.Num(); a++)
			{
				FGuid InputGUID = GetMaterialAttributes->AttributeGetTypes[a];
				const FString& Str = FMaterialAttributeDefinitionMap::GetAttributeName( InputGUID );
				if (Str.Compare( TEXT( "Displacement" ) ) == 0)
				{
					GetMaterialAttributes->AttributeGetTypes.RemoveAt( a );
					GetMaterialAttributes->Outputs.RemoveAt( a + 1 );
					//if (GetMaterialAttributes->GraphNode)
					//	GetMaterialAttributes->GraphNode->RemovePinAt( a + 1, EGPD_Output );
					FixRemovedOutputIndex( Expressions, GetMaterialAttributes, a + 1 );
					break;
				}
			}
			//GetMaterialAttributes->PostEditChange();
		}
		UMaterialExpressionWorldPosition* WorldPosition = Cast< UMaterialExpressionWorldPosition>( Exp );
		if( WorldPosition && DowngraderSettings->TargetVersion < EEngineVersion::EV_5_2 )
		{			
			TArray<FExpressionOutput>& Outputs = WorldPosition->GetOutputs();
			if( Outputs.Num() > 1 )
			{
				TArray< FExpressionInput*> Connections;
				GetExpressionConnections( Expressions, Exp, Connections, 1 );
				for( int c = 0; c < Connections.Num(); c++ )
				{
					auto Connection = Connections[ c ];

					UMaterialExpression* NewExp = UMaterialEditingLibrary::CreateMaterialExpressionEx( ParentMaterial, ParentMaterialFunction, UMaterialExpressionComponentMask::StaticClass(), nullptr, Exp->MaterialExpressionEditorX + 50, Exp->MaterialExpressionEditorY + 150 );
					UMaterialExpressionComponentMask* NewCompMask = Cast< UMaterialExpressionComponentMask>( NewExp );
					NewCompMask->R = 1;
					NewCompMask->G = 1;
					NewCompMask->B = 0;
					NewCompMask->A = 0;

					Connection->Expression = NewCompMask;
					NewCompMask->Input.Expression = WorldPosition;
				}
			}
			if( Outputs.Num() > 2 )
			{
				TArray< FExpressionInput*> Connections;
				GetExpressionConnections( Expressions, Exp, Connections, 2 );
				for( int c = 0; c < Connections.Num(); c++ )
				{
					auto Connection = Connections[ c ];

					UMaterialExpression* NewExp = UMaterialEditingLibrary::CreateMaterialExpressionEx( ParentMaterial, ParentMaterialFunction, UMaterialExpressionComponentMask::StaticClass(), nullptr, Exp->MaterialExpressionEditorX + 50, Exp->MaterialExpressionEditorY + 200 );
					UMaterialExpressionComponentMask* NewCompMask = Cast< UMaterialExpressionComponentMask>( NewExp );
					NewCompMask->R = 0;
					NewCompMask->G = 0;
					NewCompMask->B = 1;
					NewCompMask->A = 0;

					Connection->Expression = NewCompMask;
					NewCompMask->Input.Expression = WorldPosition;
				}
			}
		}
		#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
		UMaterialExpressionLength* Length = Cast<UMaterialExpressionLength>( Exp );
		if (Length && DowngraderSettings->TargetVersion < EEngineVersion::EV_5_3)
		{
			UMaterialExpression* NewExp = UMaterialEditingLibrary::CreateMaterialExpressionEx( ParentMaterial, ParentMaterialFunction, UMaterialExpressionMaterialFunctionCall::StaticClass(), nullptr, Exp->MaterialExpressionEditorX, Exp->MaterialExpressionEditorY );
			UMaterialExpressionMaterialFunctionCall* NewFunctionCall = Cast< UMaterialExpressionMaterialFunctionCall>( NewExp );

			UObject* LoadedObject = StaticLoadObject( UMaterialFunction::StaticClass(), nullptr, TEXT( "/Engine/Functions/Engine_MaterialFunctions02/Utility/VectorLength" ) );
			UMaterialFunction* VectorLengthFunction = Cast<UMaterialFunction>( LoadedObject );

			NewFunctionCall->MaterialFunction = VectorLengthFunction;
			NewFunctionCall->UpdateFromFunctionResource();

			ECustomMaterialOutputType InputType = EvaluateExpressionInputType( Length->Input );

			if (NewFunctionCall->FunctionInputs.Num() > 1)
			{
				if (InputType == ECustomMaterialOutputType::CMOT_Float2)
				{
					NewFunctionCall->FunctionInputs[1].Input = Length->Input;
				}
				else
				{
					NewFunctionCall->FunctionInputs[0].Input = Length->Input;
				}
			}

			TArray< FExpressionInput*> Inputs;
			int OutputIndex = 0;
			GetOutputExpression( ParentMaterial, ParentMaterialFunction, Exp, OutputIndex, Inputs );
			if (Inputs.Num() > 0)
			{
				for (int j = 0; j < Inputs.Num(); j++)
				{
					auto ExpInput = Inputs[j];
					ExpInput->Expression = NewFunctionCall;
					if (InputType == ECustomMaterialOutputType::CMOT_Float2)
						ExpInput->OutputIndex = 1;
					else
						ExpInput->OutputIndex = 0;
				}
			}

			//Destroy it
			Exp->Rename( nullptr, GetTransientPackage() );
		}
		#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 7
		UMaterialExpressionEyeAdaptationInverse* EyeAdaptationInverse = Cast< UMaterialExpressionEyeAdaptationInverse>( Exp );
		if (EyeAdaptationInverse && DowngraderSettings->TargetVersion < EEngineVersion::EV_5_0)
		{
			UMaterialExpression* NewExp = UMaterialEditingLibrary::CreateMaterialExpressionEx( ParentMaterial, ParentMaterialFunction, UMaterialExpressionCustom::StaticClass(), nullptr, Exp->MaterialExpressionEditorX, Exp->MaterialExpressionEditorY );
			UMaterialExpressionCustom* NewCustomExp = Cast< UMaterialExpressionCustom>( NewExp );

			NewCustomExp->Inputs.Empty();
			FString DefaultInputs;
			
			FCustomInput NewInput;
			NewInput.Input = EyeAdaptationInverse->LightValueInput;
			NewInput.InputName = FName( TEXT( "LightValue" ) );
			NewCustomExp->Inputs.Add( NewInput );
			
			if (!EyeAdaptationInverse->LightValueInput.Expression)
			{
				DefaultInputs += FString::Printf( TEXT( "LightValue = float3(1,1,1);\n" ) );
			}
			
			FCustomInput NewInput2;
			NewInput2.Input = EyeAdaptationInverse->AlphaInput;
			NewInput2.InputName = FName( TEXT( "Alpha" ) );
			NewCustomExp->Inputs.Add( NewInput2 );

			if (!EyeAdaptationInverse->AlphaInput.Expression)
			{
				DefaultInputs += FString::Printf( TEXT( "Alpha = 1;\n" ) );
			}

			NewCustomExp->OutputType = ECustomMaterialOutputType::CMOT_Float3;

			NewCustomExp->Description = TEXT( "EyeAdaptationInverse_Downgrader" );
			NewCustomExp->Code = DefaultInputs;
			NewCustomExp->Code += LR"(
float InverseExposureLerp = 1.0;
float Exposure = EyeAdaptationLookup();

#if EYE_ADAPTATION_DISABLED
	InverseExposureLerp = 1.0;
#else
	// When Alpha=0.0, we want to multiply by 1.0. when Alpha = 1.0, we want to multiply by 1/Exposure.
	// So the lerped value is:
	//     LerpLogScale = Lerp(log(1),log(1/Exposure),T)
	// Which is simplified as:
	//     LerpLogScale = Lerp(0,-log(Exposure),T)
	//     LerpLogScale = -T * log(Exposure);

	float LerpLogScale = -Alpha * log(Exposure);
	float Scale = exp(LerpLogScale);
	InverseExposureLerp = Scale;
#endif

	return LightValue * InverseExposureLerp;
)";			

			ReconnectNewNode( ParentMaterial, ParentMaterialFunction, Exp, NewCustomExp, 2 );
			
			//Destroy it
			Exp->Rename( nullptr, GetTransientPackage() );
		}
		#endif
		UMaterialExpressionRgbToHsv* RgbToHsv = Cast<UMaterialExpressionRgbToHsv>( Exp );
		UMaterialExpressionHsvToRgb* HsvToRgb = Cast<UMaterialExpressionHsvToRgb>( Exp );
		if ((RgbToHsv || HsvToRgb) && DowngraderSettings->TargetVersion < EEngineVersion::EV_5_3)
		{
			UMaterialExpression* NewExp = UMaterialEditingLibrary::CreateMaterialExpressionEx( ParentMaterial, ParentMaterialFunction, UMaterialExpressionCustom::StaticClass(), nullptr, Exp->MaterialExpressionEditorX, Exp->MaterialExpressionEditorY );
			UMaterialExpressionCustom* NewCustomExp = Cast< UMaterialExpressionCustom>( NewExp );

			NewCustomExp->Inputs.Empty();

			FCustomInput ValueInput;
			if (RgbToHsv)
				ValueInput.Input = RgbToHsv->Input;
			else
				ValueInput.Input = HsvToRgb->Input;
			ValueInput.InputName = FName( TEXT( "In" ) );
			NewCustomExp->Inputs.Add( ValueInput );

			NewCustomExp->OutputType = ECustomMaterialOutputType::CMOT_Float3;

			if (RgbToHsv)
			{
				NewCustomExp->Description = TEXT( "RgbToHsv_Downgrader" );
				NewCustomExp->Code = LR"(
float3 c = In;

    float4 K = float4(0.0, -1.0/3.0, 2.0/3.0, -1.0);
    float4 p = (c.g < c.b) ? float4(c.bg, K.wz) : float4(c.gb, K.xy);
    float4 q = (c.r < p.x) ? float4(p.xyw, c.r) : float4(c.r, p.yzx);

    float d = q.x - min(q.w, q.y);
    float e = 1e-10;

    float h = abs(q.z + (q.w - q.y) / (6.0 * d + e));
    float s = d / (q.x + e);
    float v = q.x;

    return float3(h, s, v);

)";
			}
			else
			{
				NewCustomExp->Description = TEXT( "HsvToRgb_Downgrader" );
				NewCustomExp->Code = LR"(
float3 c = In;

    float h = c.x;
    float s = c.y;
    float v = c.z;

    float3 rgb = float3(0.0, 0.0, 0.0);

    float i = floor(h * 6.0);
    float f = h * 6.0 - i;
    float p = v * (1.0 - s);
    float q = v * (1.0 - f * s);
    float t = v * (1.0 - (1.0 - f) * s);

    int mod = (int)i % 6;

    if (mod == 0) rgb = float3(v, t, p);
    else if (mod == 1) rgb = float3(q, v, p);
    else if (mod == 2) rgb = float3(p, v, t);
    else if (mod == 3) rgb = float3(p, q, v);
    else if (mod == 4) rgb = float3(t, p, v);
    else rgb = float3(v, p, q);

    return rgb;
)";
			}

			TArray< FExpressionInput*> Inputs;
			int OutputIndex = 0;
			GetOutputExpression( ParentMaterial, ParentMaterialFunction, Exp, OutputIndex, Inputs );
			if (Inputs.Num() > 0)
			{
				for (int j = 0; j < Inputs.Num(); j++)
				{
					auto ExpInput = Inputs[j];
					ExpInput->Expression = NewCustomExp;
					ExpInput->OutputIndex = 0;
				}
			}

			//Destroy it
			Exp->Rename( nullptr, GetTransientPackage() );
		}
		#endif
		#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 6//This is because on 5.4 I get a linker error
		UMaterialExpressionPathTracingQualitySwitch* PathTracingQualitySwitch = Cast< UMaterialExpressionPathTracingQualitySwitch>( Exp );
		if (PathTracingQualitySwitch && DowngraderSettings->TargetVersion < EEngineVersion::EV_5_0)
		{
			TArray< FExpressionInput*> Inputs;
			int OutputIndex = 0;
			GetOutputExpression( ParentMaterial, ParentMaterialFunction, Exp, OutputIndex, Inputs );
			if (Inputs.Num() > 0)
			{
				for (int j = 0; j < Inputs.Num(); j++)
				{
					auto ExpInput = Inputs[j];
					ExpInput->Expression = PathTracingQualitySwitch->Normal.Expression;
					ExpInput->OutputIndex = PathTracingQualitySwitch->Normal.OutputIndex;
				}
			}

			//Destroy it
			Exp->Rename( nullptr, GetTransientPackage() );
		}
		#endif
		UMaterialExpressionFunctionInput* FunctionInput = Cast< UMaterialExpressionFunctionInput>( Exp );
		if (FunctionInput && FunctionInput->InputType == EFunctionInputType::FunctionInput_StaticBool &&
			!FunctionInput->Preview.Expression && DowngraderSettings->TargetVersion < EEngineVersion::EV_5_7)
		{
			UMaterialExpression* NewExp = UMaterialEditingLibrary::CreateMaterialExpressionEx( ParentMaterial, ParentMaterialFunction, UMaterialExpressionStaticBool::StaticClass(), nullptr, Exp->MaterialExpressionEditorX - 100, Exp->MaterialExpressionEditorY );
			UMaterialExpressionStaticBool* NewBoolExp = Cast< UMaterialExpressionStaticBool>( NewExp );
			NewBoolExp->Value = (uint32)FunctionInput->PreviewValue.X;

			FunctionInput->Preview.Expression = NewBoolExp;
		}
		UMaterialExpressionViewProperty* ViewProperty = Cast< UMaterialExpressionViewProperty>( Exp );
		if( ViewProperty )
		{
			TArray< EEngineVersion> Versions{ EEngineVersion::EV_5_5, EEngineVersion::EV_5_4,EEngineVersion::EV_5_0 };
			TArray< EMaterialExposedViewProperty> MaxValues{ MEVP_PostVolumeUserFlags, MEVP_ResolutionFraction, MEVP_RuntimeVirtualTextureMaxLevel };
			FixEnum< EMaterialExposedViewProperty >( ViewProperty->Property, Versions , MaxValues, MEVP_FieldOfView );
		}
	}
}
#endif
void FixMaterialInstance( UMaterialInstance* MaterialInstance )
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3

	RemovePropertyFlag( TEXT( "StaticParameterSetEditorOnlyData" ), TEXT( "StaticSwitchParameters" ), CPF_Deprecated );
	RemovePropertyFlag( TEXT( "StaticParameterSet" ), TEXT( "MaterialLayersParameters" ), CPF_Deprecated );

	int StaticParametersOffset = GetMemberOffset( TEXT( "MaterialInstance" ), TEXT( "StaticParametersRuntime" ) );

	FStaticParameterSetRuntimeData* StaticParameterSetRuntimeData = (FStaticParameterSetRuntimeData*)(((uint8*)MaterialInstance) + StaticParametersOffset);
	UMaterialInstanceEditorOnlyData* EditorOnlyData = MaterialInstance->GetEditorOnlyData();

	if( DowngraderSettings->TargetVersion < EEngineVersion::EV_5_1 )
	{
		FStaticParameterSet& StaticParameters = GetVariable< FStaticParameterSet>( MaterialInstance, TEXT( "MaterialInstance" ), TEXT( "StaticParameters" ) );
		TArray<FStaticMaterialLayersParameter>& MaterialLayersParameters = GetVariable< TArray<FStaticMaterialLayersParameter>>( &StaticParameters, TEXT( "StaticParameterSet" ), TEXT( "MaterialLayersParameters" ) );
		
		if (MaterialLayersParameters.Num() == 0 && StaticParameterSetRuntimeData->MaterialLayers.Layers.Num() > 0)
		{
			MaterialLayersParameters.AddDefaulted();
			MaterialLayersParameters[0].bOverride = true;

			FixMaterialLayersFunctions( MaterialLayersParameters[0].Value, EditorOnlyData->StaticParameters.MaterialLayers );

			for (int i = 0; i < StaticParameterSetRuntimeData->MaterialLayers.Layers.Num(); i++)
			{
				MaterialLayersParameters[0].Value.Layers.Add( StaticParameterSetRuntimeData->MaterialLayers.Layers[i] );
				StaticParameters.MaterialLayers.Layers.Add( StaticParameterSetRuntimeData->MaterialLayers.Layers[i] );
			}
			for (int i = 0; i < StaticParameterSetRuntimeData->MaterialLayers.Blends.Num(); i++)
			{
				MaterialLayersParameters[0].Value.Blends.Add( StaticParameterSetRuntimeData->MaterialLayers.Blends[i] );
				StaticParameters.MaterialLayers.Blends.Add( StaticParameterSetRuntimeData->MaterialLayers.Blends[i] );
			}

			if (MaterialLayersParameters[0].Value.Layers.Num() == 0)
			{
				MaterialLayersParameters = MaterialLayersParameters;
			}
		}		
	}
	if( DowngraderSettings->TargetVersion < EEngineVersion::EV_5_2 )
	{
		FStaticParameterSet& StaticParameters = GetVariable< FStaticParameterSet>( MaterialInstance, TEXT( "MaterialInstance" ), TEXT( "StaticParameters" ) );
		TArray<FStaticSwitchParameter>& StaticSwitchParameters = GetVariable< TArray<FStaticSwitchParameter>>( &StaticParameters, TEXT( "StaticParameterSet" ), TEXT( "StaticSwitchParameters" ) );

		for (int i = 0; i < StaticParameterSetRuntimeData->StaticSwitchParameters.Num(); i++)
		{
			EditorOnlyData->StaticParameters.StaticSwitchParameters_DEPRECATED.Add( StaticParameterSetRuntimeData->StaticSwitchParameters[i] );
		}

		for( int i = 0; i < EditorOnlyData->StaticParameters.StaticSwitchParameters_DEPRECATED.Num(); i++ )
		{
			StaticParameters.StaticSwitchParameters.Add( EditorOnlyData->StaticParameters.StaticSwitchParameters_DEPRECATED[ i ] );
			StaticSwitchParameters.Add( EditorOnlyData->StaticParameters.StaticSwitchParameters_DEPRECATED[ i ] );
		}
		
		//Process to clear them, as otherwise opening this asset in 5.3.2 will hit an ensure
		StaticParameterSetRuntimeData->Empty();
	}
#endif
}
void FixBlendableLocation( UMaterial* Material )
{
	if( DowngraderSettings->TargetVersion <= EEngineVersion::EV_5_3 )
	{
		if( Material->BlendableLocation >= 5 )//BL_MAX up to 5.3
		{
		#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4
			Material->BlendableLocation = BL_SceneColorAfterTonemapping;
		#endif
		}
	}
}
int NodesOffsetX = 220;
int NodesOffsetY = 80;
void FixDefaultOutputValue( UMaterial* Material, FColorMaterialInput* ColorInput, int Index = 0 )
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
	if (!ColorInput->Expression)
	{
		FVector2D Pos = FVector2D( Material->EditorX - NodesOffsetX, Material->EditorY - 240 + Index * NodesOffsetY );
		UMaterialExpression* NewExp = UMaterialEditingLibrary::CreateMaterialExpressionEx( Material, nullptr, UMaterialExpressionConstant3Vector::StaticClass(), nullptr, Pos.X, Pos.Y );
		UMaterialExpressionConstant3Vector* Const3Exp = Cast< UMaterialExpressionConstant3Vector>( NewExp );
		Const3Exp->Constant = ColorInput->Constant;
		ColorInput->Expression = Const3Exp;
	}
#endif
}
void FixDefaultOutputValue( UMaterial* Material, FVector2MaterialInput* Vector2Input, int Index = 0 )
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
	if (!Vector2Input->Expression)
	{
		FVector2D Pos = FVector2D( Material->EditorX - NodesOffsetX, Material->EditorY - 240 + Index * NodesOffsetY );
		UMaterialExpression* NewExp = UMaterialEditingLibrary::CreateMaterialExpressionEx( Material, nullptr, UMaterialExpressionConstant2Vector::StaticClass(), nullptr, Pos.X, Pos.Y );
		UMaterialExpressionConstant2Vector* Const2Exp = Cast< UMaterialExpressionConstant2Vector>( NewExp );
		Const2Exp->R = Vector2Input->Constant.X;
		Const2Exp->G = Vector2Input->Constant.Y;
		Vector2Input->Expression = Const2Exp;
	}
#endif
}
void FixDefaultOutputValue( UMaterial* Material, FVectorMaterialInput* VectorInput, int Index = 0 )
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
	if (!VectorInput->Expression)
	{
		FVector2D Pos = FVector2D( Material->EditorX - NodesOffsetX, Material->EditorY - 240 + Index * NodesOffsetY );
		UMaterialExpression* NewExp = UMaterialEditingLibrary::CreateMaterialExpressionEx( Material, nullptr, UMaterialExpressionConstant3Vector::StaticClass(), nullptr, Pos.X, Pos.Y );
		UMaterialExpressionConstant3Vector* Const3Exp = Cast< UMaterialExpressionConstant3Vector>( NewExp );
		Const3Exp->Constant = VectorInput->Constant;
		VectorInput->Expression = Const3Exp;
	}
#endif
}
void FixDefaultOutputValue( UMaterial* Material, FScalarMaterialInput* ScalarInput, int Index = 0 )
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
	if (!ScalarInput->Expression)
	{
		FVector2D Pos = FVector2D( Material->EditorX - NodesOffsetX, Material->EditorY - 240 + Index * NodesOffsetY );
		UMaterialExpression* NewExp = UMaterialEditingLibrary::CreateMaterialExpressionEx( Material, nullptr, UMaterialExpressionConstant::StaticClass(), nullptr, Pos.X, Pos.Y );
		UMaterialExpressionConstant* ConstExp = Cast< UMaterialExpressionConstant>( NewExp );
		ConstExp->R = ScalarInput->Constant;
		ScalarInput->Expression = ConstExp;
	}
#endif
}

void FixMaterial( UMaterial* Material )
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
	FixBlendableLocation( Material );
	if ( DowngraderSettings->TargetVersion < EEngineVersion::EV_5_4 &&//5.4 introduced default outputs
		!Material->bUseMaterialAttributes )
	{
		UMaterialEditorOnlyData* EditorOnlyData = Material->GetEditorOnlyData();
		FixDefaultOutputValue( Material, &EditorOnlyData->BaseColor, 0 );
		FixDefaultOutputValue( Material, &EditorOnlyData->Metallic, 1 );
		FixDefaultOutputValue( Material, &EditorOnlyData->Specular, 2 );
		FixDefaultOutputValue( Material, &EditorOnlyData->Roughness, 3 );
		FixDefaultOutputValue( Material, &EditorOnlyData->Anisotropy, 4 );
		FixDefaultOutputValue( Material, &EditorOnlyData->Normal, 5 );
		FixDefaultOutputValue( Material, &EditorOnlyData->Tangent, 6 );
		FixDefaultOutputValue( Material, &EditorOnlyData->EmissiveColor, 7 );
		if (Material->BlendMode == EBlendMode::BLEND_Translucent ||
			Material->GetShadingModels().HasShadingModel( EMaterialShadingModel::MSM_TwoSidedFoliage ) ||
			Material->GetShadingModels().HasShadingModel( EMaterialShadingModel::MSM_SingleLayerWater ) ||
			Material->GetShadingModels().HasShadingModel( EMaterialShadingModel::MSM_Cloth ) ||
			Material->GetShadingModels().HasShadingModel( EMaterialShadingModel::MSM_Eye ) ||
			Material->GetShadingModels().HasShadingModel( EMaterialShadingModel::MSM_Subsurface ) ||
			Material->GetShadingModels().HasShadingModel( EMaterialShadingModel::MSM_PreintegratedSkin ))
			FixDefaultOutputValue( Material, &EditorOnlyData->Opacity, 8 );
		if (Material->BlendMode == EBlendMode::BLEND_Masked)
			FixDefaultOutputValue( Material, &EditorOnlyData->OpacityMask, 9 );
		//FixDefaultOutputValue( Material, &EditorOnlyData->WorldPositionOffset, 10 );
		if (Material->GetShadingModels().HasShadingModel( EMaterialShadingModel::MSM_Subsurface ) ||
			Material->GetShadingModels().HasShadingModel( EMaterialShadingModel::MSM_Cloth ) ||
			Material->GetShadingModels().HasShadingModel( EMaterialShadingModel::MSM_TwoSidedFoliage ))
			FixDefaultOutputValue( Material, &EditorOnlyData->SubsurfaceColor, 11 );
		if (Material->GetShadingModels().HasShadingModel( EMaterialShadingModel::MSM_ClearCoat ) ||
			Material->GetShadingModels().HasShadingModel( EMaterialShadingModel::MSM_Hair) ||
			Material->GetShadingModels().HasShadingModel( EMaterialShadingModel::MSM_Cloth) ||
			Material->GetShadingModels().HasShadingModel( EMaterialShadingModel::MSM_Eye ))
			FixDefaultOutputValue( Material, &EditorOnlyData->ClearCoat, 12 );
		if (Material->GetShadingModels().HasShadingModel( EMaterialShadingModel::MSM_ClearCoat ) ||
			Material->GetShadingModels().HasShadingModel( EMaterialShadingModel::MSM_Eye ))
			FixDefaultOutputValue( Material, &EditorOnlyData->ClearCoatRoughness, 13 );
		FixDefaultOutputValue( Material, &EditorOnlyData->AmbientOcclusion, 14 );
		if ( Material->BlendMode == EBlendMode::BLEND_Translucent ||
			(Material->BlendMode == EBlendMode::BLEND_Opaque && Material->GetShadingModels().HasShadingModel( EMaterialShadingModel::MSM_SingleLayerWater )))
			FixDefaultOutputValue( Material, &EditorOnlyData->Refraction, 15 );
		//FixDefaultOutputValue( Material, &EditorOnlyData->PixelDepthOffset, 16 );
		for (int i = 0; i < Material->NumCustomizedUVs; i++)
		{
			FixDefaultOutputValue( Material, &EditorOnlyData->CustomizedUVs[i], 17 + i );
		}
	}

	UMaterialEditorOnlyData* EditorOnlyData = Material->GetEditorOnlyData();
	//This needs to happen regardless if its < 5.1 or not
	FixMaterialExpressions( EditorOnlyData->ExpressionCollection.Expressions, Material, nullptr );

	if( DowngraderSettings->TargetVersion < EEngineVersion::EV_5_1 )
	{
		int ExpressionsOffset = GetMemberOffset( TEXT( "Material" ), TEXT( "Expressions" ) );
		int EditorCommentsOffset = GetMemberOffset( TEXT( "Material" ), TEXT( "EditorComments" ) );
		RemovePropertyFlag( TEXT( "Material" ), TEXT( "Expressions" ), CPF_Deprecated );
		RemovePropertyFlag( TEXT( "Material" ), TEXT( "EditorComments" ), CPF_Deprecated );

		TArray<TObjectPtr<class UMaterialExpression>>& DeprecatedExpressions = GetVariable< TArray<TObjectPtr<UMaterialExpression>>>( Material, ExpressionsOffset );
		if( DeprecatedExpressions.Num() == 0 )
			DeprecatedExpressions = EditorOnlyData->ExpressionCollection.Expressions;

		TArray<TObjectPtr<class UMaterialExpressionComment>>& DeprecatedComments = GetVariable< TArray<TObjectPtr<UMaterialExpressionComment>>>( Material, EditorCommentsOffset );
		if( DeprecatedComments.Num() == 0 )
			DeprecatedComments = EditorOnlyData->ExpressionCollection.EditorComments;

		AssignVariable<FColorMaterialInput>( Material, TEXT( "Material" ), TEXT( "BaseColor" ), EditorOnlyData->BaseColor );
		AssignVariable<FScalarMaterialInput>( Material, TEXT( "Material" ), TEXT( "Metallic" ), EditorOnlyData->Metallic );
		AssignVariable<FScalarMaterialInput>( Material, TEXT( "Material" ), TEXT( "Specular" ), EditorOnlyData->Specular );
		AssignVariable<FScalarMaterialInput>( Material, TEXT( "Material" ), TEXT( "Roughness" ), EditorOnlyData->Roughness );
		AssignVariable<FScalarMaterialInput>( Material, TEXT( "Material" ), TEXT( "Anisotropy" ), EditorOnlyData->Anisotropy );
		AssignVariable<FVectorMaterialInput>( Material, TEXT( "Material" ), TEXT( "Normal" ), EditorOnlyData->Normal );
		AssignVariable<FVectorMaterialInput>( Material, TEXT( "Material" ), TEXT( "Tangent" ), EditorOnlyData->Tangent );
		AssignVariable<FColorMaterialInput>( Material, TEXT( "Material" ), TEXT( "EmissiveColor" ), EditorOnlyData->EmissiveColor );
		AssignVariable<FScalarMaterialInput>( Material, TEXT( "Material" ), TEXT( "Opacity" ), EditorOnlyData->Opacity );
		AssignVariable<FScalarMaterialInput>( Material, TEXT( "Material" ), TEXT( "OpacityMask" ), EditorOnlyData->OpacityMask );
		AssignVariable<FVectorMaterialInput>( Material, TEXT( "Material" ), TEXT( "WorldPositionOffset" ), EditorOnlyData->WorldPositionOffset );
		AssignVariable<FColorMaterialInput>( Material, TEXT( "Material" ), TEXT( "SubsurfaceColor" ), EditorOnlyData->SubsurfaceColor );
		AssignVariable<FScalarMaterialInput>( Material, TEXT( "Material" ), TEXT( "ClearCoat" ), EditorOnlyData->ClearCoat );
		AssignVariable<FScalarMaterialInput>( Material, TEXT( "Material" ), TEXT( "ClearCoatRoughness" ), EditorOnlyData->ClearCoatRoughness );
		AssignVariable<FScalarMaterialInput>( Material, TEXT( "Material" ), TEXT( "AmbientOcclusion" ), EditorOnlyData->AmbientOcclusion );
		AssignVariable<FScalarMaterialInput>( Material, TEXT( "Material" ), TEXT( "Refraction" ), EditorOnlyData->Refraction );
		for( int i = 0; i < 8; i++ )
		{
			int CustomizedUVsOffset = GetMemberOffset( TEXT( "Material" ), TEXT( "CustomizedUVs" ), i );
			RemovePropertyFlag( TEXT( "Material" ), TEXT( "CustomizedUVs" ), CPF_Deprecated );

			AssignVariable<FVector2MaterialInput>( Material, CustomizedUVsOffset, EditorOnlyData->CustomizedUVs[ i ] );
		}
		AssignVariable<FMaterialAttributesInput>( Material, TEXT( "Material" ), TEXT( "MaterialAttributes" ), EditorOnlyData->MaterialAttributes );
		AssignVariable<FScalarMaterialInput>( Material, TEXT( "Material" ), TEXT( "PixelDepthOffset" ), EditorOnlyData->PixelDepthOffset );
		AssignVariable<FShadingModelMaterialInput>( Material, TEXT( "Material" ), TEXT( "ShadingModelFromMaterialExpression" ), EditorOnlyData->ShadingModelFromMaterialExpression );

		EditorOnlyData->ExpressionCollection.Expressions.Reset( 0 );
		EditorOnlyData->ExpressionCollection.EditorComments.Reset( 0 );
	}
	#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5
	if( DowngraderSettings->TargetVersion <= EEngineVersion::EV_5_4 )
	{
		Material->PixelDepthOffsetMode = PDOM_Legacy;
	}
	
	if (DowngraderSettings->TargetVersion < EEngineVersion::EV_5_2)
	{
		if ( Material->RefractionMethod > ERefractionMode::RM_PixelNormalOffset )
		{
			Material->RefractionMethod = ERefractionMode::RM_IndexOfRefraction;
		}
	}
	if (DowngraderSettings->TargetVersion < EEngineVersion::EV_5_0)
	{
		//Deprecated from 5.0+
		Material->DecalBlendMode = DBM_DBuffer_ColorNormalRoughness;
	}

	Material->RefractionMode_DEPRECATED = Material->RefractionMethod;
	#endif
#endif
}
void FixMaterialFunction( UMaterialFunction* MaterialFunction )
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
	RemovePropertyFlag( TEXT( "MaterialFunction" ), TEXT( "FunctionExpressions" ), CPF_Deprecated );
	RemovePropertyFlag( TEXT( "MaterialFunction" ), TEXT( "FunctionEditorComments" ), CPF_Deprecated );

	UMaterialFunctionEditorOnlyData* EditorOnlyData = MaterialFunction->GetEditorOnlyData();

	FixMaterialExpressions( EditorOnlyData->ExpressionCollection.Expressions, nullptr, MaterialFunction );

	if ( EditorOnlyData->ExpressionCollection.Expressions.Num() > 0 )
		AssignVariable<TArray<TObjectPtr<UMaterialExpression>>>( MaterialFunction, TEXT( "MaterialFunction" ), TEXT( "FunctionExpressions" ), EditorOnlyData->ExpressionCollection.Expressions );
	if( EditorOnlyData->ExpressionCollection.EditorComments.Num() > 0 )
		AssignVariable<TArray<TObjectPtr<UMaterialExpressionComment>>>( MaterialFunction, TEXT( "MaterialFunction" ), TEXT( "FunctionEditorComments" ), EditorOnlyData->ExpressionCollection.EditorComments );

	EditorOnlyData->ExpressionCollection.Expressions.Reset( 0 );
	EditorOnlyData->ExpressionCollection.EditorComments.Reset( 0 );
#endif
}
void ReorderMaterialsToBeConsecutive( UStaticMesh* StaticMesh, TArray<int>& MaterialIndices, int LOD )
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
	if( StaticMesh->GetRenderData()->LODResources.Num() == 0 )
		return;

	bool Modified = false;
	TArray<FStaticMaterial> OldMaterials = StaticMesh->GetStaticMaterials();
	TArray<FStaticMaterial> NewMaterials;

	FMeshSectionInfoMap& SectionInfoMap = StaticMesh->GetSectionInfoMap();

	int NumSections = SectionInfoMap.GetSectionNumber( LOD );
	for( int i = 0; i < NumSections; i++ )
	{
		FMeshSectionInfo SectionInfo = SectionInfoMap.Get( LOD, i );

		MaterialIndices.Add( SectionInfo.MaterialIndex );

		if( SectionInfo.MaterialIndex >= 0 && SectionInfo.MaterialIndex < OldMaterials.Num() &&
			SectionInfo.MaterialIndex != i )
		{
			NewMaterials.Add( OldMaterials[ SectionInfo.MaterialIndex ] );
			SectionInfo.MaterialIndex = i;
			SectionInfoMap.Set( LOD, i, SectionInfo );
			Modified = true;
		}
		else if( SectionInfo.MaterialIndex == i )
		{
			NewMaterials.Add( OldMaterials[ SectionInfo.MaterialIndex ] );
		}
	}
	
	//For the other LODs, delete their material indices and make it identical to LOD0
	int NumLODs = StaticMesh->GetNumLODs();
	for (int l = 1; l < NumLODs; l++)
	{
		NumSections = SectionInfoMap.GetSectionNumber( l );
		for (int s = 0; s < NumSections; s++)
		{
			SectionInfoMap.Remove( l, s );
		}
	}

	FMeshSectionInfoMap& OriginalSectionInfoMap = StaticMesh->GetOriginalSectionInfoMap();
	NumLODs = StaticMesh->GetNumLODs();
	for (int l = 1; l < NumLODs; l++)
	{
		NumSections = OriginalSectionInfoMap.GetSectionNumber( l );
		for (int s = 0; s < NumSections; s++)
		{
			OriginalSectionInfoMap.Remove( l, s );
		}
	}

	//Not sure if this is required, because RenderData is deserialized from DDC and then SectionInfoMap gets populated from RenderData
	FStaticMeshRenderData* RenderData = StaticMesh->GetRenderData();
	for (int l = 0; l < RenderData->LODResources.Num(); l++)
	{
		FStaticMeshLODResources& LODResources = RenderData->LODResources[l];
		
		for (int s = 0; s < LODResources.Sections.Num(); s++)
		{
			FStaticMeshSection& Section = LODResources.Sections[s];
			
			FMeshSectionInfo SectionInfo = SectionInfoMap.Get( 0, s );
			Section.MaterialIndex = SectionInfo.MaterialIndex;
		}
	}

	if( Modified )
	{
		StaticMesh->SetStaticMaterials( NewMaterials );

		//StaticMesh->PostEditChange();
		//StaticMesh->MarkPackageDirty();
	}
#endif
}
void FixStaticMesh( UStaticMesh* StaticMesh, EEngineVersion TargetVersion )
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
	
	//Only nanite meshes hit an assert related to nan UVs if those exist
	bool Modified = false;
	if( StaticMesh->NaniteSettings.bEnabled )
	{
		int NumSourceModels = StaticMesh->GetNumSourceModels();
		for( int i = 0; i < NumSourceModels; i++ )
		{
			FStaticMeshSourceModel& SourceModel = StaticMesh->GetSourceModel( i );
			FMeshDescription* MeshDescription = SourceModel.GetOrCacheMeshDescription();
			if( MeshDescription )
			{
				FStaticMeshAttributes Attributes( *MeshDescription );
				TVertexInstanceAttributesRef<FVector2f> VertexInstanceUVs = Attributes.GetVertexInstanceUVs();
				const int32 NumTextureCoord = VertexInstanceUVs.IsValid() ? VertexInstanceUVs.GetNumChannels() : 0;

				if( SourceModel.BuildSettings.bGenerateLightmapUVs )
				{
					if( SourceModel.BuildSettings.DstLightmapIndex > NumTextureCoord )
					{
						SourceModel.BuildSettings.DstLightmapIndex = NumTextureCoord;
						Modified = true;
					}
				}

				for( int e = 0; e < VertexInstanceUVs.GetNumElements(); e++ )
				{
					for( int uv = 0; uv < NumTextureCoord; uv++ )
					{
						FVector2f UV = VertexInstanceUVs.Get( e, uv );
						bool UVModified = false;
						if( FMath::IsNaN( UV.X ) )
						{
							UV.X = 0;
							UVModified = true;
						}
						if( FMath::IsNaN( UV.Y ) )
						{
							UV.Y = 0;
							UVModified = true;
						}
						if( UVModified )
						{
							VertexInstanceUVs.Set( e, uv, UV );
							Modified = true;
						}
					}
				}
			}
		}
	}
	if( TargetVersion <= EEngineVersion::EV_4_27 )
	{
		//BodySetup primitives work in 1.19
		//if( StaticMesh->GetBodySetup() )
		//{
		//	StaticMesh->SetBodySetup( nullptr );
		//	Modified = true;
		//}
		if( StaticMesh->NaniteSettings.bEnabled )
		{
			StaticMesh->NaniteSettings.bEnabled = 0;
			Modified = true;
		}

		TArray<int> MaterialIndices;
		ReorderMaterialsToBeConsecutive( StaticMesh, MaterialIndices, 0 );

		for( int i = 0; i < StaticMesh->GetNumSourceModels(); i++ )
		{
			FStaticMeshSourceModel& MeshSourceModel = StaticMesh->GetSourceModel( i );
			FRawMesh RawMesh;
			MeshSourceModel.LoadRawMesh( RawMesh );
			//FStaticMeshSourceModel_LoadRawMesh( MeshSourceModel, RawMesh );
			
			MeshSourceModel.RawMeshBulkData->SaveRawMesh( RawMesh );
			Modified = true;
		}
	}
	if ( Modified )
	{
		StaticMesh->PostEditChange();
		StaticMesh->MarkPackageDirty();
	}
#endif
}
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4
class FRawSkeletalMeshBulkData_Downgrader
{
public:
	/** Protects simultaneous access to BulkData */
	TDontCopy<FRWLock> BulkDataLock;

	/** Internally store bulk data as bytes. */
	FByteBulkData BulkData;
	/** GUID associated with the data stored herein. */
	FGuid Guid;
	/** If true, the GUID is actually a hash of the contents. */
	bool bGuidIsHash;

	//The custom version when this was load
	FCustomVersionContainer SerializeLoadingCustomVersionContainer;
	FPackageFileVersion UEVersion;
	int32 LicenseeUEVersion = 0;
	bool bUseSerializeLoadingCustomVersion = false;

	/*
	 * The last geo imported version, we use this flag to know if we have some data or not.
	 * This flag must be updated every time we import a new geometry
	 */
	ESkeletalMeshGeoImportVersions GeoImportVersion;

	/*
	 * The last skinning imported version, we use this flag to know if we have some data or not.
	 * This flag must be updated every time we import the skinning
	 */
	ESkeletalMeshSkinningImportVersions SkinningImportVersion;
};
struct FMeshDescriptionBulkData_Downgrader
{
	/** Protects simultaneous access to BulkData */
	FRWLock       BulkDataLock;

	/** Internally store bulk data as bytes */
	UE::Serialization::FEditorBulkData BulkData;

	/** GUID associated with the data stored herein. */
	FGuid Guid;

	/** Gets this bulk data hash */
	MESHDESCRIPTION_API FGuid GetHash() const;

	/** Take a copy of the bulk data versioning so it can be propagated to the bulk data reader when deserializing MeshDescription */
	FCustomVersionContainer CustomVersions;
	FPackageFileVersion UEVersion;
	int32 LicenseeUEVersion;

	/** Whether the bulk data has been written via SaveMeshDescription */
	bool bBulkDataUpdated;

	/** Uses hash instead of guid to identify content to improve DDC cache hit. */
	bool bGuidIsHash;
};
void SharedBufferToBulkData( FSharedBuffer& SharedBuffer, FByteBulkData& ToData )
{
	ToData.Lock( LOCK_READ_WRITE );
	void* WritePointer = ToData.Realloc( SharedBuffer.GetSize() );
	memcpy( WritePointer, SharedBuffer.GetData(), SharedBuffer.GetSize() );
	ToData.Unlock();
}
void ConvertBulkData( UE::Serialization::FEditorBulkData& FromData, FByteBulkData& ToData )
{
	FSharedBuffer SharedBuffer = FromData.GetPayload().Get();
	SharedBufferToBulkData( SharedBuffer, ToData );
}
enum
{
	// Engine raw mesh version:
	RAW_SKELETAL_MESH_BULKDATA_VER_INITIAL = 0,
	RAW_SKELETAL_MESH_BULKDATA_VER_AlternateInfluence = 1,
	RAW_SKELETAL_MESH_BULKDATA_VER_RebuildSystem = 2,
	RAW_SKELETAL_MESH_BULKDATA_VER_CompressMorphTargetData = 3,
	RAW_SKELETAL_MESH_BULKDATA_VER_VertexAttributes = 4,
	RAW_SKELETAL_MESH_BULKDATA_VER_Keep_Sections_Separate = 5,
	// Add new raw mesh versions here.

	RAW_SKELETAL_MESH_BULKDATA_VER_PLUS_ONE,
	RAW_SKELETAL_MESH_BULKDATA_VER = RAW_SKELETAL_MESH_BULKDATA_VER_PLUS_ONE - 1,

	// Licensee raw mesh version:
	RAW_SKELETAL_MESH_BULKDATA_LIC_VER_INITIAL = 0,
	// Licensees add new raw mesh versions here.

	RAW_SKELETAL_MESH_BULKDATA_LIC_VER_PLUS_ONE,
	RAW_SKELETAL_MESH_BULKDATA_LIC_VER = RAW_SKELETAL_MESH_BULKDATA_LIC_VER_PLUS_ONE - 1
};
FArchive& operator<<( FArchive& Ar, FSkeletalMeshImportData& RawMesh )
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4
	int32 Version = RAW_SKELETAL_MESH_BULKDATA_VER;
	int32 LicenseeVersion = RAW_SKELETAL_MESH_BULKDATA_LIC_VER;

	if( DowngraderSettings->TargetVersion <= EEngineVersion::EV_5_2 )//4.26->5..2
	{
		Version = RAW_SKELETAL_MESH_BULKDATA_VER_CompressMorphTargetData;
	}
	else if (DowngraderSettings->TargetVersion <= EEngineVersion::EV_Latest)
	{
		Version = RAW_SKELETAL_MESH_BULKDATA_VER_Keep_Sections_Separate;
	}
	
	Ar << Version;
	Ar << LicenseeVersion;

	/**
	* Serialization should use the raw mesh version not the archive version.
	* Additionally, stick to serializing basic types and arrays of basic types.
	*/
	bool bDummyFlag1 = false, bDummyFlag2 = false;

	Ar << bDummyFlag1;
	Ar << RawMesh.bHasNormals;
	Ar << RawMesh.bHasTangents;
	Ar << RawMesh.bHasVertexColors;
	Ar << bDummyFlag2;
	Ar << RawMesh.MaxMaterialIndex;
	Ar << RawMesh.NumTexCoords;

	Ar << RawMesh.Faces;
	Ar << RawMesh.Influences;
	Ar << RawMesh.Materials;
	Ar << RawMesh.Points;
	Ar << RawMesh.PointToRawMap;
	Ar << RawMesh.RefBonesBinary;
	Ar << RawMesh.Wedges;

	//In the old version this processing was done after we save the asset
	//We now save it after the processing is done so for old version we do it here when loading
	if( Ar.IsLoading() && Version < RAW_SKELETAL_MESH_BULKDATA_VER_AlternateInfluence )
	{
		//SkeletalMeshImportUtils::ProcessImportMeshInfluences( RawMesh, FString( TEXT( "Unknown" ) ) ); // Not sure how to get owning mesh name at this point...
	}


	if( Version >= RAW_SKELETAL_MESH_BULKDATA_VER_RebuildSystem )
	{
		Ar << RawMesh.MorphTargets;
		Ar << RawMesh.MorphTargetModifiedPoints;
		Ar << RawMesh.MorphTargetNames;
		Ar << RawMesh.AlternateInfluences;
		Ar << RawMesh.AlternateInfluenceProfileNames;
	}
	else if( Ar.IsLoading() )
	{
		RawMesh.MorphTargets.Empty();
		RawMesh.MorphTargetModifiedPoints.Empty();
		RawMesh.MorphTargetNames.Empty();
		RawMesh.AlternateInfluences.Empty();
		RawMesh.AlternateInfluenceProfileNames.Empty();
	}

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4
	if( Version >= RAW_SKELETAL_MESH_BULKDATA_VER_VertexAttributes )
	{
		Ar << RawMesh.VertexAttributes;
		Ar << RawMesh.VertexAttributeNames;
	}
	else
	{
		RawMesh.VertexAttributes.Empty();
		RawMesh.VertexAttributeNames.Empty();
	}
#endif

	if( Ar.IsLoading() && Version < RAW_SKELETAL_MESH_BULKDATA_VER_CompressMorphTargetData )
	{
		if( RawMesh.MorphTargetModifiedPoints.Num() != 0 )
		{
			//Compress the morph target data
			for( int32 MorphTargetIndex = 0; MorphTargetIndex < RawMesh.MorphTargets.Num(); ++MorphTargetIndex )
			{
				if( !RawMesh.MorphTargetModifiedPoints.IsValidIndex( MorphTargetIndex ) )
				{
					continue;
				}
				const TSet<uint32>& ModifiedPoints = RawMesh.MorphTargetModifiedPoints[ MorphTargetIndex ];
				FSkeletalMeshImportData& ToCompressShapeImportData = RawMesh.MorphTargets[ MorphTargetIndex ];
				TArray<FVector3f> CompressPoints;
				CompressPoints.Reserve( ToCompressShapeImportData.Points.Num() );
				for( uint32 PointIndex : ModifiedPoints )
				{
					CompressPoints.Add( ToCompressShapeImportData.Points[ PointIndex ] );
				}
				ToCompressShapeImportData.Points = CompressPoints;
			}
		}
	}

	if( Version >= RAW_SKELETAL_MESH_BULKDATA_VER_Keep_Sections_Separate )
	{
		bool bDummyFlag3 = false;
		Ar << bDummyFlag3;
	}
#endif
	return Ar;
}
#endif
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 0
EEngineVersion ToDowngraderEngineVersion( EUnrealEngineObjectUE5Version UE5Version )
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 0
	if( UE5Version < EUnrealEngineObjectUE5Version::LARGE_WORLD_COORDINATES )
#endif
		return EEngineVersion::EV_4_27;
	switch (UE5Version)
	{
		case EUnrealEngineObjectUE5Version::LARGE_WORLD_COORDINATES					: return EEngineVersion::EV_5_0;
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
		case EUnrealEngineObjectUE5Version::ADD_SOFTOBJECTPATH_LIST					: return EEngineVersion::EV_5_1;
#endif
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 2
		//5.2 and 5.3 are at the same level
		case EUnrealEngineObjectUE5Version::DATA_RESOURCES							: return EEngineVersion::EV_5_3;
#endif
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4
		case EUnrealEngineObjectUE5Version::PROPERTY_TAG_COMPLETE_TYPE_NAME			: return EEngineVersion::EV_5_4;
#endif
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5
		case EUnrealEngineObjectUE5Version::ASSETREGISTRY_PACKAGEBUILDDEPENDENCIES	: return EEngineVersion::EV_5_5;
#endif		
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 6
		default://for newer versions assume
		case EUnrealEngineObjectUE5Version::OS_SUB_OBJECT_SHADOW_SERIALIZATION		: return EEngineVersion::EV_5_7;//return EEngineVersion::EV_5_6;
#else
	default:return EEngineVersion::EV_4_27;
#endif
	}
};
#endif
void FixSkeletalMesh( USkeletalMesh* SkeletalMesh, EEngineVersion TargetVersion )
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4

	bool Modified = false;
	//TArray<FSkeletalMeshLODInfo>& LODInfoArray = SkeletalMesh->GetLODInfoArray();
	for( int i = 0; i < SkeletalMesh->GetLODNum(); i++)
	{
		//FSkeletalMeshLODInfo& LODInfo = LODInfoArray[ i ];
		FSkeletalMeshLODInfo* LODInfo = SkeletalMesh->GetLODInfo( i );
		//BoneInfluenceLimit doesn't exist in < 5.2
		if (DowngraderSettings->TargetVersion < EEngineVersion::EV_5_2)
		{
			if (LODInfo->BuildSettings.BoneInfluenceLimit == 0 || LODInfo->BuildSettings.BoneInfluenceLimit > 4)
			{
				LODInfo->BuildSettings.BoneInfluenceLimit = 4;
				Modified = true;
			}
		}
	}
	if( Modified )
	{
		SkeletalMesh->PostEditChange();
		SkeletalMesh->MarkPackageDirty();

		FinishAllCompilation();
	}

	RemovePropertyFlag( TEXT( "SkeletalMesh" ), TEXT( "MeshEditorDataObject" ), CPF_Deprecated );

	TObjectPtr<USkeletalMeshEditorData>& MeshEditorDataObject = GetVariable< TObjectPtr<USkeletalMeshEditorData>>( SkeletalMesh, TEXT( "SkeletalMesh" ), TEXT( "MeshEditorDataObject" ) );
	MeshEditorDataObject = NewObject<USkeletalMeshEditorData>( SkeletalMesh, NAME_None, RF_Transactional );

	static int BulkDatasOffset = 48;
	TArray<TSharedRef<FRawSkeletalMeshBulkData>>& RawSkeletalMeshBulkDatas = GetVariable<TArray<TSharedRef<FRawSkeletalMeshBulkData>>>( MeshEditorDataObject.Get(), BulkDatasOffset );

	if (DowngraderSettings->TargetVersion < EEngineVersion::EV_5_7)
	{
		TArray<FSkeletalMeshLODInfo>& LODInfo = GetVariable<TArray<FSkeletalMeshLODInfo>>( SkeletalMesh, TEXT( "SkeletalMesh" ), TEXT( "LODInfo" ) );
		LODInfo.Reset( 0 );
		for (int i = 0; i < SkeletalMesh->GetLODNum(); i++)
		{
			FSkeletalMeshLODInfo* Element = SkeletalMesh->GetLODInfo( i );
			LODInfo.Add( *Element );
		}
	}
	for( int i = 0; i < SkeletalMesh->GetNumSourceModels(); i++ )
	{
		FSkeletalMeshSourceModel& SourceModel = SkeletalMesh->GetSourceModel( i );

		RawSkeletalMeshBulkDatas.Add( MakeShared<FRawSkeletalMeshBulkData>() );
		FRawSkeletalMeshBulkData& RawBulkData = RawSkeletalMeshBulkDatas[ i ].Get();

		TObjectPtr<USkeletalMeshDescriptionBulkData>& SkeletalMeshDescriptionBulkData = GetVariable < TObjectPtr<USkeletalMeshDescriptionBulkData>>( &SourceModel, TEXT( "SkeletalMeshSourceModel" ), TEXT( "MeshDescriptionBulkData" ) );

		FMeshDescriptionBulkData* MeshDescriptionBulkData = ( FMeshDescriptionBulkData* )SourceModel.GetMeshDescriptionBulkData();

		FMeshDescriptionBulkData_Downgrader* MeshDescriptionBulkData_Downgrader = (FMeshDescriptionBulkData_Downgrader*)MeshDescriptionBulkData;
		FRawSkeletalMeshBulkData_Downgrader* RawSkeletalMeshBulkData_Downgrader = (FRawSkeletalMeshBulkData_Downgrader*)&RawBulkData;

		RawSkeletalMeshBulkData_Downgrader->Guid = MeshDescriptionBulkData_Downgrader->Guid;
		RawSkeletalMeshBulkData_Downgrader->SerializeLoadingCustomVersionContainer = MeshDescriptionBulkData_Downgrader->CustomVersions;
		RawSkeletalMeshBulkData_Downgrader->UEVersion = MeshDescriptionBulkData_Downgrader->UEVersion;
		RawSkeletalMeshBulkData_Downgrader->LicenseeUEVersion = MeshDescriptionBulkData_Downgrader->LicenseeUEVersion;
		
		FSharedBuffer OldBytes = MeshDescriptionBulkData_Downgrader->BulkData.GetPayload().Get();
		if( OldBytes.GetSize() == 0 )
		{
			//if this is 0, RawMeshBulkData is probably valid because the data is from a previous version
			continue;
		}
		EEngineVersion MeshEngineVersion = ToDowngraderEngineVersion( (EUnrealEngineObjectUE5Version)MeshDescriptionBulkData_Downgrader->UEVersion.FileVersionUE5 );
		if( MeshEngineVersion < EEngineVersion::EV_5_4 )
			break;
		FMemoryReaderView Reader( OldBytes.GetView(), true /* bIsPersistent */ );
		MeshDescriptionBulkData_Downgrader->BulkData.UnloadData();
		Reader.SetUEVer( MeshDescriptionBulkData_Downgrader->UEVersion );//Should put the old version in ?
		Reader.SetLicenseeUEVer( MeshDescriptionBulkData_Downgrader->LicenseeUEVersion );
		Reader.SetCustomVersions( MeshDescriptionBulkData_Downgrader->CustomVersions );
		FMeshDescription MeshDescription;
		MeshDescription.Empty();
		Reader << MeshDescription;

		FSkeletalMeshImportData SkeletalMeshImportData = FSkeletalMeshImportData::CreateFromMeshDescription( MeshDescription );

		FLargeMemoryWriter NewBytes( OldBytes.GetSize(), true /* bIsPersistent */ );

		int PrevVersion = GetCustomVersion( FEditorObjectVersion::GUID );
		if( DowngraderSettings->TargetVersion <= EEngineVersion::EV_4_27 )
		{
			//In SkeletalMeshLODImporterData.cpp, line 741, archive gets the custom versions from the file, which has Dev-Editor at 27
			FCustomVersion* EditorObjectVersion = (FCustomVersion*)RawSkeletalMeshBulkData_Downgrader->SerializeLoadingCustomVersionContainer.GetVersion( FEditorObjectVersion::GUID );
			if (EditorObjectVersion)
				EditorObjectVersion->Version = FEditorObjectVersion::ChangeSceneCaptureRootComponent;
			//Requires this for 4.27 otherwise it crashes on save in 4.27
			ChangeCustomVersion( FEditorObjectVersion::GUID, FEditorObjectVersion::ChangeSceneCaptureRootComponent, TEXT( "Dev-Editor" ) );
		}

		NewBytes << SkeletalMeshImportData;

		//Revert back, we're not in spoof mode yet
		ChangeCustomVersion( FEditorObjectVersion::GUID, PrevVersion, TEXT( "Dev-Editor" ) );

		uint64 NumBytes = static_cast<uint64>( NewBytes.TotalSize() );

		FSharedBuffer Buff = FSharedBuffer::TakeOwnership( NewBytes.ReleaseOwnership(), NumBytes, FMemory::Free );
		SharedBufferToBulkData( Buff, RawSkeletalMeshBulkData_Downgrader->BulkData );
		
		static int Offset = 164;
		bool& bBulkDataUpdated = GetVariable< bool>( MeshDescriptionBulkData, Offset );
		if ( bBulkDataUpdated == 0 )
			bBulkDataUpdated = true;
		else if ( bBulkDataUpdated != 1 )
		{
			ensureMsgf( false, TEXT( "FixSkeletalMesh error -> bBulkDataUpdated bad offset ??" ) );
		}
	}
	if( DowngraderSettings->TargetVersion < EEngineVersion::EV_5_4 )
	{
		TArray<FSkeletalMeshSourceModel>& SourceModels = GetVariable< TArray<FSkeletalMeshSourceModel>>( SkeletalMesh, TEXT( "SkeletalMesh" ), TEXT( "SourceModels" ) );
		SourceModels.Reset( 0 );
	}

	UAssetImportData* AssetImportData = SkeletalMesh->GetAssetImportData();
	UInterchangeAssetImportData* InterchangeAssetImportData = Cast< UInterchangeAssetImportData>( AssetImportData );
	if( InterchangeAssetImportData )
	{
		RemovePropertyFlag( TEXT( "InterchangeAssetImportData" ), TEXT( "NodeContainer" ), CPF_Deprecated );
		RemovePropertyFlag( TEXT( "InterchangeAssetImportData" ), TEXT( "Pipelines" ), CPF_Deprecated );

		if( DowngraderSettings->TargetVersion == EEngineVersion::EV_5_1 ||
			DowngraderSettings->TargetVersion == EEngineVersion::EV_5_2 )
		{
			TObjectPtr<UInterchangeBaseNodeContainer>& NodeContainer = GetVariable<TObjectPtr<UInterchangeBaseNodeContainer>>( InterchangeAssetImportData, TEXT( "InterchangeAssetImportData" ), TEXT( "NodeContainer" ) );

			if( !NodeContainer.Get() )
			{
				NodeContainer = NewObject<UInterchangeBaseNodeContainer>( InterchangeAssetImportData );
			}
		}
	}
	UFbxSkeletalMeshImportData* FbxSkeletalMeshImportData = Cast< UFbxSkeletalMeshImportData>( AssetImportData );
	if ( FbxSkeletalMeshImportData )
	{
		//if (Ar.CustomVer( FEditorObjectVersion::GUID ) < FEditorObjectVersion::ComputeWeightedNormals)
		if (DowngraderSettings->TargetVersion <= EEngineVersion::EV_4_27)
		{
			//Old asset are not using weighted normals
			FbxSkeletalMeshImportData->bComputeWeightedNormals = false;
		}
	}	
#endif
}
void FixSkeleton( USkeleton* Skeleton )
{
	//< FUE5MainStreamObjectVersion::AnimationRemoveSmartNames
	if( DowngraderSettings->TargetVersion < EEngineVersion::EV_5_3 )
	{
		#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
		UAnimCurveMetaData* AnimCurveMetaData = Skeleton->GetAssetUserData<UAnimCurveMetaData>();
		
		FSmartNameContainer& SmartNames = GetVariable<FSmartNameContainer>( Skeleton, TEXT( "Skeleton" ), TEXT( "SmartNames" ) );
		static int NameMappingsOffset = 0;
		TMap<FName, FSmartNameMapping>& NameMappings = GetVariable< TMap<FName, FSmartNameMapping> >( &SmartNames, NameMappingsOffset );

		NameMappings.Reset();

		FSmartNameMapping Mapping;
		static int CurveMetaDataMapOffset = 16;
		TMap<FName, FCurveMetaData>& CurveMetaDataMap = GetVariable< TMap<FName, FCurveMetaData> >( &Mapping, CurveMetaDataMapOffset );

		AnimCurveMetaData->ForEachCurveMetaData(
			[&]( const FName& InCurveName, const FCurveMetaData& InMetaData )
			{
				CurveMetaDataMap.Add( InCurveName, InMetaData );
			});

		NameMappings.Add( TEXT("AnimationCurves"), Mapping);
		#endif
	}
	//Is this required since the above fix looks more complete ?
	//if (DowngraderSettings->TargetVersion < EEngineVersion::EV_5_3)
	//{
	//	RemovePropertyFlag( TEXT( "Skeleton" ), TEXT( "SmartNames" ), CPF_Deprecated );
	//
	//	FSmartNameContainer& SmartNames = GetVariable<FSmartNameContainer>( Skeleton, TEXT( "Skeleton" ), TEXT( "SmartNames" ) );
	//	//Constructor in 4.27
	//	FName AnimCurveMappingName = FName( TEXT( "AnimationCurves" ) );//USkeleton::AnimCurveMappingName
	//	SmartNames.AddContainer( AnimCurveMappingName );
	//}
}
template<class RET, class TYPE>
RET ObjectPtrGetter( TYPE& in)
{
	#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
		return in.Get();
	#else
		return in;
	#endif
}
void FixMapBuildData( UMapBuildDataRegistry* MapBuildDataRegistry )
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
	static int MeshMapBuildDataOffset = 56;//Valid for 5.4-5.5
	TMap<FGuid, FMeshMapBuildData>& MeshBuildData = GetVariable< TMap<FGuid, FMeshMapBuildData>>( MapBuildDataRegistry, MeshMapBuildDataOffset );

	TArray<UTexture*> Textures;
	for( auto It = MeshBuildData.CreateIterator(); It; ++It )
	{
		FMeshMapBuildData& Data = It.Value();
		if( Data.LightMap )
		{
			FLightMap2D* LightMapData = Data.LightMap->GetLightMap2D();

			for( int i = 0; i < 2; i++ )
			{
				Textures.Add( LightMapData->Textures[ i ] );
				#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
					ULightMapVirtualTexture2D* VTex = LightMapData->VirtualTextures[ i ].Get();
				#else
					ULightMapVirtualTexture2D* VTex = LightMapData->VirtualTextures[ i ];
				#endif
					Textures.Add( VTex );
			}
	
			#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
				Textures.Add( LightMapData->SkyOcclusionTexture.Get() );
				Textures.Add( LightMapData->AOMaterialMaskTexture.Get() );
				Textures.Add( LightMapData->ShadowMapTexture.Get() );
			#else
				Textures.Add( LightMapData->SkyOcclusionTexture );
				Textures.Add( LightMapData->AOMaterialMaskTexture );
				Textures.Add( LightMapData->ShadowMapTexture );
			#endif
		}
		if( Data.ShadowMap.GetReference() && Data.ShadowMap.GetReference()->GetShadowMap2D() )
		{
			FShadowMap2D* ShadowMap2D = Data.ShadowMap.GetReference()->GetShadowMap2D();
			Textures.Add( ShadowMap2D->GetTexture() );
		}
	}

	for (int i = 0; i < Textures.Num(); i++)
	{
		UTexture* Tex = Textures[i];
		//static int BulkDataOffset = 56;
		//UE::Serialization::FEditorBulkData& BulkData = GetVariable< UE::Serialization::FEditorBulkData>( &Tex->Source, BulkDataOffset );
		//BulkData.UpdateKeyIfNeeded();
		FixTexture( Tex );
	}

	TArray< UTextureCube* > CubeTextures;
	EObjectFlags TopLevelFlags = RF_Standalone;
	ForEachObjectWithPackage( MapBuildDataRegistry->GetPackage(), [&]( UObject* InObject )
	{
		if( InObject->HasAnyFlags( TopLevelFlags ) )
		{
			UTextureCube* CubeTex = Cast<UTextureCube>( InObject );
			if( CubeTex )
			{
				CubeTextures.Add( CubeTex );
			}
		}
		return true;
	}, true/*bIncludeNestedObjects */, RF_Transient );

	for( int i = 0; i < CubeTextures.Num(); i++ )
	{
		UTextureCube* CubeTex = CubeTextures[ i ];
		//try to destroy it
		//CubeTex->MarkAsGarbage();
		CubeTex->Rename( nullptr, GetTransientPackage() );// , REN_DoNotDirty | REN_DontCreateRedirectors );
	}
#endif
}
//bool HasConvexHullData( const FManagedArrayCollection* Collection )
//{
//	return Collection->HasAttribute( "TransformToConvexIndices", FTransformCollection::TransformGroup ) && Collection->HasAttribute( FGeometryCollection::ConvexHullAttribute, FGeometryCollection::ConvexGroup );
//}

void FixGeometryCollection( UGeometryCollection* Collection )
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
	if( DowngraderSettings->TargetVersion <= EEngineVersion::EV_4_27 )
	{	
		for( int i = 0; i < Collection->GeometrySource.Num(); i++ )
		{
			FGeometryCollectionSource& Source = Collection->GeometrySource[ i ];
			//Source
		}

		#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4
			Collection->CacheMaterialDensity();
		#endif

		IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable( TEXT( "p.GeometryCollectionEnableForcedConvexGenerationInSerialize" ) );

		if( CVar && CVar->GetBool() )
		{
			if( //!HasConvexHullData( GeometryCollectionPtr.Get() ) &&
				GeometryCollection::SizeSpecific::UsesImplicitCollisionType( Collection->SizeSpecificData, EImplicitTypeEnum::Chaos_Implicit_Convex ) )
			{
				GeometryCollection::SizeSpecific::SetImplicitCollisionType( Collection->SizeSpecificData, EImplicitTypeEnum::Chaos_Implicit_Box, EImplicitTypeEnum::Chaos_Implicit_Convex );
				//Collection->bCreateSimulationData = true;
				Collection->InvalidateCollection();
			}
		}

		TSharedPtr<FGeometryCollection, ESPMode::ThreadSafe> GeometryCollectionPtr = Collection->GetGeometryCollection();

		FGeometryCollection* BaseCollection = GeometryCollectionPtr.Get();
		if( BaseCollection )
		{
			#ifdef DOWNGRADER_CUSTOM_ENGINE
			TMap< FGeometryCollection::FKeyType, FGeometryCollection::FValueType>& Map = BaseCollection->Map;
			bool Removals = false;
			do
			{
				Removals = false;
				int LastValueFor427 = (int)FManagedArrayCollection::EArrayType::FTPBDRigidClusteredParticleHandle3fPtrType;
				for( auto It = Map.CreateIterator(); It; ++It )
				{
					FGeometryCollection::FValueType& Val = It.Value();
					if( Val.ArrayType == FManagedArrayCollection::EArrayType::FTransform3fType )
					{
						Val.ArrayType = FManagedArrayCollection::EArrayType::FTransformType;
					}
					else if( (int)Val.ArrayType > LastValueFor427 )
					{
						Map.Remove( It.Key() );
						Removals = true;
						break;
					}
				}
			} while( Removals );
			#endif
		}
		if( Collection->ImplicitType_DEPRECATED == EImplicitTypeEnum::Chaos_Implicit_Convex )
		{
			Collection->ImplicitType_DEPRECATED = EImplicitTypeEnum::Chaos_Implicit_None;
		}
		if( Collection->SizeSpecificData.Num() > 0 && Collection->SizeSpecificData[ 0 ].ImplicitType_DEPRECATED == EImplicitTypeEnum::Chaos_Implicit_Convex )
		{
			Collection->SizeSpecificData[ 0 ].ImplicitType_DEPRECATED = EImplicitTypeEnum::Chaos_Implicit_None;
		}
	}
	if( DowngraderSettings->TargetVersion <= EEngineVersion::EV_5_0 )
	{
		Collection->PerClusterOnlyDamageThreshold = true;
		Collection->DamagePropagationData.bEnabled = false;
	}
	if( DowngraderSettings->TargetVersion <= EEngineVersion::EV_5_2 )
	{
		Collection->bConvertVertexColorsToSRGB = false;
	}
	#endif
}
void FixPhysicsAsset(UPhysicsAsset* PhysicsAsset )
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
	if( DowngraderSettings->TargetVersion <= EEngineVersion::EV_5_2 )
	{
		PhysicsAsset->SolverSettings.PositionIterations = PhysicsAsset->SolverIterations.SolverIterations * PhysicsAsset->SolverIterations.JointIterations;
		PhysicsAsset->SolverSettings.VelocityIterations = 1;
		PhysicsAsset->SolverSettings.ProjectionIterations = PhysicsAsset->SolverIterations.SolverPushOutIterations;
		PhysicsAsset->SolverSettings.bUseLinearJointSolver = false;
		PhysicsAsset->SolverSettings.CullDistance = 1.0f;
	}
	if( DowngraderSettings->TargetVersion <= EEngineVersion::EV_5_5 )
	{
		#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5
			PhysicsAsset->SolverSettings.bUseManifolds = false;
		#endif
	}
#endif
}
class UDataflow;
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5
	using namespace UE;
#endif
void FixDataFlow( UDataflow* DataFlow )
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 2
	Dataflow::FGraph* Graph = DataFlow->Dataflow.Get();
	TArray< TSharedPtr<FDataflowNode> >& Nodes = Graph->GetNodes();
	for( int i = 0; i < Nodes.Num(); i++ )
	{
		FDataflowNode* Node = Nodes[ i ].Get();
		//FChaosClothAssetUSDImportNode* ChaosClothAssetUSDImportNode =
		FName Type = Node->GetType();
		if( DowngraderSettings->TargetVersion <= EEngineVersion::EV_5_2 )
		{
			if( Type == FName( TEXT( "FChaosClothAssetUSDImportNode" ) ) ||
				Type == FName( TEXT( "FChaosClothAssetTerminalNode" ) ) ||
				Type == FName( TEXT( "FChaosClothAssetAddWeightMapNode" ) ) ||
				Type == FName( TEXT( "FChaosClothAssetTransferSkinWeightsNode" ) ) ||
				Type == FName( TEXT( "FChaosClothAssetProxyDeformerNode" ) )

				)
			{
				Graph->RemoveNode( Nodes[ i ] );
				i--;
			}
		}
	}
#endif
}
class UInputMappingContext;
void FixInputMappingContext( UInputMappingContext* InputMappingContext )
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 7
	if (DowngraderSettings->TargetVersion < EEngineVersion::EV_5_7)
	{
		TArray<FEnhancedActionKeyMapping>& Mappings = GetVariable< TArray<FEnhancedActionKeyMapping> >( InputMappingContext, TEXT( "InputMappingContext" ), TEXT( "Mappings" ) );
		FInputMappingContextMappingData& DefaultKeyMappings = GetVariable< FInputMappingContextMappingData >( InputMappingContext, TEXT( "InputMappingContext" ), TEXT( "DefaultKeyMappings" ) );

		Mappings = DefaultKeyMappings.Mappings;
	}
#endif
}
class UMetaSoundSource;
void FixMetaSound( UMetaSoundSource* MetaSoundSource )
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 6
	if (DowngraderSettings->TargetVersion < EEngineVersion::EV_5_6)
	{
		FMetasoundFrontendDocument& RootMetasoundDocument = GetVariable< FMetasoundFrontendDocument>( MetaSoundSource, TEXT( "MetaSoundSource" ), TEXT( "RootMetasoundDocument" ) );

		//FMetasoundFrontendDocument& Document = MetaSoundSource->GetDocumentChecked();

		TArray<FMetasoundFrontendGraph>& PagedGraphs = GetVariable< TArray<FMetasoundFrontendGraph> >( &RootMetasoundDocument.RootGraph, TEXT( "MetasoundFrontendGraphClass" ), TEXT( "PagedGraphs" ) );
		if (PagedGraphs.Num() == 0)
			return;
		// Access the root graph where comments are stored
		FMetasoundFrontendGraph& Graph = PagedGraphs[0];

		UScriptStruct* ScriptStruct = FMetaSoundFrontendGraphComment::StaticStruct();
		FStructProperty* PositionProperty = FindFProperty<FStructProperty>( ScriptStruct, TEXT( "Position" ) );
		FStructProperty* SizeProperty = FindFProperty<FStructProperty>( ScriptStruct, TEXT( "Size" ) );

		for (auto it = Graph.Style.Comments.CreateConstIterator(); it; ++it)
		{
			FGuid GUID = it.Key();
			FMetaSoundFrontendGraphComment& Comment = (FMetaSoundFrontendGraphComment&)it.Value();
			void* ValuePtr = nullptr;
			if (PositionProperty)
			{
				ValuePtr = PositionProperty->ContainerPtrToValuePtr<void>( &Comment );
				if (ValuePtr)
				{
					FIntVector2* Vec = (FIntVector2*)ValuePtr;
					FVector2D Vec2DTemp;
					Vec2DTemp.X = Vec->X;
					Vec2DTemp.Y = Vec->Y;
					FVector2D* Vec2D = (FVector2D*)ValuePtr;
					*Vec2D = Vec2DTemp;
					//UE_LOG( LogTemp, Error, TEXT( "%d %d" ), Vec->X, Vec->Y );
				}
				PositionProperty->Struct = TBaseStructure<FVector2D>::Get();
			}
			if (SizeProperty)
			{
				ValuePtr = SizeProperty->ContainerPtrToValuePtr<void>( &Comment );
				if (ValuePtr)
				{
					FIntVector2* Vec = (FIntVector2*)ValuePtr;
					FVector2D Vec2DTemp;
					Vec2DTemp.X = Vec->X;
					Vec2DTemp.Y = Vec->Y;
					FVector2D* Vec2D = (FVector2D*)ValuePtr;
					*Vec2D = Vec2DTemp;
					//UE_LOG( LogTemp, Error, TEXT( "%d %d" ), Vec->X, Vec->Y );
				}
				SizeProperty->Struct = TBaseStructure<FVector2D>::Get();
			}
		}
	}
#endif
}
class UGroomAsset;
void FixGroomAsset( UGroomAsset* GroomAsset )
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 7
	if (DowngraderSettings->TargetVersion < EEngineVersion::EV_5_4)
	{
		for (FHairGroupsInterpolation& Group : GroomAsset->GetHairGroupsInterpolation())
		{
			if (Group.InterpolationSettings.GuideType == EGroomGuideType::Rigged )
			{
				Group.RiggingSettings.bEnableRigging_DEPRECATED = true;
				Group.InterpolationSettings.bOverrideGuides_DEPRECATED = true;
			}
			else if (Group.InterpolationSettings.GuideType == EGroomGuideType::Generated)
			{
				Group.InterpolationSettings.bOverrideGuides_DEPRECATED = true;
			}

			Group.RiggingSettings.NumCurves_DEPRECATED = Group.InterpolationSettings.RiggedGuideNumCurves;
			Group.RiggingSettings.NumPoints_DEPRECATED = Group.InterpolationSettings.RiggedGuideNumPoints;			
		}

		// Convert cards
		for (FHairGroupsCardsSourceDescription& Group : GroomAsset->GetHairGroupsCards())
		{
			// Convert old textures
			if (Group.Textures.Layout == EHairTextureLayout::Layout2 ||
				Group.Textures.Layout == EHairTextureLayout::Layout3)
			{
				if (Group.Textures.Textures.Num() > 0)
					Group.Textures.TangentTexture_DEPRECATED = Group.Textures.Textures[0];
				if (Group.Textures.Textures.Num() > 1)
					Group.Textures.AttributeTexture_DEPRECATED = Group.Textures.Textures[1];
				if (Group.Textures.Textures.Num() > 2)
					Group.Textures.MaterialTexture_DEPRECATED = Group.Textures.Textures[2];
			}
			if (Group.Textures.Layout == EHairTextureLayout::Layout0)
			{
				if (Group.Textures.Textures.Num() > 0)
					Group.Textures.DepthTexture_DEPRECATED = Group.Textures.Textures[0];
				if (Group.Textures.Textures.Num() > 1)
					Group.Textures.CoverageTexture_DEPRECATED = Group.Textures.Textures[1];
				if (Group.Textures.Textures.Num() > 2)
					Group.Textures.TangentTexture_DEPRECATED = Group.Textures.Textures[2];
				if (Group.Textures.Textures.Num() > 3)
					Group.Textures.AttributeTexture_DEPRECATED = Group.Textures.Textures[3];
				if (Group.Textures.Textures.Num() > 4)
					Group.Textures.MaterialTexture_DEPRECATED = Group.Textures.Textures[4];
				if (Group.Textures.Textures.Num() > 5)
					Group.Textures.AuxilaryDataTexture_DEPRECATED = Group.Textures.Textures[5];
			}
		}

		// Convert meshes
		for (FHairGroupsMeshesSourceDescription& Group : GroomAsset->GetHairGroupsMeshes())
		{
			// Convert old textures
			if (Group.Textures.Layout == EHairTextureLayout::Layout2 ||
				Group.Textures.Layout == EHairTextureLayout::Layout3)
			{
				if (Group.Textures.Textures.Num() > 0)
					Group.Textures.TangentTexture_DEPRECATED = Group.Textures.Textures[0];
				if (Group.Textures.Textures.Num() > 1)
					Group.Textures.DepthTexture_DEPRECATED = Group.Textures.Textures[1];
				if (Group.Textures.Textures.Num() > 2)
					Group.Textures.CoverageTexture_DEPRECATED = Group.Textures.Textures[2];
			}
			if (Group.Textures.Layout == EHairTextureLayout::Layout1)
			{
				if (Group.Textures.Textures.Num() > 0)
					Group.Textures.DepthTexture_DEPRECATED = Group.Textures.Textures[0];
				if (Group.Textures.Textures.Num() > 1)
					Group.Textures.CoverageTexture_DEPRECATED = Group.Textures.Textures[1];
				if (Group.Textures.Textures.Num() > 2)
					Group.Textures.TangentTexture_DEPRECATED = Group.Textures.Textures[2];
				if (Group.Textures.Textures.Num() > 3)
					Group.Textures.AttributeTexture_DEPRECATED = Group.Textures.Textures[3];
				if (Group.Textures.Textures.Num() > 4)
					Group.Textures.MaterialTexture_DEPRECATED = Group.Textures.Textures[4];
				if (Group.Textures.Textures.Num() > 5)
					Group.Textures.AuxilaryDataTexture_DEPRECATED = Group.Textures.Textures[5];
			}
		}
	}
#endif
}
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 0
#include "ExternalPackageHelper.h"
TArray<FAssetData> GetActorAssetFiles( const FString& ExternalActorsPath )
{
	TArray<FAssetData> ActorAssets;
	if( !ExternalActorsPath.IsEmpty() )
	{
		FARFilter Filter;
		Filter.bIncludeOnlyOnDiskAssets = true;
		Filter.PackagePaths.Add( *ExternalActorsPath );
		Filter.bRecursivePaths = true;

		
		IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>( TEXT( "AssetRegistry" ) ).Get();
		#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
			AssetRegistry.ScanSynchronous( { ExternalActorsPath }, TArray<FString>() );
		#endif

		#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4
			FExternalPackageHelper::GetSortedAssets( Filter, ActorAssets );
		#endif
	}
	return ActorAssets;
}

TArray<FAssetData> GetActorAssetFilesForLevel( ULevel* Level, bool bTryUsingPackageLoadedPath )
{
	UWorld* World = Level->GetTypedOuter<UWorld>();
	FString LevelPackageName;
	FString LevelPackageShortName;
	if( bTryUsingPackageLoadedPath && !World->GetPackage()->GetLoadedPath().IsEmpty() )
	{
		LevelPackageName = World->GetPackage()->GetLoadedPath().GetPackageName();
	}
	else
	{
		LevelPackageName = World->GetPackage()->GetName();
		LevelPackageShortName = !World->OriginalWorldName.IsNone() ? World->OriginalWorldName.ToString() : World->GetName();
	}
	TArray<FAssetData> ActorAssets;
	TArray<FAssetData> ObjectAssets;
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4
	TArray<FString> ActorsPaths = ULevel::GetExternalActorsPaths( LevelPackageName, LevelPackageShortName );
	TArray<FString> ObjectsPaths = ULevel::GetExternalObjectsPaths( LevelPackageName, LevelPackageShortName );
	for( const FString& Path : ActorsPaths )
	{
		ActorAssets = GetActorAssetFiles( Path );
	}
	for( const FString& Path : ObjectsPaths )
	{
		ObjectAssets = GetActorAssetFiles( Path );
	}

	for( int i = 0; i < ObjectAssets.Num(); i++ )
	{
		ActorAssets.Add( ObjectAssets[ i ] );
	}
#endif
	return ActorAssets;
}
#endif
void RemoveWorldPartition( FAssetData& Asset )
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 2
	UObject* AssetObject = Asset.GetAsset();
	if( AssetObject )
	{
		UWorld* World = Cast< UWorld >( AssetObject );

		UWorldPartition* WorldPartition = World->GetWorldSettings()->GetWorldPartition();
		if( WorldPartition )
		{
			if( WorldPartition->IsStreamingEnabled() )
				WorldPartition->SetEnableStreaming( false );

			UWorldPartition::RemoveWorldPartition( World->GetWorldSettings() );

			FString Extension = AssetObject->IsA<UWorld>() ? FPackageName::GetMapPackageExtension() : FPackageName::GetAssetPackageExtension();
			SavePackage( AssetObject->GetPackage(), Asset.PackageName.ToString(), Extension );
		}
	}
#endif
}
void FixWorld( UWorld* world )
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
	bool Modified = false;
	AWorldSettings* WorldSettings = world->GetWorldSettings();
	if( WorldSettings )
	{
		UNavigationSystemConfig* Config = WorldSettings->GetNavigationSystemConfig();
		Config->NavigationSystemClass = FSoftClassPath( TEXT( "Script/NavigationSystem.NavigationSystemV1" ) );
		Modified = true;

		UWorldPartition* Partition = WorldSettings->GetWorldPartition();
		if( Partition )
		{
			UWorldPartitionRuntimeHash* Hash = Partition->RuntimeHash.Get();
			UWorldPartitionRuntimeSpatialHash* SpatialHash = Cast< UWorldPartitionRuntimeSpatialHash>( Hash );
			if ( SpatialHash )			
			{
				EWorldPartitionCVarProjectDefaultOverride& UseAlignedGridLevels = GetVariable< EWorldPartitionCVarProjectDefaultOverride>( SpatialHash, TEXT("WorldPartitionRuntimeSpatialHash"), TEXT("UseAlignedGridLevels"));
				EWorldPartitionCVarProjectDefaultOverride& SnapNonAlignedGridLevelsToLowerLevels = GetVariable< EWorldPartitionCVarProjectDefaultOverride>( SpatialHash, TEXT( "WorldPartitionRuntimeSpatialHash" ), TEXT( "SnapNonAlignedGridLevelsToLowerLevels" ) );
				EWorldPartitionCVarProjectDefaultOverride& PlaceSmallActorsUsingLocation = GetVariable< EWorldPartitionCVarProjectDefaultOverride>( SpatialHash, TEXT( "WorldPartitionRuntimeSpatialHash" ), TEXT( "PlaceSmallActorsUsingLocation" ) );
				EWorldPartitionCVarProjectDefaultOverride& PlacePartitionActorsUsingLocation = GetVariable< EWorldPartitionCVarProjectDefaultOverride>( SpatialHash, TEXT( "WorldPartitionRuntimeSpatialHash" ), TEXT( "PlacePartitionActorsUsingLocation" ) );

				UseAlignedGridLevels = EWorldPartitionCVarProjectDefaultOverride::ProjectDefault;
				SnapNonAlignedGridLevelsToLowerLevels = EWorldPartitionCVarProjectDefaultOverride::ProjectDefault;
				PlaceSmallActorsUsingLocation = EWorldPartitionCVarProjectDefaultOverride::ProjectDefault;
				PlacePartitionActorsUsingLocation = EWorldPartitionCVarProjectDefaultOverride::ProjectDefault;
			}
		}
	}
	for( TActorIterator<AActor> Iterator( world ); Iterator; ++Iterator )
	{
		AActor* Actor = *Iterator;
		Modified = FixActor( Actor );
	}
	TArray<FString> ActorPackageNames = world->PersistentLevel->GetOnDiskExternalActorPackages();
	TArray<UPackage*> ExternalObjectPackages = world->PersistentLevel->GetLoadedExternalObjectPackages();
	
	//for( int i = 0; i < ActorPackageNames.Num(); i++ )
	//{
	//	FString PackageName = ActorPackageNames[ i ];
	//}
	for( int i = 0; i < ExternalObjectPackages.Num(); i++ )
	{
		UPackage* Package = ExternalObjectPackages[ i ];
		TArray<UObject*> Objects;
		GetObjectsWithOuter( Package, Objects, false );
	}

	const TArray<ULevel*>& Levels = world->GetLevels();
	for( int i = 0; i < Levels.Num(); i++ )
	{
		ULevel* Level = ( ULevel*)Levels[ i ];
		if( DowngraderSettings->TargetVersion <= EEngineVersion::EV_4_27 )
		{
			if ( Level->IsUsingExternalActors() )
				//External actors don't exist in 4.27 so disable it here
				Level->ConvertAllActorsToPackaging( false );
		}
		if( Level->LevelScriptBlueprint )
		{
			ULevelScriptBlueprint* LevelBlueprint = Level->LevelScriptBlueprint;
			FixBlueprint( LevelBlueprint );
		}
	}
	TArray<UObject*> Objects;
	GetObjectsWithOuter( world, Objects, false );
	GetObjectsWithOuter( world->GetPackage(), Objects, false);
	for( int i = 0; i < Objects.Num(); i++ )
	{
		UTexture2D* Tex2D = Cast< UTexture2D >( Objects [ i ]);
		if( Tex2D )
		{
			FixTexture( Tex2D );
		}
		UMapBuildDataRegistry* MapBuildDataRegistry = Cast< UMapBuildDataRegistry >( Objects[ i ] );
		if ( MapBuildDataRegistry )
		{
			FixMapBuildData( MapBuildDataRegistry );
		}
	}
	if( Modified )
	{
		world->PostEditChange();
		world->MarkPackageDirty();
	}
#endif
}

bool PreparePackage( UObject* AssetObject, UPackage* Package, FName PackageName, FString AssetPath, EEngineVersion CurrentVersion,
					 AssetForModification* NewAssetForModification, TArray<FString>& Messages )
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
#if WITH_EDITOR
	// Perforce Support
	auto& SourceControlProvider = ISourceControlModule::Get().GetProvider();
	SourceControlProvider.Init();

	if( SourceControlProvider.IsEnabled() )
	{
		auto PackageState = SourceControlProvider.GetState( Package, EStateCacheUsage::ForceUpdate );
		if( PackageState.IsValid() && PackageState->CanCheckout() )
		{
			const ECommandResult::Type Result = SourceControlProvider.Execute( ISourceControlOperation::Create<FCheckOut>(), Package );
			if( Result != ECommandResult::Succeeded )
			{
				UE_LOG( LogTemp, Fatal, TEXT( "Package %s failed to checkout." ), *PackageName.ToString() );
			}
		}

		auto CurrentState = SourceControlProvider.GetState( Package, EStateCacheUsage::ForceUpdate );
		if( !CurrentState->IsLocal() && !CurrentState->CanEdit() )
		{
			UE_LOG( LogTemp, Error, TEXT( "Package %s is not available to edit." ), *Package->GetName() );
		}
	}
#endif
	//int PackageSummarySize = 0;
	//int PackageCustomVersions = 0;
	
	FString Extension = AssetObject->IsA<UWorld>() ? FPackageName::GetMapPackageExtension() : FPackageName::GetAssetPackageExtension();
	FString FilePath = FPackageName::LongPackageNameToFilename( PackageName.ToString(), Extension );

	if (FPlatformFileManager::Get().GetPlatformFile().IsReadOnly( *FilePath ))
	{
		bool Success = FPlatformFileManager::Get().GetPlatformFile().SetReadOnly( *FilePath, false );
		if (!Success)
		{
			UE_LOG( LogTemp, Error, TEXT( "SetReadOnly Failed for %s" ), *Package->GetName() );
		}
	}

	//PackageReader not available to link in 5.0

	{
		//First save asset so it has all the "latest" custom versions, as even with same packageID some custom versions may be missing
		//Set FEngineVersion to CurrentVersion since in saving it's possible to get asset loads
		SpoofEngineVersion( CurrentVersion );//DowngraderSettings->TargetVersion );
		RemoveBlueprintRedirects();
		SavePackage( Package, PackageName.ToString(), Extension );
		//SpoofEngineVersion( CurrentVersion );
		RemoveBlueprintRedirects( true );

		/*FPackageReader PackageReader;
		FPackageReader::EOpenPackageResult OpenPackageResult;
		//FString AssetPath = Asset.GetSoftObjectPath().ToString();// Was Asset.ObjectPath.ToString(); previously
		ReadSuccess = PackageReader.OpenPackageFile( AssetPath, FilePath, &OpenPackageResult );
		if( ReadSuccess )
		{
			FPackageFileSummary* PackageFileSummary = GetPackageSummary( PackageReader );
			FArchiveFileReaderGeneric2* Loader = nullptr;
			GetMemory( &PackageReader, LoaderOffset, &Loader, sizeof( FArchiveFileReaderGeneric2* ) );

			int Tell = Loader->Tell();
			if( Tell != PackageFileSummary->NameOffset )
			{
				FString Msg = FString::Printf( TEXT( "%s Package Summary size mismatch !" ), *AssetPath );
				Messages.Add( Msg );
				return false;
			}
			PackageSummarySize = PackageFileSummary->NameOffset;
			//Header might be different if name is bigger
			
			#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
				PackageSummarySize -= PackageFileSummary->PackageName.Len();
			#else
				PackageSummarySize -= PackageFileSummary->FolderName.Len();
			#endif
			PackageSummarySize += PackageName.ToString().Len();
			PackageCustomVersions = PackageFileSummary->GetCustomVersionContainer().GetAllVersions().Num();
			//if( AssetObject->GetFName().GetNumber() != PackageName.GetNumber() )
			//{
			//	return false;
			//}
			

			if( PackageFileSummary->GetFileVersionUE() != EUnrealEngineObjectUE5Version::AUTOMATIC_VERSION )
			{
				FString Msg = FString::Printf( TEXT( "%s PackageFileSummary->GetFileVersionUE() != EUnrealEngineObjectUE5Version::AUTOMATIC_VERSION !" ), *AssetPath );
				Messages.Add( Msg );
				return false;
			}
		}*/
	}

	int PackageNameLength = PackageName.ToString().Len() + 1;
	if (!FCString::IsPureAnsi( *PackageName.ToString() ))
	{
		PackageNameLength *= 2;
	}

	NewAssetForModification->AssetObject = AssetObject;
	NewAssetForModification->PackageName = PackageName;
	NewAssetForModification->Extension = Extension;
	//NewAssetForModification->PackageSummarySize = PackageSummarySize;
	//NewAssetForModification->PackageCustomVersions = PackageCustomVersions;
	NewAssetForModification->PackageNameLength = PackageNameLength;
	NewAssetForModification->FilePath = FilePath;
#endif
	return true;
}
FString CustomEngineURL = TEXT( "https://drive.google.com/drive/folders/1jvPfzknzWHo_diP3k9OJhAmRQZQSjQ7v" );
void ShowEngineModificationRequirement( int NumAssets )
{
	FString DialogText = FString::Printf( TEXT(
		"Warning!\n\n"
		"%d assets require engine modifications to properly downgrade.\n"
		"Do you want to download the custom engine & plugin with modifications in order to downgrade them ?\n"),
		NumAssets
	);
	FText Txt = FText::FromString( DialogText );
	if( FMessageDialog::Open( EAppMsgType::YesNo, Txt ) == EAppReturnType::Yes )
	{
		FPlatformProcess::LaunchURL( *CustomEngineURL, nullptr, nullptr );
	}
}
TArray<FAssetData> GetAssetsOfType( TArray<FAssetData>& AllAssets, FString ClassName )
{
	TArray<FAssetData> ReturnArray;
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
	for( int i = 0; i < AllAssets.Num(); i++ )
	{
		FAssetData& Asset = AllAssets[ i ];
		if( Asset.AssetClassPath.ToString().Contains( ClassName ) )
		{
			ReturnArray.Add( Asset );
		}
	}
#endif
	return ReturnArray;
}
void SortAssets( TArray<FAssetData>& AllAssets )
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
	const int NumClasses = 21;
	FString ClassNames[ NumClasses ] = { TEXT("MaterialFunction"), TEXT("MaterialFunctionMaterialLayer"), TEXT("MaterialFunctionMaterialLayerBlend"),
		TEXT( "Material" ), TEXT("MaterialInstanceConstant"), TEXT( "Skeleton" ), TEXT( "AnimSequence" ), TEXT( "BlendSpace" ), TEXT( "BlendSpace1D" ),
		TEXT( "BehaviorTree" ), TEXT( "NiagaraEmitter" ),TEXT( "NiagaraSystem" ), TEXT( "PoseAsset" ), TEXT("UserDefinedEnum"), TEXT( "UserDefinedStruct" ),
		TEXT( "ControlRigBlueprint" ), TEXT("AnimBlueprint"), TEXT("Blueprint"), TEXT( "WidgetBlueprint" ),
		TEXT( "MapBuildDataRegistry" ), TEXT( "World" ) };
	TArray<FAssetData> AssetClasses[ NumClasses ];
	TArray<FAssetData> RestOfAssets;
	//int RestOfClassesIndex = 0;
	for( int i = 0; i < AllAssets.Num(); i++ )
	{
		FAssetData& Asset = AllAssets[ i ];
		bool KnownClass = false;
		for( int c = 0; c < NumClasses; c++ )
		{
			if( Asset.AssetClassPath.GetAssetName().ToString().Compare( ClassNames[ c ] ) == 0 )
			{
				KnownClass = true;
				AssetClasses[ c ].Add( Asset );
				break;
			}
		}
		if( !KnownClass )
		{
			RestOfAssets.Add( Asset );
		}
	}

	AllAssets.Empty();
	for( int i = 0; i < RestOfAssets.Num(); i++ )
	{
		AllAssets.Add( RestOfAssets[ i ] );
	}
	for( int c = 0; c < NumClasses; c++ )
	{
		for( int i = 0; i < AssetClasses[c].Num(); i++ )
		{
			AllAssets.Add( AssetClasses[ c ][ i ] );
		}
	}
#endif
}
void UnloadAllLoadedAssets( TArray<FAssetData>& AllAssets )
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
	TArray< UPackage*> PackagesToUnload;
	for( int i = 0; i < AllAssets.Num(); i++ )
	{
		auto Asset = AllAssets[ i ];
		UObject* LoadedAsset = Asset.FastGetAsset( false );
		if( LoadedAsset )
		{
			PackagesToUnload.Add( LoadedAsset->GetPackage() );
		}
	}

	bool bOutAnyPackagesUnloaded;
	FText OutErrorMessage;
	UEditorLoadingAndSavingUtils::UnloadPackages( PackagesToUnload, bOutAnyPackagesUnloaded, OutErrorMessage );
#endif
}
void UnloadAsset( UObject* Asset )
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>( TEXT( "AssetRegistry" ) );
	IAssetRegistry& AssetRegistry = AssetRegistryModule.GetRegistry();

	TArray< UPackage*> PackagesToUnload;
	UPackage* Package = Asset->GetPackage();

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 6
	//Fix the occasional crash happening because Metadata is dead in Unload
	UDEPRECATED_MetaData*& DeprecatedMetaData = GetVariable< UDEPRECATED_MetaData* >( Package, MetaDataOffset );
	DeprecatedMetaData = nullptr;
#endif

	PackagesToUnload.Add( Package );

	TArray<FName> PackageDependencies;
	AssetRegistry.GetDependencies( Package->GetFName(), PackageDependencies);

	for( FName& DependencyPath : PackageDependencies )
	{
		TArray<FAssetData> Assets;
		AssetRegistry.GetAssetsByPackageName( DependencyPath, Assets );

		for( int i=0; i<Assets.Num(); i++)
		{
			const FAssetData& DependencyData = Assets[ i ];
			if( DependencyData.IsAssetLoaded() )
			{
				UObject* DependentAssetObject = DependencyData.GetAsset();
				UPackage* DependentPackage = DependentAssetObject->GetPackage();
				if (DependentPackage->IsDirty())
				{
					UE_LOG( LogTemp, Warning, TEXT( "Package %s IsDirty()" ), *DependentPackage->GetName() );
				}
				PackagesToUnload.Add( DependentPackage );
			}
		}
	}

	bool bOutAnyPackagesUnloaded;
	FText OutErrorMessage;
	
	UEditorLoadingAndSavingUtils::UnloadPackages( PackagesToUnload, bOutAnyPackagesUnloaded, OutErrorMessage );
	
	FinishAllCompilation();
#endif
}
int IdentifyAssetDataIndex( UObject* AssetObject, TArray<FAssetData>& AllAssets )
{
	for( int i = 0; i < AllAssets.Num(); i++ )
	{
		FAssetData& AssetData = AllAssets[ i ];
		if( AssetData.AssetName.ToString().Compare( AssetObject->GetName() ) == 0 )
		{
			return i;
		}
	}

	return -1;
}
void WarnAboutNeedingCustomEngine( FString DialogText )
{
	FText Txt = FText::FromString( DialogText );
	if (FMessageDialog::Open( EAppMsgType::YesNo, Txt ) == EAppReturnType::Yes)
	{
		FPlatformProcess::LaunchURL( *CustomEngineURL, nullptr, nullptr );
	}
}
void FDowngraderModule::DowngradeSelectedAssets()
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
	EEngineVersion CurrentVersion = GetCurrentVersion();

	if( DowngraderSettings == nullptr )
	{
		DowngraderSettings = NewObject<UDowngraderParams>();
		DowngraderSettings->AddToRoot();
	}

	TArray<FAssetData> AllAssets;
	EEngineVersion LastBatchTargetVersion = EEngineVersion::EV_5_6;
	bool LastBatchExists = ReadDowngradeLastBatch( AllAssets, LastBatchTargetVersion );
	bool WantsToResume = false;
	if (LastBatchExists)
	{
		FString DialogText = FString::Printf( TEXT(
			"Last downgrade process probably crashed, do you want to resume downgrading the remaining files ?\n"
		) );
		FText Txt = FText::FromString( DialogText );
		if (FMessageDialog::Open( EAppMsgType::YesNo, Txt ) == EAppReturnType::Yes)
		{
			WantsToResume = true;
		}
	}
	if (!WantsToResume || !LastBatchExists)
	{
		AllAssets = GetAllSelectedAssets();
		int NumAssets = AllAssets.Num();
		if (NumAssets == 0)
		{
			FString DialogText = FString::Printf( TEXT(
				"You haven't selected any assets !\n"
				"Did you make the selection in the treeview instead of the listview ?\n"
			) );
			FText Txt = FText::FromString( DialogText );
			FMessageDialog::Open( EAppMsgType::Ok, Txt );
			return;
		}		
	
		FString TitleStr = TEXT( "Asset Downgrader" );

		TSharedPtr<IPlugin> ThisPlugin = IPluginManager::Get().FindPlugin( TEXT( "Downgrader" ) );
		if( ThisPlugin.Get() )
		{
			const FPluginDescriptor& Descriptor = ThisPlugin->GetDescriptor();
		
			FString Suffix = TEXT( "without engine modifications" );
			if( HasEngineModifications() )
			{
				Suffix = TEXT( "with engine modifications" );
			}

			TitleStr = FString::Printf( TEXT( "Downgrader %s %s" ), *Descriptor.VersionName, *Suffix );		
			UE_LOG( LogTemp, Warning, TEXT("%s"), *TitleStr );
		}

		FText Title = FText::FromString( TitleStr );

		// pop up a dialog to input params to the function
		TSharedRef<SWindow> Window = SNew( SWindow )
			.Title( Title )
			.ClientSize( FVector2D( 400, 150 ) )
			.SupportsMinimize( false )
			.SupportsMaximize( false );

		TSharedPtr<SFunctionParamDialog> Dialog;
		Window->SetContent(
			SAssignNew( Dialog, SFunctionParamDialog, Window )
			.OkButtonText( LOCTEXT( "OKButton", "OK" ) ) );

		GEditor->EditorAddModalWindow( Window );

		if( !Dialog->bOKPressed )
			return;
	}
	else
	{
		DowngraderSettings->TargetVersion = LastBatchTargetVersion;
	}

	if( CurrentVersion == EEngineVersion::EV_5_4 && !HasEngineModifications() )
	{
		FString DialogText = FString::Printf( TEXT(
			"Warning!\n\n"
			"All assets in 5.4 require a core engine modification!\n"
			"Can't downgrade any assets without the Custom Engine modifications for Downgrader !\n"
			"Do you want to download the custom engine & plugin with modifications ?\n"
		) );
		WarnAboutNeedingCustomEngine( DialogText );
		return;
	}
	if (CurrentVersion == EEngineVersion::EV_5_5 && !HasEngineModifications() && DowngraderSettings->TargetVersion < EEngineVersion::EV_5_4 )
	{
		FString DialogText = FString::Printf( TEXT(
			"Warning!\n\n"
			"To downgrade all assets to versions prior to 5.4 requires the use of the Downgrader Custom Engine !\n"
			"Do you want to download the Custom Engine & Plugin with modifications ?\n"
		) );
		WarnAboutNeedingCustomEngine( DialogText );
		return;
	}
	if( CurrentVersion == EEngineVersion::EV_5_6 && !HasEngineModifications() )
	{
		FString DialogText = FString::Printf( TEXT(
			"Warning!\n\n"
			"All assets in 5.6 require a core engine modification!\n"
			"Can't downgrade any assets without the Custom Engine modifications for Downgrader !\n"
			"Do you want to download the custom engine & plugin with modifications ?\n"
		) );
		WarnAboutNeedingCustomEngine( DialogText );
		return;
	}
	if (CurrentVersion == EEngineVersion::EV_5_7 && !HasEngineModifications() && DowngraderSettings->TargetVersion < EEngineVersion::EV_5_6)
	{
		FString DialogText = FString::Printf( TEXT(
			"Warning!\n\n"
			"Downgrading lower than 5.6 requires a core engine modification due to PACKAGE_SAVED_HASH!\n"
			"Can't downgrade any assets without the Custom Engine modifications for Downgrader !\n"
			"Do you want to download the custom engine & plugin with modifications ?\n"
		) );
		WarnAboutNeedingCustomEngine( DialogText );
		return;
	}

	FString Str = UEnum::GetValueAsString( TEXT( "Downgrader.EEngineVersion" ), DowngraderSettings->TargetVersion );
	UE_LOG( LogTemp, Warning, TEXT( "Downgrading to %s" ), *Str );
	
	TArray< AssetForModification*> AssetsToModify;
	TArray<FString> Messages;
	
	FScopedSlowTask* LoadingProgress = nullptr;
	
	int Index = 0;
	UWorld* world = GEditor->GetEditorWorldContext().World();
	//int ThresholdForGC = 1;
	int AssetsThatRequireCustomEngine = 0;

	auto LoadAndPatchLambda = [&]( FAssetData& Asset ) ->AssetForModification*
	{
		FString Text = FString::Printf( TEXT( "Loading and Patching assets %d/%d ( %s )" ), Index + 1, AllAssets.Num(), *Asset.AssetName.ToString());
		FText StrTxt = FText::FromString( Text );
		if ( LoadingProgress )
			LoadingProgress->EnterProgressFrame( 1, StrTxt );
		Index++;
		
		UObject* AssetObject = Asset.GetAsset();
		if( !AssetObject )
		{
			FString Str = FString::Printf( TEXT( "PackageName=%s AssetName=%s couldn't be loaded !" ), *Asset.PackageName.ToString(), *Asset.AssetName.ToString() );
			Messages.Add( Str );
			return nullptr;
		}

		AssetObject->AddToRoot();
		RemovePropertyFlagOnAllProperties( CPF_Deprecated );//Need to put it here because not all classes are loaded upfront
		//FAssetCompilingManager::Get().FinishCompilationForObjects( { AssetObject } );
		FAssetCompilingManager::Get().FinishAllCompilation();

		TArray<UObject*> PackageObjects;
		UPackage* Package = AssetObject->GetPackage();
		GetObjectsWithOuter( Package, PackageObjects, false );

		TArray<FName> PackageDependencies;
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>( TEXT( "AssetRegistry" ) );
		IAssetRegistry& AssetRegistry = AssetRegistryModule.GetRegistry();
		AssetRegistry.GetDependencies( Package->GetFName(), PackageDependencies );

		FixPackage( Package );

		UMapBuildDataRegistry* MapBuildDataRegistry = Cast< UMapBuildDataRegistry >( AssetObject );
		if (MapBuildDataRegistry)
		{
			for (int t = 0; t < AllAssets.Num(); t++)
			{
				//Search for the World and if they have the same target file, don't save this
				if (AllAssets[t].ObjectPath != Asset.ObjectPath)
				{
					if (AllAssets[t].PackageName == Asset.PackageName)
					{
						return nullptr;
					}
				}
			}
			FixMapBuildData( MapBuildDataRegistry );
		}

		#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4
		UPoseSearchDatabase* PoseSearchDatabase = Cast<UPoseSearchDatabase>( AssetObject );
		if( PoseSearchDatabase )
		{
			if( DowngraderSettings->TargetVersion < EEngineVersion::EV_5_4 )
			{
				return nullptr;
			}
		}
		#endif
		UTexture* Texture = Cast<UTexture>( AssetObject );		
		if( Texture )
		{
			ETextureSourceFormat Format = Texture->Source.GetFormat();
			if( DowngraderSettings->TargetVersion < EEngineVersion::EV_5_1 )
			{
				if( Format == ETextureSourceFormat::TSF_RGBA32F || Format == ETextureSourceFormat::TSF_R16F || Format == ETextureSourceFormat::TSF_R32F )
				{
					const UEnum* FormatEnum = StaticEnum<ETextureSourceFormat>();
					FString FormatStr = FormatEnum->GetNameStringByValue( (uint8)Format );
					FString Str = FString::Printf( TEXT( "Texture %s has unsupported format %s in 5.0" ), *Asset.PackageName.ToString(), *FormatStr );
					Messages.Add( Str );
					return nullptr;
				}
			}
			if( DowngraderSettings->TargetVersion <= EEngineVersion::EV_4_27 && !HasEngineModifications() )
			{
				FString Str = FString::Printf( TEXT( "Texture %s requires engine modifications when downgrading to 4.27" ), *Asset.PackageName.ToString() );
				Messages.Add( Str );
				AssetsThatRequireCustomEngine++;
				return nullptr;
			}
			FixTexture( Texture );
		}
		UVirtualTextureBuilder* VirtualTextureBuilder = Cast<UVirtualTextureBuilder>( AssetObject );
		if( VirtualTextureBuilder )
		{
			FixTexture( VirtualTextureBuilder->Texture.Get() );
			FixTexture( VirtualTextureBuilder->TextureMobile.Get() );
		}
		UParticleSystem* ParticleSystem = Cast<UParticleSystem>( AssetObject );
		if (ParticleSystem)
		{
			FixParticleSystem( ParticleSystem );
		}
		UNiagaraSystem* NiagaraSystem = Cast<UNiagaraSystem>( AssetObject );
		if( NiagaraSystem )
		{
			FixNiagaraSystem( NiagaraSystem );
		}
		UNiagaraEmitter* NiagaraEmitter = Cast<UNiagaraEmitter>( AssetObject );
		if( NiagaraEmitter )
		{
			FixNiagaraEmitter( NiagaraEmitter, nullptr );
		}
		UNiagaraScript* NiagaraScript = Cast<UNiagaraScript>( AssetObject );
		if( NiagaraScript )
		{
			FixNiagaraScript( NiagaraScript );
		}
		UPhysicsAsset* PhysicsAsset = Cast<UPhysicsAsset>( AssetObject );
		if ( PhysicsAsset )
		{
			FixPhysicsAsset( PhysicsAsset );
		}
		UAnimSequence* AnimationSequence = Cast<UAnimSequence>( AssetObject );
		if( AnimationSequence )
		{
			if(DowngraderSettings->TargetVersion <= EEngineVersion::EV_4_27 && !HasEngineModifications() )
			{
				FString Str = FString::Printf( TEXT( "Animation %s requires engine modifications" ), *Asset.PackageName.ToString() );
				Messages.Add( Str );
				AssetsThatRequireCustomEngine++;
				return nullptr;
			}
			FixAnimSequence( AnimationSequence );
		}
		UBlendSpace* BlendSpace = Cast<UBlendSpace>( AssetObject );
		if (BlendSpace)
		{
			FixBlendSpace( BlendSpace );
		}
		UPoseAsset* PoseAsset = Cast<UPoseAsset>( AssetObject );
		if( PoseAsset )
		{
			FixPoseAsset( PoseAsset );
		}
		UAnimMontage* AnimMontage = Cast<UAnimMontage>( AssetObject );
		if( AnimMontage )
		{
			FixAnimSequenceBase( AnimMontage );
		}
		ULevelSequence* LevelSequence = Cast<ULevelSequence>( AssetObject );
		if( LevelSequence )
		{
			FixLevelSequence( LevelSequence );
		}
		UBlueprint* Blueprint = Cast<UBlueprint>( AssetObject );
		if ( Blueprint )
		{
			if( DowngraderSettings->TargetVersion <= EEngineVersion::EV_4_27 && !HasEngineModifications() )
			{
				FString Str = FString::Printf( TEXT( "Blueprint %s requires engine modifications when downgrading to 4.27" ), *Asset.PackageName.ToString() );
				Messages.Add( Str );
				AssetsThatRequireCustomEngine++;
				return nullptr;
			}
			FixBlueprint( Blueprint );
		}
		UControlRigBlueprint* ControlRigBlueprint = Cast<UControlRigBlueprint>( AssetObject );
		if( ControlRigBlueprint )
		{
			if(DowngraderSettings->TargetVersion <= EEngineVersion::EV_4_27 && !HasEngineModifications() )
			{
				FString Str = FString::Printf( TEXT( "ControlRigBlueprint %s requires engine modifications" ), *Asset.PackageName.ToString() );
				Messages.Add( Str );
				AssetsThatRequireCustomEngine++;
				return nullptr;
			}
			FixControlRigBlueprint( ControlRigBlueprint );
		}
		//BlueprintStructure
		UUserDefinedStruct* UserDefinedStruct = Cast<UUserDefinedStruct>( AssetObject );
		if( UserDefinedStruct )
		{
			FixUserDefinedStruct( UserDefinedStruct );
		}
		UWorld* world = Cast<UWorld>( AssetObject );
		if( world )
		{
			if( DowngraderSettings->TargetVersion <= EEngineVersion::EV_4_27 && !HasEngineModifications() )
			{
				FString Str = FString::Printf( TEXT( "Level %s requires engine modifications" ), *Asset.PackageName.ToString() );
				Messages.Add( Str );
				AssetsThatRequireCustomEngine++;
				return nullptr;
			}
			//If this is not done, WP maps will appear as empty in 5.1
			if( DowngraderSettings->TargetVersion <= EEngineVersion::EV_5_1 )
			{
				RemoveWorldPartition( Asset );
			}
			FixWorld( world );
		}
		AActor* Actor = Cast<AActor>( AssetObject );
		if( Actor )
		{
			FixActor( Actor );
		}
		UMaterial* Material = Cast<UMaterial>( AssetObject );
		if( Material )
		{
			FixMaterial( Material );
		}
		UMaterialFunction* MaterialFunction = Cast<UMaterialFunction>( AssetObject );
		if( MaterialFunction )
		{
			FixMaterialFunction( MaterialFunction );
		}
		UMaterialInstance* MaterialInstance = Cast<UMaterialInstance>( AssetObject );
		if ( MaterialInstance )
		{
			FixMaterialInstance( MaterialInstance );
		}
		UStaticMesh* StaticMesh = Cast< UStaticMesh >( AssetObject );
		if( StaticMesh )
		{
			//if( !HasEngineModifications() )
			//{
			//	FString Str = FString::Printf( TEXT( "StaticMesh %s requires engine modifications" ), *Asset.PackageName.ToString() );
			//	Messages.Add( Str );
			//	return nullptr;
			//}
			FixStaticMesh( StaticMesh, DowngraderSettings->TargetVersion );
		}
		USkeleton* Skeleton = Cast< USkeleton >( AssetObject );
		if (Skeleton)
		{
			FixSkeleton( Skeleton );
		}
		USkeletalMesh* SkeletalMesh = Cast< USkeletalMesh >( AssetObject );
		if( SkeletalMesh )
		{
			if( DowngraderSettings->TargetVersion <= EEngineVersion::EV_4_27 && !HasEngineModifications() )
			{
				FString Str = FString::Printf( TEXT( "Blueprint %s requires engine modifications when downgrading to 4.27" ), *Asset.PackageName.ToString() );
				Messages.Add( Str );
				return nullptr;
			}
			FixSkeletalMesh( SkeletalMesh, DowngraderSettings->TargetVersion );
		}
		USoundWave* SoundWave = Cast< USoundWave >( AssetObject );
		if( SoundWave )
		{
			if(DowngraderSettings->TargetVersion <= EEngineVersion::EV_4_27 && !HasEngineModifications() )
			{
				FString Str = FString::Printf( TEXT( "SoundWave %s requires engine modifications" ), *Asset.PackageName.ToString() );
				Messages.Add( Str );
				return nullptr;
			}
		}
		UMetaSoundSource* MetaSoundSource = Cast< UMetaSoundSource >( AssetObject );
		if( MetaSoundSource )
		{
			if( DowngraderSettings->TargetVersion <= EEngineVersion::EV_4_27 )
			{
				FString Str = FString::Printf( TEXT( "MetaSound %s doesn't exist in 4.27" ), *Asset.PackageName.ToString() );
				Messages.Add( Str );
				return nullptr;
			}
			FixMetaSound( MetaSoundSource );
		}		
		UGeometryCollection* GeometryCollection = Cast< UGeometryCollection >( AssetObject );
		if( GeometryCollection )
		{
			FixGeometryCollection( GeometryCollection );
		}
		UDataflow* Dataflow = Cast< UDataflow >( AssetObject );
		if( Dataflow )
		{
			FixDataFlow( Dataflow );
		}
		UInputMappingContext* InputMappingContext = Cast<UInputMappingContext>( AssetObject );
		if (InputMappingContext)
		{
			FixInputMappingContext( InputMappingContext );
		}
		UGroomAsset* GroomAsset = Cast< UGroomAsset>( AssetObject );
		if (GroomAsset)
		{
			FixGroomAsset( GroomAsset );
		}
		
		FAssetCompilingManager::Get().FinishAllCompilation( );

		AssetForModification* NewAssetForModification = new AssetForModification;

		bool ReadSuccess = PreparePackage( AssetObject, AssetObject->GetPackage(), Asset.PackageName, Asset.GetSoftObjectPath().ToString(), CurrentVersion,
										   NewAssetForModification, Messages );

		FAssetCompilingManager::Get().FinishAllCompilation();

		if( ReadSuccess )
		{
			AssetsToModify.Add( NewAssetForModification );
			return NewAssetForModification;
		}
		else
		{
			delete NewAssetForModification;
			return nullptr;
		}
	};
	
	SortAssets( AllAssets );

	WriteDowngradeLastBatch( AllAssets, DowngraderSettings->TargetVersion );

	//Gather the uassets for one file per actor option
	if (DowngraderSettings->TargetVersion >= EEngineVersion::EV_5_0)
	{
		for (int i = 0; i < AllAssets.Num(); i++)
		{
			FAssetData& AssetData = AllAssets[i];
			if (AssetData.AssetClassPath.GetAssetName() == TEXT( "World" ))
			{
				UObject* AssetObject = AssetData.GetAsset();
				UWorld* AssetWorld = Cast<UWorld>( AssetObject );
				if (AssetWorld)
				{
					bool Modified = false;
					if (DowngraderSettings->TargetVersion >= EEngineVersion::EV_5_0)
					{
						TArray<FAssetData> ActorAssetFiles = GetActorAssetFilesForLevel( AssetWorld->PersistentLevel, false );
						if (ActorAssetFiles.Num() > 0)
						{
							for (int f = 0; f < ActorAssetFiles.Num(); f++)
							{
								AllAssets.Add( ActorAssetFiles[f] );
							}
						}
					}
					else//for <4.27
					{
						//FixWorld( AssetWorld );
						//
						//ULevelInstanceSubsystem* LevelInstanceSubsystem = AssetWorld->GetSubsystem<ULevelInstanceSubsystem>();
						//TArray<ILevelInstanceInterface*> BreakableLevelInstances;
						//for (TActorIterator<AActor> Iterator( AssetWorld ); Iterator; ++Iterator)
						//{
						//	AActor* Actor = *Iterator;
						//	APackedLevelActor* PackedLevelActor = Cast< APackedLevelActor>( Actor );
						//	if (PackedLevelActor)
						//	{
						//		BreakableLevelInstances.Add( PackedLevelActor );
						//	}
						//}
						//ELevelInstanceBreakFlags Flags = ELevelInstanceBreakFlags::None;
						//int32 BreakLevels = 1;
						//for (ILevelInstanceInterface* LevelInstance : BreakableLevelInstances)
						//{
						//	LevelInstanceSubsystem->BreakLevelInstance( LevelInstance, BreakLevels, nullptr, Flags );
						//	Modified = true;
						//}
					}
					if (Modified)
					{
						world->PostEditChange();
						world->MarkPackageDirty();

						GEditor->GetSelectedActors()->DeselectAll();

						FString Extension = FPackageName::GetMapPackageExtension();
						SavePackage( AssetObject->GetPackage(), AssetObject->GetPackage()->GetName(), Extension );
					}
				}
				UnloadAsset( AssetObject );
			}
		}
	}
	//bool FixMaterialFunctionDependencies = true;
	//if ( FixMaterialFunctionDependencies )
	//{
	//	for( int i = 0; i < AllAssets.Num(); i++ )
	//	{
	//		FAssetData& AssetData = AllAssets[ i ];
	//		if( AssetData.AssetClassPath.GetAssetName() == TEXT( "MaterialFunction" ) )
	//		{
	//			UObject* AssetObject = AssetData.GetAsset();
	//			UMaterialFunction* MaterialFunction = Cast<UMaterialFunction>( AssetObject );
	//			if( MaterialFunction )
	//			{
	//				TArray<UMaterialFunctionInterface*> DependentFunctions;
	//				MaterialFunction->GetDependentFunctions( DependentFunctions );
	//				for( int d = 0; d < DependentFunctions.Num(); d++ )
	//				{
	//					UMaterialFunction* DependentFunction = Cast< UMaterialFunction >( DependentFunctions[ d ] );
	//
	//					int DependentFunctionIndex = IdentifyAssetDataIndex( DependentFunction, AllAssets );
	//					if( DependentFunctionIndex != -1 && DependentFunctionIndex > i )
	//					{
	//						FAssetData Temp = AssetData;
	//						AllAssets[ i ] = AllAssets[ DependentFunctionIndex ];
	//						AllAssets[ DependentFunctionIndex ] = Temp;
	//						i--;
	//						break;
	//					}
	//				}
	//			}
	//
	//			UnloadAsset( AssetObject );
	//		}
	//	}
	//}

	LoadingProgress = new FScopedSlowTask( AllAssets.Num() );
	LoadingProgress->MakeDialog();

	TArray< AssetForModification*> AssetsWithErrors;
	int SuccessfulDowngrade = 0;
	
	FScopedSlowTask* SavingProgress = nullptr;
	if( DoFullDowngradingPerAsset )// && !DowngraderSettings->ExternalActors )
	{
		for( int i = 0; i < AllAssets.Num(); i++ )
		{
			AssetForModification* Asset = LoadAndPatchLambda( AllAssets[ i ] );
			if( !Asset )
				continue;

			UpdateLastBatch( i );

			SpoofCustomVersions( DowngraderSettings->TargetVersion );
			RemoveBlueprintRedirects();

			//Only for 5.0 ?
			if (DowngraderSettings->TargetVersion == EEngineVersion::EV_5_0)
			{
				AddPropertyFlag( TEXT( "Skeleton" ), TEXT( "PreviewSkeletalMesh" ), CPF_Deprecated );
			}
			if( DowngraderSettings->TargetVersion <= EEngineVersion::EV_4_27 )
			{
				//For StaticMeshes
				AddPropertyFlag( TEXT( "MeshBuildSettings" ), TEXT( "BuildScale3D" ), CPF_Deprecated );
				AddPropertyFlag( TEXT( "StaticMesh" ), TEXT( "HiResSourceModel" ), CPF_Deprecated );
				AddPropertyFlag( TEXT( "StaticMesh" ), TEXT( "EditorCameraPosition" ), CPF_Deprecated );

				//For textures
				AddPropertyFlag( TEXT( "Texture2D" ), TEXT( "AlphaCoverageThresholds" ), CPF_Deprecated );
			}

			if( Asset->Downgrade() )
			{
				SuccessfulDowngrade++;
			}
			else
			{
				AssetsWithErrors.Add( Asset );
			}

			//Revert back to load stuff correctly
			SpoofCustomVersions( CurrentVersion );
			RemoveBlueprintRedirects( true );

			//Unloading sometimes loads stuff in
			if( DoUnloadPerAsset )
			{
				UnloadAsset( Asset->AssetObject );
				//bool bOutAnyPackagesUnloaded;
				//FText OutErrorMessage;
				//TArray< UPackage*> PackagesToUnload;
				//PackagesToUnload.Add( Asset->Package );
				//UEditorLoadingAndSavingUtils::UnloadPackages( PackagesToUnload, bOutAnyPackagesUnloaded, OutErrorMessage );
				Asset->AssetObject = nullptr;
			}
			if( WriteAssetAfterUnload )
			{
				uint32 WriteFlags = FILEWRITE_EvenIfReadOnly;
				bool Status = FFileHelper::SaveArrayToFile( Asset->PackageFileData, *Asset->FilePath, &IFileManager::Get(), WriteFlags );
				if( !Status )
				{
					AssetsWithErrors.Add( Asset );
				}
			}
		}

		UpdateLastBatch( AllAssets.Num() );

		delete LoadingProgress;
		LoadingProgress = nullptr;
	}
	else
	{
		for( int i = 0; i < AllAssets.Num(); i++ )
		{
			LoadAndPatchLambda( AllAssets[ i ] );
		}

		delete LoadingProgress;
		LoadingProgress = nullptr;

		SpoofCustomVersions( DowngraderSettings->TargetVersion );
		RemoveBlueprintRedirects();

		AddPropertyFlag( TEXT( "Skeleton" ), TEXT( "PreviewSkeletalMesh" ), CPF_Deprecated );

		if( DowngraderSettings->TargetVersion <= EEngineVersion::EV_4_27 )
		{
			//For StaticMeshes
			AddPropertyFlag( TEXT( "MeshBuildSettings" ), TEXT( "BuildScale3D" ), CPF_Deprecated );
			AddPropertyFlag( TEXT( "StaticMesh" ), TEXT( "HiResSourceModel" ), CPF_Deprecated );
			AddPropertyFlag( TEXT( "StaticMesh" ), TEXT( "EditorCameraPosition" ), CPF_Deprecated );

			//For textures
			AddPropertyFlag( TEXT( "Texture2D" ), TEXT( "AlphaCoverageThresholds" ), CPF_Deprecated );
		}

		SavingProgress = new FScopedSlowTask( AssetsToModify.Num() );
		SavingProgress->MakeDialog();

		for( int i = 0; i < AssetsToModify.Num(); i++ )
		{
			//if( i % ThresholdForGC )
			//{
				//GEngine->Exec( world, TEXT( "obj gc" ) );
			//}
			auto Asset = AssetsToModify[ i ];

			FString Text = FString::Printf( TEXT( "Saving assets %d/%d" ), i + 1, AssetsToModify.Num() );
			FText StrTxt = FText::FromString( Text );
			SavingProgress->EnterProgressFrame( 1, StrTxt );

			if( Asset->Downgrade() )
			{
				SuccessfulDowngrade++;
				//if( DoUnloadPerAsset )
					//PackagesToUnload.Add( Asset->Package );
			}
			else
			{
				AssetsWithErrors.Add( Asset );
			}
		}

		//Revert back to (maybe) test outputs
		SpoofCustomVersions( CurrentVersion );
		RemoveBlueprintRedirects( true );
	}

	for( int i = 0; i < Messages.Num(); i++ )
	{
		UE_LOG( LogTemp, Error, TEXT( "%s" ), *Messages[ i ] );
	}

	if ( SavingProgress )
		delete SavingProgress;

	FString AssetsWithErrorsList;
	for( int m = 0; m < Messages.Num(); m++ )
	{
		AssetsWithErrorsList += Messages[ m ];
		AssetsWithErrorsList += TEXT( "\n" );
	}
	for( int i = 0; i < AssetsWithErrors.Num(); i++ )
	{
		auto Asset = AssetsWithErrors[ i ];
		if( i == 0 )
		{
			AssetsWithErrorsList += TEXT( "Files with errors :\n" );
		}
		AssetsWithErrorsList += FString::Printf( TEXT( "%s\n" ), *Asset->PackageName.ToString() );
	}
	FString DialogText = FString::Printf( TEXT(
		"Downgrade Complete!\n\n"
		"Downgraded %d/%d assets!\n\n"
		"Don't forget to also install the Downgrader plugin in the Target version project !\n"
		"%s"
	), SuccessfulDowngrade, AssetsToModify.Num(), *AssetsWithErrorsList );

	FText Txt = FText::FromString( DialogText );
	FMessageDialog::Open( EAppMsgType::Ok, Txt );

	if( AssetsThatRequireCustomEngine > 0 )
	{
		ShowEngineModificationRequirement( AssetsThatRequireCustomEngine );
	}

	for( int i = 0; i < AssetsToModify.Num(); i++ )
	{
		delete AssetsToModify[ i ];
	}
	AssetsToModify.Reset( 0 );
	DowngraderSettings->SaveConfig();
#else
	
	FString DialogText = FString::Printf( TEXT(
		"Error!\n\n"
		"Assets can only be downgraded from UE 5.3.2 or 5.4 !\n"
		) );
	FText Txt = FText::FromString( DialogText );
	FMessageDialog::Open( EAppMsgType::Ok, Txt );
#endif
}
void FDowngraderModule::PreloadSelectedAssets()
{
	int NumAssets = GetNumSelectedAssets();
	FScopedSlowTask LoadingProgress( NumAssets );
	LoadingProgress.MakeDialog();

	int Index = 0;
	UWorld* world = GEditor->GetEditorWorldContext().World();

	auto lambda = [&]( FAssetData& Asset )
	{
		FString Text = FString::Printf( TEXT( "Preloading assets %d/%d" ), Index + 1, NumAssets );
		FText StrTxt = FText::FromString( Text );
		LoadingProgress.EnterProgressFrame( 1, StrTxt );
		Index++;
		
		UObject* AssetObject = Asset.GetAsset();
	};

	IterateOverSelection( lambda );

	FinishAllCompilation();
}
void EnablePackageTrailer( bool Enable )
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION == 0
	if( UE::FPackageTrailer::IsEnabled() != Enable )
	{
		uint8* Reference = (uint8*)&FPrimaryAssetId::PrimaryAssetTypeTag;
		int Offset = 164344;
		Reference -= Offset;
		bool Failed = false;
		if( *Reference == !Enable )
		{
			*Reference = Enable;
			if( UE::FPackageTrailer::IsEnabled() != Enable )
				Failed = true;
		}
		else//Something else was there, pointer magic didn't work
			Failed = true;

		if( Failed )
		{
			FString DialogText = FString::Printf( TEXT(
				"The hack to disable PackageTrailer failed ! Target project still needs to have \n"
				"[Core.System]\n"
				"UsePackageTrailer=True\n"
				"added to Config/DefaultEngine.ini \n"
			) );
			FText Txt = FText::FromString( DialogText );
			FMessageDialog::Open( EAppMsgType::Ok, Txt );
		}
	}
#endif
}
void FDowngraderModule::SaveSelectedAssets()
{
	int NumAssets = GetNumSelectedAssets();
	FScopedSlowTask LoadingProgress( NumAssets );
	LoadingProgress.MakeDialog();

	int Index = 0;
	UWorld* world = GEditor->GetEditorWorldContext().World();

	auto lambda = [&]( FAssetData& Asset )
	{
		FString Text = FString::Printf( TEXT( "Saving assets %d/%d" ), Index + 1, NumAssets );
		FText StrTxt = FText::FromString( Text );
		LoadingProgress.EnterProgressFrame( 1, StrTxt );
		Index++;

		EnablePackageTrailer( true );
		UObject* AssetObject = Asset.GetAsset();
		if( AssetObject )
		{
			#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION <= 27
			UStaticMesh* StaticMesh = Cast< UStaticMesh >( AssetObject );
			if( StaticMesh )
			{
				StaticMesh->Build();
			}
			USkeletalMesh* SkeletalMesh = Cast< USkeletalMesh >( AssetObject );
			if( SkeletalMesh )
			{
				USkeletalMeshEditorData* MeshEditorDataObject = SkeletalMesh->MeshEditorDataObject;
				//if( MeshEditorDataObject )
				//{
				//	bool Modified = false;
				//	for( int i = 0; i < SkeletalMesh->GetLODNum(); i++ )
				//	{
				//		if( MeshEditorDataObject->IsLODImportDataValid( i ) )
				//		{
				//			FRawSkeletalMeshBulkData& MeshBulkData = MeshEditorDataObject->GetLODImportedData( i );
				//			static int Offset = 176;
				//			FCustomVersionContainer& SerializeLoadingCustomVersionContainer = GetVariable< FCustomVersionContainer>( &MeshBulkData, Offset );
				//			FCustomVersion* EditorVersion = (FCustomVersion*)SerializeLoadingCustomVersionContainer.GetVersion( FEditorObjectVersion::GUID );
				//
				//			if(EditorVersion && EditorVersion->Version != FEditorObjectVersion::LatestVersion )
				//			{
				//				EditorVersion->Version = FEditorObjectVersion::LatestVersion;
				//				Modified = true;
				//			}
				//		}
				//	}
				//
				//	if( Modified )
				//	{
				//		SkeletalMesh->PostEditChange();
				//		SkeletalMesh->MarkPackageDirty();
				//	}
				//}
			}
			UAnimSequence* AnimSequence = Cast< UAnimSequence >( AssetObject );
			if( AnimSequence )
			{
				//UAnimSequence_Downgrader* AnimationSequence2 = (UAnimSequence_Downgrader*)AnimSequence;
				//TArray<struct FRawAnimSequenceTrack>& RawAnimationData = AnimationSequence2->GetRawAnimationData();
				//TArray<struct FTrackToSkeletonMap>& TrackToSkeletonMapTable = AnimationSequence2->GetTrackToSkeletonMapTable();
				//if( TrackToSkeletonMapTable.Num() == 0 || TrackToSkeletonMapTable.Num() != RawAnimationData.Num() )
				//{
				//	TrackToSkeletonMapTable.Empty( 0 );
				//	for( int i = 0; i < RawAnimationData.Num(); i++ )
				//	{
				//		TrackToSkeletonMapTable.Add( i );
				//	}
				//}
				//
				//FAsyncCompressedAnimationsManagement* AsyncCompressedAnimationsManagement = &FAsyncCompressedAnimationsManagement::Get();
				//TArray<FActiveAsyncCompressionTask>& ActiveTasks = GetVariable<TArray<FActiveAsyncCompressionTask>>( AsyncCompressedAnimationsManagement, 24 );
				//for( int i = 0; i < ActiveTasks.Num(); i++ )
				//{
				//	FActiveAsyncCompressionTask& Task = ActiveTasks[ i ];
				//	if( Task.DataToCompress->AnimFName == AnimSequence->GetFName() )
				//	{
				//		if( Task.DataToCompress->TrackToSkeletonMapTable.Num() == 0 || Task.DataToCompress->TrackToSkeletonMapTable.Num() != RawAnimationData.Num() )
				//		{
				//			Task.DataToCompress->TrackToSkeletonMapTable.Empty( 0 );
				//			for( int t = 0; t < RawAnimationData.Num(); t++ )
				//			{
				//				Task.DataToCompress->TrackToSkeletonMapTable.Add( t );
				//			}
				//		}
				//	}
				//}
			}
			#else
				//Trigger object rebuild so I know for sure it's saved in the current version format
					//this is needed for skeletal meshes when going from 5.4 to anything bellow
				AssetObject->PostEditChange();
				AssetObject->MarkPackageDirty();
			#endif

			EnablePackageTrailer( false );

			//Remove the "/Script/Engine." from ClassName
			//RegisterObjectClassWithEngineOuter( AssetObject );
			//Don't save these because they conflict the umap file (at least in 4.27)
			UBlueprintGeneratedClass* GeneratedClass = Cast< UBlueprintGeneratedClass>( AssetObject );
			if (!GeneratedClass)
			{
				FString Extension = AssetObject->IsA<UWorld>() ? FPackageName::GetMapPackageExtension() : FPackageName::GetAssetPackageExtension();
				SavePackage( AssetObject->GetPackage(), Asset.PackageName.ToString(), Extension );
			}
		}
	};

	IterateOverSelection( lambda );
}
void FDowngraderModule::RevertSubstrateMaterials()
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
	auto lambda = [&]( FAssetData& Asset )
	{
		UObject* AssetObject = Asset.GetAsset();
		UMaterial* Material = Cast< UMaterial >( AssetObject );
		if( Material )
		{
			TArray<UMaterialExpression*> Expressions = GetExpressions( Material );
			for( int i = 0; i < Expressions.Num(); i++ )
			{
				UMaterialExpression* Exp = Expressions[ i ];

				UMaterialEditorOnlyData* EditorOnlyData = Material->GetEditorOnlyData();
				bool Modified = false;

			#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4
				UMaterialExpressionSubstrateShadingModels* ShadingModels = Cast< UMaterialExpressionSubstrateShadingModels >( Exp );
				UMaterialExpressionSubstrateSlabBSDF* SlabBSDF = Cast< UMaterialExpressionSubstrateSlabBSDF >( Exp );
				UMaterialExpressionSubstrateUnlitBSDF* UnlitBSDF = Cast< UMaterialExpressionSubstrateUnlitBSDF >( Exp );
				UMaterialExpressionSubstrateSingleLayerWaterBSDF* SingleLayerWaterBSDF = Cast< UMaterialExpressionSubstrateSingleLayerWaterBSDF >( Exp );
			#else
				UMaterialExpressionStrataLegacyConversion* ShadingModels = Cast< UMaterialExpressionStrataLegacyConversion>( Exp );
				UMaterialExpressionStrataSlabBSDF* SlabBSDF = Cast< UMaterialExpressionStrataSlabBSDF>( Exp );
				UMaterialExpressionStrataUnlitBSDF* UnlitBSDF = Cast< UMaterialExpressionStrataUnlitBSDF >(Exp);
				UMaterialExpressionStrataSingleLayerWaterBSDF* SingleLayerWaterBSDF = Cast< UMaterialExpressionStrataSingleLayerWaterBSDF >(Exp);
			#endif
				
				if( ShadingModels )
				{
					if( !EditorOnlyData->Opacity.Expression )
						EditorOnlyData->Opacity.Expression = ShadingModels->Opacity.Expression;
					if( !EditorOnlyData->EmissiveColor.Expression )
						EditorOnlyData->EmissiveColor.Expression = ShadingModels->EmissiveColor.Expression;
					if( !EditorOnlyData->BaseColor.Expression )
						EditorOnlyData->BaseColor.Expression = ShadingModels->BaseColor.Expression;
					if( !EditorOnlyData->Normal.Expression )
						EditorOnlyData->Normal.Expression = ShadingModels->Normal.Expression;
					if( !EditorOnlyData->SubsurfaceColor.Expression )
						EditorOnlyData->SubsurfaceColor.Expression = ShadingModels->SubSurfaceColor.Expression;
					if( !EditorOnlyData->ClearCoat.Expression )
						EditorOnlyData->ClearCoat.Expression = ShadingModels->ClearCoat.Expression;
					if( !EditorOnlyData->Metallic.Expression )
						EditorOnlyData->Metallic.Expression = ShadingModels->Metallic.Expression;
					if( !EditorOnlyData->Specular.Expression )
						EditorOnlyData->Specular.Expression = ShadingModels->Specular.Expression;
					if( !EditorOnlyData->Roughness.Expression )
						EditorOnlyData->Roughness.Expression = ShadingModels->Roughness.Expression;

					Modified = true;
				}
				if( SlabBSDF )
				{
					if( !EditorOnlyData->EmissiveColor.Expression )
						EditorOnlyData->EmissiveColor.Expression = SlabBSDF->EmissiveColor.Expression;
					if( !EditorOnlyData->BaseColor.Expression )
						EditorOnlyData->BaseColor.Expression = SlabBSDF->DiffuseAlbedo.Expression;
					if( !EditorOnlyData->Normal.Expression )
						EditorOnlyData->Normal.Expression = SlabBSDF->Normal.Expression;
					if( !EditorOnlyData->Roughness.Expression )
						EditorOnlyData->Roughness.Expression = SlabBSDF->Roughness.Expression;

					Modified = true;
				}
				if( UnlitBSDF )
				{
					if( !EditorOnlyData->EmissiveColor.Expression )
						EditorOnlyData->EmissiveColor.Expression = UnlitBSDF->EmissiveColor.Expression;
					#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4
					if( !EditorOnlyData->Normal.Expression )
						EditorOnlyData->Normal.Expression = UnlitBSDF->Normal.Expression;
					#endif

					Modified = true;
				}
				if( SingleLayerWaterBSDF )
				{
					if( !EditorOnlyData->EmissiveColor.Expression )
						EditorOnlyData->EmissiveColor.Expression = SingleLayerWaterBSDF->EmissiveColor.Expression;
					if( !EditorOnlyData->BaseColor.Expression )
						EditorOnlyData->BaseColor.Expression = SingleLayerWaterBSDF->BaseColor.Expression;
					if( !EditorOnlyData->Normal.Expression )
						EditorOnlyData->Normal.Expression = SingleLayerWaterBSDF->Normal.Expression;
					if( !EditorOnlyData->Metallic.Expression )
						EditorOnlyData->Metallic.Expression = SingleLayerWaterBSDF->Metallic.Expression;
					if( !EditorOnlyData->Specular.Expression )
						EditorOnlyData->Specular.Expression = SingleLayerWaterBSDF->Specular.Expression;
					if( !EditorOnlyData->Roughness.Expression )
						EditorOnlyData->Roughness.Expression = SingleLayerWaterBSDF->Roughness.Expression;

					Modified = true;
				}
				if( Modified )
				{
					Material->PostEditChange();
					Material->MarkPackageDirty();
				}
			}
		}
	};

	IterateOverSelection( lambda );
#endif
}
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
const TMap<UFoliageType*, TUniqueObj<FFoliageInfo>>& GetFoliageInfos( AInstancedFoliageActor* IFA )
{
#if ENGINE_MAJOR_VERSION == 4
	auto& Infos = IFA->FoliageInfos;
#else
	auto& Infos = IFA->GetFoliageInfos();
#endif

	return Infos;
}
#endif
void FDowngraderModule::FixLandscape()
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
	UWorld* world = GEditor->GetEditorWorldContext().World();
	for( TActorIterator<AActor> Iterator( world ); Iterator; ++Iterator )
	{
		AActor* Actor = *Iterator;
		TArray<UActorComponent*> Components;
		Actor->GetComponents( Components );
		ALandscapeProxy* LandscapeProxy = Cast<ALandscapeProxy>( Actor );

		if(LandscapeProxy)
		{
			ULandscapeInfo* Info = LandscapeProxy->GetLandscapeInfo();
			if( Info != nullptr)
			{
				FMaterialUpdateContext MaterialUpdateContext;
				Info->UpdateLayerInfoMap(/*this*/ );

				//ChangedMaterial = true;

				// Clear the parents out of combination material instances
				for( const auto& MICPair : LandscapeProxy->MaterialInstanceConstantMap )
				{
					UMaterialInstanceConstant* MaterialInstance = MICPair.Value;
					MaterialInstance->BasePropertyOverrides.bOverride_BlendMode = false;
					MaterialInstance->SetParentEditorOnly( nullptr );
					MaterialUpdateContext.AddMaterialInstance( MaterialInstance );
				}

				// Remove our references to any material instances
				LandscapeProxy->MaterialInstanceConstantMap.Empty();
			}

			LandscapeProxy->UpdateAllComponentMaterialInstances();
		}
		AInstancedFoliageActor* FoliageActor = Cast<AInstancedFoliageActor>( Actor );
		if (FoliageActor)
		{
			auto& Infos = GetFoliageInfos( FoliageActor );
			for (auto& Pair : Infos)
			{
				FFoliageInfo& FoliageInfo = (FFoliageInfo&)Pair.Value.Get();
				//if (!FoliageInfo) continue;

				UHierarchicalInstancedStaticMeshComponent* HISMComponent = FoliageInfo.GetComponent();
				if (HISMComponent)
				{
					#if ENGINE_MAJOR_VERSION == 4
						FoliageInfo.Refresh( FoliageActor, false, true ); // Synchronous refresh
					#else
						FoliageInfo.Refresh( false, true ); // Synchronous refresh
					#endif

					// Rebuild culling tree for this component
					HISMComponent->BuildTreeIfOutdated( false, true );
				}
			}
		}
	}
#endif
}
void FDowngraderModule::DeleteUnloadedWorldPartitionActors()
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5
	UWorld* world = GEditor->GetEditorWorldContext().World();

	auto LevelCollection = world->GetActiveLevelCollection();

	const IWorldPartitionCell* Cell = LevelCollection->GetPersistentLevel()->GetWorldPartitionRuntimeCell();
	UWorldPartition* WP = LevelCollection->GetPersistentLevel()->GetWorldPartition();

	UWorldPartitionEditorSpatialHash* EditorSpatialHash = (UWorldPartitionEditorSpatialHash*)WP->EditorHash;

	FBox WorldBounds = EditorSpatialHash->GetEditorWorldBounds();
	TArray<FWorldPartitionActorDescInstance*> ActorInstances;
	TArray<FGuid> ActorGUIDs;
	
	EditorSpatialHash->ForEachIntersectingActor( WorldBounds, [&]( FWorldPartitionActorDescInstance* ActorDescInstance )
	{
		const FGuid& GUID = ActorDescInstance->GetGuid();
		ActorInstances.Add( ActorDescInstance );
		ActorGUIDs.Add( GUID );
	} );
	
	TArray<FGuid> ActorsLoaded;
	for( TActorIterator<AActor> Iterator( world ); Iterator; ++Iterator )
	{
		AActor* Actor = *Iterator;

		const FGuid GUID = Actor->GetActorGuid();
		const FGuid InstanceGUID = Actor->GetActorInstanceGuid();
		
		auto Index = ActorGUIDs.Find( GUID );
		if( Index != INDEX_NONE )
		{
			ActorGUIDs.RemoveAt( Index );
			ActorInstances.RemoveAt( Index );
		}
		auto InstanceIndex = ActorGUIDs.Find( InstanceGUID );
		if( InstanceIndex != INDEX_NONE )
		{
			ActorGUIDs.RemoveAt( InstanceIndex );
			ActorInstances.RemoveAt( InstanceIndex );
		}
	}

	int Removals = 0;
	for( int i = 0; i < ActorInstances.Num(); i++ )
	{
		FWorldPartitionActorDescInstance* ActorDescInstance = ActorInstances[ i ];
		UActorDescContainerInstance* ContainerInstance = ActorDescInstance->GetContainerInstance();
		UActorDescContainer* Container = ( UActorDescContainer* )ContainerInstance->GetContainer();
		bool Removed = Container->RemoveActor( ActorDescInstance->GetGuid() );
		if( Removed )
			Removals++;
	}
#endif
}
void FDowngraderModule::RemoveWorldPartitionFromSelectedAssets()
{
	int NumAssets = GetNumSelectedAssets();
	FScopedSlowTask LoadingProgress( NumAssets );
	LoadingProgress.MakeDialog();

	int Index = 0;
	UWorld* world = GEditor->GetEditorWorldContext().World();

	auto lambda = [&]( FAssetData& Asset )
	{
		FString Text = FString::Printf( TEXT( "RemoveWorldPartitionFromSelectedAssets %d/%d" ), Index + 1, NumAssets );
		FText StrTxt = FText::FromString( Text );
		LoadingProgress.EnterProgressFrame( 1, StrTxt );
		Index++;
		
		RemoveWorldPartition( Asset );
	};

	IterateOverSelection( lambda );
}
void FDowngraderModule::BreakPackedActors()
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
	UWorld* world = GEditor->GetEditorWorldContext().World();
	ULevelInstanceSubsystem* LevelInstanceSubsystem = world->GetSubsystem<ULevelInstanceSubsystem>();
	TArray<ILevelInstanceInterface*> BreakableLevelInstances;

	for (TActorIterator<AActor> Iterator( world ); Iterator; ++Iterator)
	{
		AActor* Actor = *Iterator;
		APackedLevelActor* PackedLevelActor = Cast< APackedLevelActor>( Actor );
		if (PackedLevelActor)
		{
			BreakableLevelInstances.Add( PackedLevelActor );
		}
	}
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4
	ELevelInstanceBreakFlags Flags = ELevelInstanceBreakFlags::None;
#endif
	int32 BreakLevels = 1;
	bool Modified = false;
	for (ILevelInstanceInterface* LevelInstance : BreakableLevelInstances)
	{
		LevelInstanceSubsystem->BreakLevelInstance( LevelInstance, BreakLevels, nullptr
			#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4
			, Flags
			#endif
		);
		Modified = true;
	}
	if (Modified)
	{
		world->PostEditChange();
		world->MarkPackageDirty();
	}

	FString Text = FString::Printf( TEXT( "%d PackedLevelActors were broken down !" ), BreakableLevelInstances.Num() );
	FText Txt = FText::FromString( Text );
	FMessageDialog::Open( EAppMsgType::Ok, Txt );
#endif
}

FString NormalizeConfigIniPath( FString NonNormalizedPath )
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5
	return FConfigCacheIni::NormalizeConfigIniPath( NonNormalizedPath );
#else
	FString Result = NonNormalizedPath;
	FPaths::RemoveDuplicateSlashes( Result );
	return FPaths::CreateStandardFilename( Result );
#endif
}
void SortIniSection( const FString& IniFileName, const FString& SectionName )
{
	if (!GConfig->DoesSectionExist( *SectionName, IniFileName ))
	{
		UE_LOG( LogTemp, Warning, TEXT( "Section '%s' does not exist in '%s'." ), *SectionName, *IniFileName );
		return;
	}

	// Read all existing key-value pairs from the section
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4
	const FConfigSection* Section = GConfig->GetSection( *SectionName, false, IniFileName );	
#else
	const FConfigSection* Section = GConfig->GetSectionPrivate( *SectionName, false, false, IniFileName );
#endif
	
	FConfigSection KeyValueMap = *(FConfigSection*)Section;
	// Extract and sort keys alphabetically
	TArray<FName> Keys;
	KeyValueMap.GetKeys( Keys );
	Keys.Sort( []( const FName& A, const FName& B )
		{
			return A.LexicalLess( B );
		} );
	//
	// Clear the original section
	GConfig->EmptySection( *SectionName, IniFileName );
	
	// Write back the key-value pairs in sorted order
	for (const FName& Key : Keys)
	{
		//FString Value;
		auto ConfigValue = KeyValueMap.Find( Key );
		GConfig->SetString( *SectionName, *Key.ToString(), *ConfigValue->GetValue(), IniFileName);
	}
	
	// Flush changes to disk
	GConfig->Flush( false, IniFileName );
}
void FDowngraderModule::WriteCustomVersions()
{
	FText FailureMessage;
	bool SaveProject = false;
	bool Result = false;
	bool EnableAllPlugins = false;
	FText Txt = FText::FromString( TEXT("Do you want to enable all plugins ?"));
	if (FMessageDialog::Open( EAppMsgType::YesNo, Txt ) == EAppReturnType::Yes)
		EnableAllPlugins = true;
//#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
	if( EnableAllPlugins )
	{
		TArray<FString> ExcludedPlugins;
		ExcludedPlugins.Add( TEXT( "OnlineSubsystemTencent" ) );
		ExcludedPlugins.Add( TEXT( "ScriptPlugin" ) );//makes the project not build in 5.6

		TArray<TSharedRef<IPlugin>> Plugins = IPluginManager::Get().GetDiscoveredPlugins();
		for( int i = 0; i < Plugins.Num(); i++ )
		{
			TSharedPtr<IPlugin> Plugin = Plugins[ i ];
			auto Descriptor = Plugin->GetDescriptor();
			bool SupportedPlatform = Descriptor.SupportsTargetPlatform( TEXT( "Win64" ) );
			bool SupportedType = false;
			for( int m = 0; m < Descriptor.Modules.Num(); m++ )
			{
				FModuleDescriptor ModuleDescriptor = Descriptor.Modules[ m ];
				if( ModuleDescriptor.Type == EHostType::Type::Runtime ||
					ModuleDescriptor.Type == EHostType::Type::RuntimeAndProgram )
				{
					SupportedType = true;
				}
			}
			bool IsExcluded = false;
			for( int e = 0; e < ExcludedPlugins.Num(); e++ )
			{
				if( Plugin->GetName().Contains( ExcludedPlugins[ e ] ) )
				{
					IsExcluded = true;
					break;
				}
			}
			if( !Plugin->IsEnabled() && SupportedPlatform && SupportedType && !IsExcluded )
			{
				SaveProject = true;
				Result = IProjectManager::Get().SetPluginEnabled( Plugin->GetName(), true, FailureMessage );
			}
		}
		if( SaveProject )
		{
		//#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5
			FGameProjectGenerationModule::Get().TryMakeProjectFileWriteable( FPaths::GetProjectFilePath() );
			Result = IProjectManager::Get().SaveCurrentProjectToDisk( FailureMessage );
			FUnrealEdMisc::Get().RestartEditor( true );
		//#endif
		}
	}
//#endif
	TSharedPtr<IPlugin> ThisPlugin = IPluginManager::Get().FindPlugin( TEXT( "Downgrader" ) );
	FString EngineVersion = FString::Printf( TEXT( "%d.%d" ), ENGINE_MAJOR_VERSION, ENGINE_MINOR_VERSION );
	bool Write = true;
	if (Write)
	{
		struct CustomVersionData
		{
			FString Name;
			FString GUID;
			int Version;
		};
		TArray< CustomVersionData > CustomVersionDataArray;
		FString Category = FString::Printf( TEXT( "CustomVersions_%s" ), *EngineVersion );

		FString IniFile = ThisPlugin->GetBaseDir() / FString::Printf( TEXT( "Config/%s.ini" ), *Category );
		FConfigFile& ConfigFile = GConfig->Add( IniFile, FConfigFile() );
		
		FCustomVersionContainer Container = FCurrentCustomVersions::GetAll();
		const FCustomVersionArray& Array = Container.GetAllVersions();
		for (int i = 0; i < Array.Num(); i++)
		{
			const FCustomVersion& CustomVersion = Array[i];
			FString Name = CustomVersion.GetFriendlyName().ToString();
			FString GUID = CustomVersion.Key.ToString();
			int Version = CustomVersion.Version;

			CustomVersionData NewData;
			NewData.Name = Name;
			NewData.GUID = GUID;
			NewData.Version = Version;

			CustomVersionDataArray.Add( NewData );
		}

		CustomVersionDataArray.Sort( []( const CustomVersionData& A, const CustomVersionData& B )
			{
				return FCString::Strcmp( *A.Name, *B.Name ) < 0;
			} );

		for (int i = 0;i < CustomVersionDataArray.Num(); i++)
		{
			CustomVersionData& Entry = CustomVersionDataArray[i];

			GConfig->SetInt( *Category, *Entry.Name, Entry.Version, *IniFile );
			GConfig->SetInt( *Category, *Entry.GUID, Entry.Version, *IniFile );
		}

		GConfig->Flush( false, IniFile );
	}
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4

	const int NumVersions = sizeof( AllVersionStrings ) / sizeof( AllVersionStrings[ 0 ] );
	const FConfigSection* Sections[ NumVersions ] = { 0 };
	for( int i = 0; i <= (int)EEngineVersion::EV_Latest; i++ )
	{
		EEngineVersion Version = (EEngineVersion)i;
		Sections[ i ] = GetCustomVersionSectionFor( Version );
	}
#endif

#if (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 27) || ENGINE_MAJOR_VERSION == 5
	FString NiagaraIniFile = ThisPlugin->GetBaseDir() / FString::Printf( TEXT( "Config/NiagaraVersions_%s.ini" ), *EngineVersion );
	FConfigFile& NiagaraConfigFile = GConfig->Add( NiagaraIniFile, FConfigFile() );
	FString NiagaraSection = TEXT( "Modules" );

	auto lambda = [&]( FAssetData& Asset )
	{
		UObject* AssetObject = Asset.GetAsset();
		UNiagaraScript* Script = Cast<UNiagaraScript>( AssetObject );
		if ( Script )
		{
			TArray<FNiagaraAssetVersion> Versions = Script->GetAllAvailableVersions();
			//if (Versions.Num() > 1)
			{
				int HighestMin = -1;
				int HighestMajor = -1;
				FGuid SelectedGUID;
				for (int i = 0; i < Versions.Num(); i++)
				{
					FNiagaraAssetVersion& Version = Versions[i];
					if (Version.MajorVersion > HighestMajor ||
						(Version.MajorVersion == HighestMajor && Version.MinorVersion > HighestMin)
						)
					{
						HighestMajor = Version.MajorVersion;
						HighestMin = Version.MinorVersion;
						SelectedGUID = Version.VersionGuid;
					}
				}

				int Version = HighestMajor * 10 + HighestMin;
				//float Version = (float)HighestMajor + (float)HighestMin * 0.1;
				GConfig->SetInt( *NiagaraSection, *Script->GetName(), Version, NiagaraIniFile );
			}
		}
	};

	IterateOverAllAssetsFromPath( TEXT( "/Niagara/" ), lambda );

	SortIniSection( NiagaraIniFile, NiagaraSection );
	GConfig->Flush( false, NiagaraIniFile );
#endif
}

FString SourceFolder = TEXT( "H:\\UnrealEngine3\\" );
//FString LauncherVersionFolder = TEXT( "C:\\UE_5.6\\" );
FString TargetFolder;

void CopyToOutput( FString SourceFile, FString OutFilePath )
{
	FString SourceFilePath = SourceFile;
	FString To = OutFilePath;

	if (FPlatformFileManager::Get().GetPlatformFile().IsReadOnly( *To ))
		FPlatformFileManager::Get().GetPlatformFile().SetReadOnly( *To, false );

	bool Result = FPlatformFileManager::Get().GetPlatformFile().CopyFile( *To, *SourceFilePath );
	if( !Result )
	{
		if( !FPlatformFileManager::Get().GetPlatformFile().FileExists( *SourceFilePath ) )
		{
			FString Text = FString::Printf( TEXT( "SourceFile %s not found !" ), *SourceFilePath );
			FText Txt = FText::FromString( Text );
			FMessageDialog::Open( EAppMsgType::Ok, Txt );
		}
		else
		{
			FString Text = FString::Printf( TEXT( "CopyToOutput Error on SourceFile=%s OutFilePath=%s" ), *SourceFilePath, *To );
			FText Txt = FText::FromString( Text );
			FMessageDialog::Open( EAppMsgType::Ok, Txt );
		}
	}
}
void CopyToOutput( TArray<FString>& Files, FString Name )
{
	FScopedSlowTask LoadingProgress( Files.Num() );
	LoadingProgress.MakeDialog();

	for( int i = 0; i < Files.Num(); i++ )
	{
		FString Text = FString::Printf( TEXT( "%s %d/%d" ), *Name, i + 1, Files.Num() );
		FText StrTxt = FText::FromString( Text );
		LoadingProgress.EnterProgressFrame( 1, StrTxt );

		FString& File = Files[ i ];
		FString Left, Right;
		File.Split( SourceFolder, &Left, &Right );

		FString OutFile = TargetFolder + Right;

		FString OutFolder = FPaths::GetPath( OutFile );
		if (!FPlatformFileManager::Get().GetPlatformFile().DirectoryExists( *OutFolder ))
			continue;

		CopyToOutput( File, OutFile );
	}
}
TArray<FString> FilterOutFiles( TArray<FString> Files, FString Filter )
{
	TArray<FString> Ret;
	for( int i = 0; i < Files.Num(); i++ )
	{
		if( !Files[ i ].Contains( Filter ) )
		{
			Ret.Add( Files[ i ] );
		}
	}

	return Ret;
}
void AddPluginBinaries( FString Path, TArray<FString>& Files )
{
	FString FinalPath = /*SourceFolder +*/ Path;

	if (!FPlatformFileManager::Get().GetPlatformFile().DirectoryExists( *(FinalPath + TEXT( "\\Binaries" ) )))
		return;

	TArray<FString> BinaryFiles;
	FString DLLPath = FinalPath + "\\Binaries\\Win64\\";
	FPlatformFileManager::Get().GetPlatformFile().FindFiles( BinaryFiles, *DLLPath, TEXT( ".dll" ) );

	for( int i = 0; i < BinaryFiles.Num(); i++ )
	{
		Files.Add( BinaryFiles[ i ]);
	}

	if( BinaryFiles.Num() == 0 )
	{
		FString DialogText = FString::Printf( TEXT("AddBinaries(%s) found 0 files!\n"), *Path );
		FText Txt = FText::FromString( DialogText );
		FMessageDialog::Open( EAppMsgType::Ok, Txt );
	}
}
void FindFolders( FString Path, FString Extension, TArray<FString>& Folders )
{
	TArray<FString> FoundFiles;
	FPlatformFileManager::Get().GetPlatformFile().FindFilesRecursively( FoundFiles, *Path, *Extension );
	for (int i = 0; i < FoundFiles.Num(); i++)
	{
		FString Folder = FPaths::GetPath( FoundFiles[i] );

		if (!Folders.Contains( Folder ))
		{
			Folders.Add( Folder );
		}
	}
}
void FDowngraderModule::UpdateUEBuild()
{
	TargetFolder = FString::Printf( TEXT( "C:\\UE_%d.%d_Downgrader\\" ), ENGINE_MAJOR_VERSION, ENGINE_MINOR_VERSION );

	FString BinariesFilePath = SourceFolder + TEXT( "Engine\\Binaries\\Win64\\" );
	TArray<FString> BinaryFiles;
	FPlatformFileManager::Get().GetPlatformFile().FindFiles( BinaryFiles, *BinariesFilePath, TEXT( ".exe" ));
	FPlatformFileManager::Get().GetPlatformFile().FindFiles( BinaryFiles, *BinariesFilePath, TEXT( ".dll" ) );

	//AddPluginBinaries( TEXT( "Engine\\Plugins\\Animation\\ControlRig" ), PluginFiles );
	//AddPluginBinaries( TEXT( "Engine\\Plugins\\Experimental\\StructUtils" ), PluginFiles );
	//AddPluginBinaries( TEXT( "Engine\\Plugins\\FX\\Niagara" ), PluginFiles );
	//AddPluginBinaries( TEXT( "Engine\\Plugins\\Runtime\\HairStrands" ), PluginFiles );
	//AddPluginBinaries( TEXT( "Engine\\Plugins\\Runtime\\RigVM" ), PluginFiles );
	//AddPluginBinaries( TEXT( "Engine\\Plugins\\PCG" ), PluginFiles );
	//AddPluginBinaries( TEXT( "Engine\\Plugins\\ChaosVD" ), PluginFiles );

	TArray<FString> PluginFiles;
	TArray<FString> PluginFolders;
	FindFolders( *(SourceFolder + TEXT( "Engine/Plugins/" )), TEXT( ".uplugin" ), PluginFolders );
	for (int i = 0; i < PluginFolders.Num(); i++)
	{
		AddPluginBinaries( PluginFolders[i], PluginFiles);
	}

	for( int i = 0; i < PluginFiles.Num(); i++ )
	{
		BinaryFiles.Add( PluginFiles[ i ] );
	}

	BinaryFiles = FilterOutFiles( BinaryFiles, TEXT( "LiveLink" ) );
	BinaryFiles = FilterOutFiles( BinaryFiles, TEXT( "-Win64-Debug" ) );
	BinaryFiles = FilterOutFiles( BinaryFiles, TEXT( "-Debug.modules" ) );
	BinaryFiles = FilterOutFiles( BinaryFiles, TEXT( ".sup.lib" ) );
	BinaryFiles = FilterOutFiles( BinaryFiles, TEXT( "linux" ) );
	BinaryFiles = FilterOutFiles( BinaryFiles, TEXT( "MetalShaderFormat" ) );
	//BinaryFiles = FilterOutFiles( BinaryFiles, TEXT( "SequencerAnimTools" ) );
	
	TArray<FString> SourceFiles;
	SourceFiles.Add( *(SourceFolder + TEXT( "Engine/Source/Runtime/Engine/Public/WorldPartition/WorldPartitionActorDescArchive.h" )) );
	SourceFiles.Add( *(SourceFolder + TEXT( "Engine/Source/Runtime/Engine/Classes/Animation/AnimSequence.h" )) );
	SourceFiles.Add( *(SourceFolder + TEXT( "Engine/Source/Runtime/Experimental/Chaos/Public/GeometryCollection/ManagedArrayCollection.h" )) );

	//TArray<FString> ShaderFiles;
	//FPlatformFileManager::Get().GetPlatformFile().FindFilesRecursively( ShaderFiles, *(SourceFolder + TEXT( "Engine/Shaders/" )), TEXT( ".usf" ) );
	//FPlatformFileManager::Get().GetPlatformFile().FindFilesRecursively( ShaderFiles, *(SourceFolder + TEXT( "Engine/Shaders/" )), TEXT( ".ush" ) );

	CopyToOutput( BinaryFiles, TEXT( "Copying binary files" ) );
	CopyToOutput( SourceFiles, TEXT( "Copying source files" ) );
	//CopyToOutput( ShaderFiles, TEXT( "Copying shader files" ) );
	
	//Make sure there's no plugin from my installation of UE in there
	FString PluginPath = TargetFolder + TEXT( "/Engine/Plugins/Marketplace/" );
	FPlatformFileManager::Get().GetPlatformFile().DeleteDirectoryRecursively( *PluginPath );

	//Delete PDBs
	TArray<FString> PDBFiles;
	FPlatformFileManager::Get().GetPlatformFile().FindFilesRecursively( PDBFiles, *TargetFolder, TEXT(".pdb"));
	for( int i = 0; i < PDBFiles.Num(); i++ )
	{
		FPlatformFileManager::Get().GetPlatformFile().DeleteFile( *PDBFiles[ i ] );
	}
}
void DeletePluginDirectories( FString PluginName, EEngineVersion FromVersion, EEngineVersion ToVersion )
{
	for (int i = (int)FromVersion; i <= (int)ToVersion; i++)
	{
		EEngineVersion Version = (EEngineVersion)i;
		FString VersionName = AllVersionStrings[i];

		FString Path = FString::Printf( TEXT( "C:/UnrealProjects/Test%s/Plugins/%s" ), *VersionName, *PluginName );
		FPlatformFileManager::Get().GetPlatformFile().DeleteDirectoryRecursively( *Path );
	}
}
bool CopyToEngineVersion( FString PluginName, const TCHAR* DestinationDirectory, const TCHAR* Source, FString Version )
{
	bool Result = true;
//#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
	FString PluginFile = Source;
	
	PluginFile += FString::Printf( TEXT( "/%s.uplugin" ), *PluginName );
	FString OriginalData;
	Result = FFileHelper::LoadFileToString( OriginalData, *PluginFile );
	if (OriginalData.Len() > 0)
	{
		FString FileAsString = OriginalData;
		FString Marker = TEXT("\"EngineVersion\": \"");
		int pos = FileAsString.Find( Marker );
		if (pos != -1)
		{
			int InsertPos = pos + Marker.Len();
			FileAsString.InsertAt( InsertPos, Version );

			Result = FFileHelper::SaveStringToFile( FileAsString, *PluginFile );
		}
		else
			return false;
	}
	else
		return false;

	Result = FPlatformFileManager::Get().GetPlatformFile().DeleteDirectoryRecursively( DestinationDirectory ) && Result;
	Result = FPlatformFileManager::Get().GetPlatformFile().CopyDirectoryTree( DestinationDirectory, Source, true ) && Result;

	if (OriginalData.Len() > 0)
	{
		//Write back unmodified uplugin so I can repeat the operation
		Result = FFileHelper::SaveStringToFile( OriginalData, *PluginFile );
	}
//#endif
	return Result;
}
bool CopyToMultipleEngineVersions( FString PluginName, EEngineVersion FromVersion, EEngineVersion ToVersion )
{
	bool Result = true;
	for (int i = (int)FromVersion; i <= (int)ToVersion; i++)
	{
		EEngineVersion Version = (EEngineVersion)i;
		
		FString VersionName = AllVersionStrings[i];
		FString VersionNameWithUnderscore = VersionName.Replace( TEXT( "." ), TEXT( "_" ) );
		
		FString PluginFolder = FString::Printf( TEXT( "C:/MyProjects/Unreal/%s" ), *PluginName );
		FString DestinationFolder = FString::Printf( TEXT( "C:/UnrealProjects/Test%s/Plugins/%s" ), *VersionNameWithUnderscore, *PluginName );
		Result = CopyToEngineVersion( PluginName, *DestinationFolder, *PluginFolder, VersionName ) && Result;
	}
	return Result;
}
void FDowngraderModule::UpdateAllPluginVersions()
{
	FString PluginName = TEXT( "Downgrader" );
	DeletePluginDirectories( PluginName, EEngineVersion::EV_4_26, EEngineVersion::EV_Latest );

	bool Result;
	Result = CopyToMultipleEngineVersions( PluginName, EEngineVersion::EV_4_26, EEngineVersion::EV_Latest );
}
//bool UDowngraderFunctionDependencies::Map_IsNotEmpty( const TMap<int32, int32>& TargetMap )
//{
//	return TargetMap.Num() > 0;
//}
bool UDowngraderFunctionDependencies::GenericMap_IsNotEmpty( const void* TargetMap, const FMapProperty* MapProperty )
{
	if( TargetMap )
	{
		FScriptMapHelper MapHelper( MapProperty, TargetMap );
		return MapHelper.Num() > 0;
	}
	return false;
}

URigVMBlueprintGeneratedClass_Downgrader::URigVMBlueprintGeneratedClass_Downgrader( const FObjectInitializer& ObjectInitializer )
	: Super( ObjectInitializer )
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE( FDowngraderModule, Downgrader )

/*Backup code for engine modifications
* 
* class SerializeMarker
{
public:
	int Offset = 0;
	int ID = 0;
};

static TArray< SerializeMarker > SaveMarkers;
static TArray< SerializeMarker > LoadMarkers;
static void AddMarker( FArchive& Ar, int ID, int TellStart )
{
	SerializeMarker NewMarker;
	NewMarker.Offset = Ar.Tell() - TellStart;
	NewMarker.ID = ID;

	if( NewMarker.Offset == 0 )
		return;

	if( Ar.IsLoading() )
	{
		LoadMarkers.Add( NewMarker );
	}
	else
		SaveMarkers.Add( NewMarker );
}

class PropertySerialization
{
public:
	PropertySerialization();
	FProperty* Property = nullptr;
	int Loaded = 0;
	int Saved = 0;
};
extern COREUOBJECT_API bool EnableSerializeStats;
extern COREUOBJECT_API TArray<PropertySerialization> SerializeStats;
extern COREUOBJECT_API TArray<FProperty*> SerializeList;
extern COREUOBJECT_API bool EnableSerializeList;

COREUOBJECT_API void DummyDebugFunction( bool Before, FArchive& Ar )
{
	if( Before )
	{
		EnableSerializeList = true;
		SerializeList.Reset( 0 );
		SerializeStats.Reset( 0 );
		BeforeArchive = (int)Ar.Tell();
	}
	else
	{
		int TotalArchive = (int)Ar.Tell() - BeforeArchive;
		int Total = 0;
		for( int i=0; i < SerializeStats.Num(); i++ )
		{
			Total += SerializeStats[ i ].Saved + SerializeStats[ i ].Loaded;
		}
		bool Store = true;
		if( Store )
		{
			SavedStats = SerializeStats;
			SavedTotal = Total;
			SavedArchiveSize = TotalArchive;
		}
		EnableSerializeList = false;
	}
}
void NormalizeWeightsInUint8( uint16* InfluenceWeights, uint8* OutWeights, int TotalInfluences )
{
	float Sum = 0;
	float Weights[ MAX_TOTAL_INFLUENCES ] = { 0 };
	for( int i = 0; i < TotalInfluences; i++ )
	{
		uint16 Influence16 = InfluenceWeights[ i ] >> 8;
		Weights[i] = (float)Influence16 / 255.0f;
		Sum += Weights[ i ];
	}
	for( int i = 0; i < TotalInfluences; i++ )
	{
		float NormalizedWeight = Weights[ i ] / Sum;
		OutWeights[i] = (uint8)( NormalizedWeight * 255.0f );
	}
}
*/
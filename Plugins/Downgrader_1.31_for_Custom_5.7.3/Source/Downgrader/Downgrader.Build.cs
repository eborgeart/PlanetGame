// Copyright Ciprian Stanciu 2024
using UnrealBuildTool;
public class Downgrader : ModuleRules
{
    public Downgrader( ReadOnlyTargetRules Target ) : base( Target )
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange( new string[] { "Core", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay",
            "RenderCore",
            "MaterialEditor",
            "UnrealEd",
            "RHI",
            "Slate",
            "InputCore",
            "SlateCore",
            "EditorStyle",
            "Foliage",
            "AssetRegistry",
            "ControlRig",
            "ControlRigDeveloper",
            "Niagara",
            "NiagaraCore",
            "BlueprintGraph",
            "RigVM",
            "RigVMDeveloper",
            "Blutility",
            "UMG",
            "UMGEditor",
            "MeshDescription",
            "StaticMeshDescription",
            "EngineSettings",
            "RawMesh",
            "Projects",
            "Landscape",
            "AnimGraph",
            //"PropertyAccessNode"
            "NiagaraEditor",
            "GeometryCollectionEngine",
            "LevelSequence",
            "NavigationSystem",
            "GameProjectGeneration",
            "SourceControl",
            "EnhancedInput"       
            } );

        PrivateDependencyModuleNames.AddRange( new string[] {"Core", "CoreUObject", "Engine", "InputCore",
            "RenderCore",
            "MaterialEditor",
            "UnrealEd",
            "RHI",
            "Slate",
            "InputCore",
            "SlateCore",
            "EditorStyle",
            "Foliage",
            "AssetRegistry",
            "ControlRig",
            "ControlRigDeveloper",
            "Niagara",
            "NiagaraCore",
            "BlueprintGraph",
            "RigVM",
            "RigVMDeveloper",
            "Blutility",
            "UMG",
            "UMGEditor",
            "MeshDescription",
            "StaticMeshDescription",
            "EngineSettings",
            "RawMesh",
            "Projects",
            "Landscape",
            "AnimGraph",
            //"PropertyAccessNode"
            "NiagaraEditor",
            "GeometryCollectionEngine",
            "LevelSequence",
            "NavigationSystem",
            "GameProjectGeneration",
            "SourceControl",
            "EnhancedInput"            
            } );
        if( (Target.Version.MajorVersion == 4 && Target.Version.MinorVersion >= 27) ||
            (Target.Version.MajorVersion == 5 && Target.Version.MinorVersion >= 0) )
        {
            PublicDependencyModuleNames.Add("HairStrandsCore");
            PrivateDependencyModuleNames.Add("HairStrandsCore");
        }
        if( Target.Version.MajorVersion == 5 && Target.Version.MinorVersion >= 0 )
        {
            PublicDependencyModuleNames.Add( "InterchangeCore" );
            PublicDependencyModuleNames.Add( "InterchangeEngine" );
            PublicDependencyModuleNames.Add( "IKRig" );
            PublicDependencyModuleNames.Add( "MetasoundEngine" );
            PublicDependencyModuleNames.Add( "MetasoundFrontend" );
            PublicDependencyModuleNames.Add( "GeometryFramework" );
            PublicDependencyModuleNames.Add( "AnimationWarpingEditor" );
            PublicDependencyModuleNames.Add("Chaos");
            //PublicDependencyModuleNames.Add( "ImageCore" );
            
            //
            PrivateDependencyModuleNames.Add( "InterchangeCore" );
            PrivateDependencyModuleNames.Add( "InterchangeEngine" );
            PrivateDependencyModuleNames.Add( "IKRig" );
            PrivateDependencyModuleNames.Add( "MetasoundEngine" );
            PrivateDependencyModuleNames.Add( "MetasoundFrontend" );
            PrivateDependencyModuleNames.Add( "GeometryFramework" );
            PrivateDependencyModuleNames.Add( "AnimationWarpingEditor" );
            PrivateDependencyModuleNames.Add("Chaos");
            //PrivateDependencyModuleNames.Add( "ImageCore" );
        }
        if( Target.Version.MajorVersion == 5 && Target.Version.MinorVersion >= 1 )
        {
            PublicDependencyModuleNames.Add( "PCG" );            

            PrivateDependencyModuleNames.Add( "PCG" );            
        }
        if( Target.Version.MajorVersion == 5 && Target.Version.MinorVersion >= 2 )
        {
            PublicDependencyModuleNames.AddRange( new string[] { "DataflowEngine", "DataflowCore" } );

            PrivateDependencyModuleNames.AddRange( new string[] { "DataflowEngine", "DataflowCore" } );
        }
        if( Target.Version.MajorVersion == 5 && Target.Version.MinorVersion >= 4 )
        {
            PublicDependencyModuleNames.Add( "PoseSearch" );
            PublicDependencyModuleNames.Add( "BlendStackEditor" );

            PrivateDependencyModuleNames.Add( "PoseSearch" );
            PrivateDependencyModuleNames.Add( "BlendStackEditor" );
        }
        if (Target.Version.MajorVersion == 5 && Target.Version.MinorVersion >= 5)
        {
            PublicDependencyModuleNames.Add("TextureUtilitiesCommon");

            PrivateDependencyModuleNames.Add("TextureUtilitiesCommon");
        }
       
        //if( Target.Version.Changelist == 0 )
        //{
        //    this.PublicDefinitions.Add( "DOWNGRADER_CUSTOM_ENGINE=1" );
        //}
    }
}

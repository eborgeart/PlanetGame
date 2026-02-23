// Copyright Ciprian Stanciu 2024

#pragma once

#include "Runtime/Launch/Resources/Version.h"
#include "Modules/ModuleManager.h"
#include "EditorStyleSet.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Framework/Commands/Commands.h"
#include "EdGraph/EdGraphNode.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "DowngraderModule.generated.h"

UENUM()
enum class EEngineVersion : uint8
{
	EV_4_26 UMETA( DisplayName = "4.26.2" ),
	EV_4_27 UMETA( DisplayName = "4.27.2" ),
	EV_5_0 UMETA( DisplayName = "5.0.3" ),
	EV_5_1 UMETA( DisplayName = "5.1.1" ),
	EV_5_2 UMETA( DisplayName = "5.2.1" ),
	EV_5_3 UMETA( DisplayName = "5.3.2" ),
	EV_5_4 UMETA( DisplayName = "5.4.4" ),
	EV_5_5 UMETA( DisplayName = "5.5.4" ),
	EV_5_6 UMETA( DisplayName = "5.6.1" ),
	EV_5_7 UMETA( Hidden ),
	EV_Latest = EV_5_7 UMETA( Hidden )
};

UCLASS( config = EditorPerProjectUserSettings, MinimalAPI, BlueprintType )
class UDowngraderParams : public UObject
{
	GENERATED_BODY()
public:

	UDowngraderParams();

	UPROPERTY( EditAnywhere, BlueprintReadWrite, config, Category = Parameters )
	EEngineVersion TargetVersion;
	//UPROPERTY( EditAnywhere, BlueprintReadWrite, config, Category = Parameters )
	//bool ExternalActors;
};

extern UDowngraderParams* DowngraderSettings;

class FDowngraderModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	void BindEditorCommands();
	void AddCustomMenu( FToolBarBuilder& ToolbarBuilder );
	TSharedRef<SWidget> CreateMenuContent();

	void GlobalPostLoad();	
	void DowngradeSelectedAssets();
	void PreloadSelectedAssets();
	void SaveSelectedAssets();	
	void RevertSubstrateMaterials();
	void FixLandscape();
	void WriteCustomVersions();
	void DeleteUnloadedWorldPartitionActors();
	void RemoveWorldPartitionFromSelectedAssets();
	void BreakPackedActors();
	void UpdateUEBuild();
	void UpdateAllPluginVersions();

	TSharedPtr<FUICommandList> EditorCommands;
};


class FDowngrader_Commands : public TCommands<FDowngrader_Commands>
{
public:
	FDowngrader_Commands()
		: TCommands<FDowngrader_Commands>( "Downgrader", NSLOCTEXT( "Contexts", "Downgrader", "Downgrader" ), NAME_None, 
									   #if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
										   FAppStyle::GetAppStyleSetName()
									   #else
										   FEditorStyle::GetStyleSetName()
									   #endif
		)
	{
	}
	
	TSharedPtr<FUICommandInfo> DowngradeSelectedAssets;
	TSharedPtr<FUICommandInfo> PreloadSelectedAssets;
	TSharedPtr<FUICommandInfo> SaveSelectedAssets;
	TSharedPtr<FUICommandInfo> RevertSubstrateMaterials;
	TSharedPtr<FUICommandInfo> FixLandscape;
	TSharedPtr<FUICommandInfo> WriteCustomVersions;
	TSharedPtr<FUICommandInfo> DeleteUnloadedWorldPartitionActors;
	TSharedPtr<FUICommandInfo> RemoveWorldPartitionFromSelectedAssets;
	TSharedPtr<FUICommandInfo> BreakPackedActors;
	TSharedPtr<FUICommandInfo> UpdateUEBuild;
	TSharedPtr<FUICommandInfo> UpdateAllPluginVersions;
	
	virtual void RegisterCommands() override;
};

UCLASS( meta = ( BlueprintThreadSafe ), MinimalAPI )
class UDowngraderFunctionDependencies : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION( BlueprintPure, CustomThunk, meta = ( DisplayName = "Is Not Empty", CompactNodeTitle = "IS NOT EMPTY", MapParam = "TargetMap" ), Category = "Utilities|Map" )
	static bool Map_IsNotEmpty( const TMap<int32, int32>& TargetMap );

	static bool GenericMap_IsNotEmpty( const void* TargetMap, const FMapProperty* MapProperty );

	DECLARE_FUNCTION( execMap_IsNotEmpty )
	{
		Stack.MostRecentProperty = nullptr;
		Stack.StepCompiledIn<FMapProperty>( nullptr );
		void* MapAddr = Stack.MostRecentPropertyAddress;
		FMapProperty* MapProperty = CastField<FMapProperty>( Stack.MostRecentProperty );
		if( !MapProperty )
		{
			Stack.bArrayContextFailed = true;
			return;
		}

		P_FINISH;
		P_NATIVE_BEGIN;
		*(bool*)RESULT_PARAM = GenericMap_IsNotEmpty( MapAddr, MapProperty );
		P_NATIVE_END
	}
};
UCLASS( MinimalAPI )
class URigVMBlueprintGeneratedClass_Downgrader : public UBlueprintGeneratedClass
{
	GENERATED_UCLASS_BODY()

public:

};

struct FNiagaraVariableBase;
class UEdGraphNode;

UFunction* GetFunction( FString FuncName );
void RemoveFunctionFlag( FString FuncName, EFunctionFlags RemoveFlag );
FProperty* GetProperty( FString StructName, FString PropName );
void RemovePropertyFlag( FString StructName, FString PropName, EPropertyFlags RemoveFlag );
void RemoveGraphNode( UEdGraphNode* NodeToRemove );
bool FixGraphPinType( FEdGraphPinType& PinType );
void FixNiagaraVariable( FNiagaraVariableBase* Variable );
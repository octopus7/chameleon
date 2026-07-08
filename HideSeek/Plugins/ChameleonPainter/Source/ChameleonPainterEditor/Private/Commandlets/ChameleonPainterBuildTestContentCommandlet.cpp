#include "Commandlets/ChameleonPainterBuildTestContentCommandlet.h"

#include "Actors/ChameleonHiderBodyActor.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Camera/CameraActor.h"
#include "Character/ChameleonHiderCharacter.h"
#include "Components/ChameleonMetaballBodyComponent.h"
#include "Components/ChameleonPaintComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Data/ChameleonPainterInputConfig.h"
#include "Editor.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/Texture2D.h"
#include "EnhancedActionKeyMapping.h"
#include "FileHelpers.h"
#include "Factories/TextureFactory.h"
#include "Game/ChameleonPainterGameInstance.h"
#include "Game/ChameleonPainterGameMode.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/WorldSettings.h"
#include "GameMapsSettings.h"
#include "InputAction.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionTextureSample.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionVertexColor.h"
#include "MaterialEditingLibrary.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"

DEFINE_LOG_CATEGORY_STATIC(LogChameleonPainterContent, Log, All);

namespace
{
const FString RootPath = TEXT("/Game/ChameleonPainterTest");
const FString TexturePath = RootPath / TEXT("Textures");
const FString MaterialPath = RootPath / TEXT("Materials");
const FString InputPath = RootPath / TEXT("Input");
const FString BlueprintPath = RootPath / TEXT("Blueprints");
const FString MapPath = RootPath / TEXT("Maps");

FString ToObjectPath(const FString& PackageName)
{
	return PackageName + TEXT(".") + FPackageName::GetLongPackageAssetName(PackageName);
}

template <typename T>
T* LoadAssetByPackageName(const FString& PackageName)
{
	return Cast<T>(StaticLoadObject(T::StaticClass(), nullptr, *ToObjectPath(PackageName)));
}

template <typename T>
T* FindOrCreateAsset(const FString& PackageName)
{
	if (T* ExistingAsset = LoadAssetByPackageName<T>(PackageName))
	{
		return ExistingAsset;
	}

	UPackage* Package = CreatePackage(*PackageName);
	Package->FullyLoad();

	const FName AssetName(*FPackageName::GetLongPackageAssetName(PackageName));
	T* Asset = NewObject<T>(Package, T::StaticClass(), AssetName, RF_Public | RF_Standalone | RF_Transactional);
	FAssetRegistryModule::AssetCreated(Asset);
	Asset->MarkPackageDirty();
	return Asset;
}

bool SavePackageForObject(UObject* Asset)
{
	if (!Asset)
	{
		return false;
	}

	UPackage* Package = Asset->GetPackage();
	const FString Filename = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());

	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	SaveArgs.SaveFlags = SAVE_NoError;
	SaveArgs.Error = GWarn;

	return UPackage::SavePackage(Package, nullptr, *Filename, SaveArgs);
}

void SaveAssets(const TArray<UObject*>& Assets)
{
	for (UObject* Asset : Assets)
	{
		if (Asset)
		{
			SavePackageForObject(Asset);
		}
	}
}

TMap<FName, UTexture2D*> ImportTextures()
{
	const FString SourceDirectory = FPaths::Combine(FPaths::ProjectDir(), TEXT("SourceAssets/Textures/ChameleonPainterTest"));
	const TArray<FString> SourceFiles = {
		TEXT("T_CPT_Concrete.png"),
		TEXT("T_CPT_Wood.png"),
		TEXT("T_CPT_GreenPanel.png"),
		TEXT("T_CPT_FloorTile.png")
	};

	TMap<FName, UTexture2D*> Textures;
	for (const FString& SourceFile : SourceFiles)
	{
		const FString AssetName = FPaths::GetBaseFilename(SourceFile);
		const FString PackageName = TexturePath / AssetName;
		const FString FullPath = FPaths::Combine(SourceDirectory, SourceFile);

		UTexture2D* Texture = LoadAssetByPackageName<UTexture2D>(PackageName);
		if (!Texture && FPaths::FileExists(FullPath))
		{
			UPackage* Package = CreatePackage(*PackageName);
			Package->FullyLoad();

			UTextureFactory* TextureFactory = NewObject<UTextureFactory>();
			TextureFactory->AddToRoot();
			TextureFactory->bCreateMaterial = false;
			TextureFactory->ColorSpaceMode = ETextureSourceColorSpace::SRGB;
			UTextureFactory::SuppressImportOverwriteDialog(true);

			Texture = Cast<UTexture2D>(UFactory::StaticImportObject(
				UTexture2D::StaticClass(),
				Package,
				*AssetName,
				RF_Public | RF_Standalone | RF_Transactional,
				*FullPath,
				nullptr,
				TextureFactory,
				nullptr,
				GWarn));

			TextureFactory->RemoveFromRoot();
			if (Texture)
			{
				FAssetRegistryModule::AssetCreated(Texture);
			}
		}
		else if (!Texture)
		{
			UE_LOG(LogChameleonPainterContent, Warning, TEXT("Texture source file is missing: %s"), *FullPath);
		}

		if (Texture)
		{
			Texture->SRGB = true;
			Texture->MarkPackageDirty();
			Textures.Add(*AssetName, Texture);
			SavePackageForObject(Texture);
		}
	}

	return Textures;
}

UMaterial* CreateHiderPaintMaterial()
{
	UMaterial* Material = FindOrCreateAsset<UMaterial>(MaterialPath / TEXT("M_CPT_HiderPaint"));
	Material->PreEditChange(nullptr);
	UMaterialEditingLibrary::DeleteAllMaterialExpressions(Material);

	UMaterialExpressionVertexColor* VertexColor = Cast<UMaterialExpressionVertexColor>(
		UMaterialEditingLibrary::CreateMaterialExpression(Material, UMaterialExpressionVertexColor::StaticClass(), -360, -80));
	UMaterialExpressionConstant* Roughness = Cast<UMaterialExpressionConstant>(
		UMaterialEditingLibrary::CreateMaterialExpression(Material, UMaterialExpressionConstant::StaticClass(), -360, 120));

	if (Roughness)
	{
		Roughness->R = 0.84f;
	}

	UMaterialEditingLibrary::ConnectMaterialProperty(VertexColor, TEXT(""), MP_BaseColor);
	UMaterialEditingLibrary::ConnectMaterialProperty(Roughness, TEXT(""), MP_Roughness);
	UMaterialEditingLibrary::RecompileMaterial(Material);
	Material->PostEditChange();
	Material->MarkPackageDirty();
	SavePackageForObject(Material);
	return Material;
}

UMaterial* CreateSurfaceMaterial(const FString& AssetName, UTexture2D* Texture, FLinearColor SampleColor)
{
	UMaterial* Material = FindOrCreateAsset<UMaterial>(MaterialPath / AssetName);
	Material->PreEditChange(nullptr);
	UMaterialEditingLibrary::DeleteAllMaterialExpressions(Material);

	UMaterialExpressionVectorParameter* SampleColorParameter = Cast<UMaterialExpressionVectorParameter>(
		UMaterialEditingLibrary::CreateMaterialExpression(Material, UMaterialExpressionVectorParameter::StaticClass(), -640, -40));
	if (SampleColorParameter)
	{
		SampleColorParameter->ParameterName = TEXT("ChameleonSampleColor");
		SampleColorParameter->DefaultValue = SampleColor;
	}

	UMaterialExpressionConstant* Roughness = Cast<UMaterialExpressionConstant>(
		UMaterialEditingLibrary::CreateMaterialExpression(Material, UMaterialExpressionConstant::StaticClass(), -640, 140));
	if (Roughness)
	{
		Roughness->R = 0.78f;
	}

	if (Texture)
	{
		UMaterialExpressionTextureSample* TextureSample = Cast<UMaterialExpressionTextureSample>(
			UMaterialEditingLibrary::CreateMaterialExpression(Material, UMaterialExpressionTextureSample::StaticClass(), -640, -260));
		UMaterialExpressionMultiply* Multiply = Cast<UMaterialExpressionMultiply>(
			UMaterialEditingLibrary::CreateMaterialExpression(Material, UMaterialExpressionMultiply::StaticClass(), -300, -150));

		if (TextureSample)
		{
			TextureSample->Texture = Texture;
			TextureSample->AutoSetSampleType();
		}

		UMaterialEditingLibrary::ConnectMaterialExpressions(TextureSample, TEXT(""), Multiply, TEXT("A"));
		UMaterialEditingLibrary::ConnectMaterialExpressions(SampleColorParameter, TEXT(""), Multiply, TEXT("B"));
		UMaterialEditingLibrary::ConnectMaterialProperty(Multiply, TEXT(""), MP_BaseColor);
	}
	else
	{
		UMaterialEditingLibrary::ConnectMaterialProperty(SampleColorParameter, TEXT(""), MP_BaseColor);
	}

	UMaterialEditingLibrary::ConnectMaterialProperty(Roughness, TEXT(""), MP_Roughness);
	UMaterialEditingLibrary::RecompileMaterial(Material);
	Material->PostEditChange();
	Material->MarkPackageDirty();
	SavePackageForObject(Material);
	return Material;
}

UInputAction* CreateInputActionAsset(const FString& AssetName, EInputActionValueType ValueType)
{
	UInputAction* Action = FindOrCreateAsset<UInputAction>(InputPath / AssetName);
	Action->ValueType = ValueType;
	Action->MarkPackageDirty();
	SavePackageForObject(Action);
	return Action;
}

void AddSwizzle(UInputMappingContext* Context, FEnhancedActionKeyMapping& Mapping)
{
	UInputModifierSwizzleAxis* Modifier = NewObject<UInputModifierSwizzleAxis>(Context, NAME_None, RF_Transactional);
	Modifier->Order = EInputAxisSwizzle::YXZ;
	Mapping.Modifiers.Add(Modifier);
}

void AddNegate(UInputMappingContext* Context, FEnhancedActionKeyMapping& Mapping, bool bX, bool bY, bool bZ)
{
	UInputModifierNegate* Modifier = NewObject<UInputModifierNegate>(Context, NAME_None, RF_Transactional);
	Modifier->bX = bX;
	Modifier->bY = bY;
	Modifier->bZ = bZ;
	Mapping.Modifiers.Add(Modifier);
}

FEnhancedActionKeyMapping& MapKey(UInputMappingContext* Context, UInputAction* Action, FKey Key)
{
	return Context->MapKey(Action, Key);
}

UChameleonPainterInputConfig* CreateInputAssets()
{
	UInputAction* MoveAction = CreateInputActionAsset(TEXT("IA_Move"), EInputActionValueType::Axis2D);
	MoveAction->AccumulationBehavior = EInputActionAccumulationBehavior::Cumulative;
	MoveAction->MarkPackageDirty();
	SavePackageForObject(MoveAction);
	UInputAction* LookAction = CreateInputActionAsset(TEXT("IA_Look"), EInputActionValueType::Axis2D);
	LookAction->AccumulationBehavior = EInputActionAccumulationBehavior::Cumulative;
	LookAction->MarkPackageDirty();
	SavePackageForObject(LookAction);
	UInputAction* JumpAction = CreateInputActionAsset(TEXT("IA_Jump"), EInputActionValueType::Boolean);
	UInputAction* PaintAction = CreateInputActionAsset(TEXT("IA_Paint"), EInputActionValueType::Boolean);
	UInputAction* SampleColorAction = CreateInputActionAsset(TEXT("IA_SampleColor"), EInputActionValueType::Boolean);
	UInputAction* ToggleColorPickerAction = CreateInputActionAsset(TEXT("IA_ToggleColorPicker"), EInputActionValueType::Boolean);

	UInputMappingContext* MappingContext = FindOrCreateAsset<UInputMappingContext>(InputPath / TEXT("IMC_ChameleonPlayer"));
	MappingContext->UnmapAll();

	FEnhancedActionKeyMapping& MoveW = MapKey(MappingContext, MoveAction, EKeys::W);
	AddSwizzle(MappingContext, MoveW);
	FEnhancedActionKeyMapping& MoveS = MapKey(MappingContext, MoveAction, EKeys::S);
	AddSwizzle(MappingContext, MoveS);
	AddNegate(MappingContext, MoveS, false, true, false);
	FEnhancedActionKeyMapping& MoveD = MapKey(MappingContext, MoveAction, EKeys::D);
	(void)MoveD;
	FEnhancedActionKeyMapping& MoveA = MapKey(MappingContext, MoveAction, EKeys::A);
	AddNegate(MappingContext, MoveA, true, false, false);

	MapKey(MappingContext, LookAction, EKeys::MouseX);
	FEnhancedActionKeyMapping& LookY = MapKey(MappingContext, LookAction, EKeys::MouseY);
	AddSwizzle(MappingContext, LookY);

	MapKey(MappingContext, JumpAction, EKeys::SpaceBar);
	MapKey(MappingContext, PaintAction, EKeys::LeftMouseButton);
	MapKey(MappingContext, SampleColorAction, EKeys::RightMouseButton);
	MapKey(MappingContext, SampleColorAction, EKeys::E);
	MapKey(MappingContext, ToggleColorPickerAction, EKeys::Tab);

	MappingContext->MarkPackageDirty();
	SavePackageForObject(MappingContext);

	UChameleonPainterInputConfig* InputConfig = FindOrCreateAsset<UChameleonPainterInputConfig>(InputPath / TEXT("DA_ChameleonPainterInputConfig"));
	InputConfig->DefaultMappingContext = MappingContext;
	InputConfig->MoveAction = MoveAction;
	InputConfig->LookAction = LookAction;
	InputConfig->JumpAction = JumpAction;
	InputConfig->PaintAction = PaintAction;
	InputConfig->SampleColorAction = SampleColorAction;
	InputConfig->ToggleColorPickerAction = ToggleColorPickerAction;
	InputConfig->MarkPackageDirty();
	SavePackageForObject(InputConfig);

	return InputConfig;
}

UBlueprint* FindOrCreateBlueprint(const FString& PackageName, UClass* ParentClass)
{
	if (UBlueprint* ExistingBlueprint = LoadAssetByPackageName<UBlueprint>(PackageName))
	{
		if (ExistingBlueprint->ParentClass != ParentClass)
		{
			ExistingBlueprint->ParentClass = ParentClass;
			FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(ExistingBlueprint);
		}
		FKismetEditorUtilities::CompileBlueprint(ExistingBlueprint);
		return ExistingBlueprint;
	}

	UPackage* Package = CreatePackage(*PackageName);
	Package->FullyLoad();
	const FName AssetName(*FPackageName::GetLongPackageAssetName(PackageName));
	UBlueprint* Blueprint = FKismetEditorUtilities::CreateBlueprint(
		ParentClass,
		Package,
		AssetName,
		BPTYPE_Normal,
		UBlueprint::StaticClass(),
		UBlueprintGeneratedClass::StaticClass());
	FAssetRegistryModule::AssetCreated(Blueprint);
	FKismetEditorUtilities::CompileBlueprint(Blueprint);
	Blueprint->MarkPackageDirty();
	return Blueprint;
}

void ConfigureBlueprints(UChameleonPainterInputConfig* InputConfig, UMaterialInterface* HiderMaterial, UClass*& OutGameModeClass)
{
	UBlueprint* CharacterBlueprint = FindOrCreateBlueprint(BlueprintPath / TEXT("BP_ChameleonHiderCharacter"), AChameleonHiderCharacter::StaticClass());
	if (AChameleonHiderCharacter* CharacterCDO = CharacterBlueprint && CharacterBlueprint->GeneratedClass
		? Cast<AChameleonHiderCharacter>(CharacterBlueprint->GeneratedClass->GetDefaultObject())
		: nullptr)
	{
		if (CharacterCDO->BodyComponent)
		{
			CharacterCDO->BodyComponent->BodyMaterial = HiderMaterial;
		}
		CharacterCDO->CurrentBrushColor = FLinearColor(0.84f, 0.82f, 0.76f, 1.0f);
		CharacterCDO->MarkPackageDirty();
		FBlueprintEditorUtils::MarkBlueprintAsModified(CharacterBlueprint);
	}
	FKismetEditorUtilities::CompileBlueprint(CharacterBlueprint);

	UBlueprint* GameModeBlueprint = FindOrCreateBlueprint(BlueprintPath / TEXT("BP_ChameleonPainterGameMode"), AChameleonPainterGameMode::StaticClass());
	if (AGameModeBase* GameModeCDO = GameModeBlueprint && GameModeBlueprint->GeneratedClass
		? Cast<AGameModeBase>(GameModeBlueprint->GeneratedClass->GetDefaultObject())
		: nullptr)
	{
		GameModeCDO->DefaultPawnClass = CharacterBlueprint->GeneratedClass;
		GameModeCDO->MarkPackageDirty();
		FBlueprintEditorUtils::MarkBlueprintAsModified(GameModeBlueprint);
	}
	FKismetEditorUtilities::CompileBlueprint(GameModeBlueprint);
	OutGameModeClass = GameModeBlueprint->GeneratedClass ? static_cast<UClass*>(GameModeBlueprint->GeneratedClass.Get()) : AChameleonPainterGameMode::StaticClass();

	UBlueprint* GameInstanceBlueprint = FindOrCreateBlueprint(BlueprintPath / TEXT("BP_ChameleonPainterGameInstance"), UChameleonPainterGameInstance::StaticClass());
	if (UChameleonPainterGameInstance* GameInstanceCDO = GameInstanceBlueprint && GameInstanceBlueprint->GeneratedClass
		? Cast<UChameleonPainterGameInstance>(GameInstanceBlueprint->GeneratedClass->GetDefaultObject())
		: nullptr)
	{
		GameInstanceCDO->ChameleonInputConfig = InputConfig;
		GameInstanceCDO->MarkPackageDirty();
		FBlueprintEditorUtils::MarkBlueprintAsModified(GameInstanceBlueprint);
	}
	FKismetEditorUtilities::CompileBlueprint(GameInstanceBlueprint);

	UGameMapsSettings* MapsSettings = UGameMapsSettings::GetGameMapsSettings();
	if (GameInstanceBlueprint && GameInstanceBlueprint->GeneratedClass)
	{
		MapsSettings->GameInstanceClass = FSoftClassPath(GameInstanceBlueprint->GeneratedClass->GetPathName());
	}
	if (OutGameModeClass)
	{
		UGameMapsSettings::SetGlobalDefaultGameMode(OutGameModeClass->GetPathName());
		GConfig->SetString(
			TEXT("/Script/EngineSettings.GameMapsSettings"),
			TEXT("GlobalDefaultGameMode"),
			*OutGameModeClass->GetPathName(),
			GEngineIni);
	}
	if (GameInstanceBlueprint && GameInstanceBlueprint->GeneratedClass)
	{
		GConfig->SetString(
			TEXT("/Script/EngineSettings.GameMapsSettings"),
			TEXT("GameInstanceClass"),
			*GameInstanceBlueprint->GeneratedClass->GetPathName(),
			GEngineIni);
	}
	GConfig->Flush(false, GEngineIni);
	MapsSettings->SaveConfig();

	SaveAssets({ CharacterBlueprint, GameModeBlueprint, GameInstanceBlueprint });
}

AStaticMeshActor* SpawnStaticMeshActor(UWorld* World, UStaticMesh* Mesh, UMaterialInterface* Material, const FVector& Location, const FRotator& Rotation, const FVector& Scale, const FString& Label)
{
	AStaticMeshActor* Actor = World->SpawnActor<AStaticMeshActor>(Location, Rotation);
	if (!Actor)
	{
		return nullptr;
	}

	Actor->SetActorLabel(Label);
	Actor->SetActorScale3D(Scale);

	if (UStaticMeshComponent* MeshComponent = Actor->GetStaticMeshComponent())
	{
		MeshComponent->SetStaticMesh(Mesh);
		MeshComponent->SetMaterial(0, Material);
		MeshComponent->SetMobility(EComponentMobility::Static);
		MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		MeshComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	}

	return Actor;
}

bool CreateTestLevel(
	UMaterialInterface* ConcreteMaterial,
	UMaterialInterface* WoodMaterial,
	UMaterialInterface* GreenMaterial,
	UMaterialInterface* FloorMaterial,
	UClass* GameModeClass)
{
	UWorld* World = UEditorLoadingAndSavingUtils::NewBlankMap(false);
	if (!World)
	{
		return false;
	}

	AWorldSettings* WorldSettings = World->GetWorldSettings();
	if (WorldSettings)
	{
		WorldSettings->DefaultGameMode = GameModeClass ? GameModeClass : AChameleonPainterGameMode::StaticClass();
	}

	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	UStaticMesh* Plane = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));

	SpawnStaticMeshActor(World, Plane, FloorMaterial, FVector::ZeroVector, FRotator::ZeroRotator, FVector(18.0f, 18.0f, 1.0f), TEXT("CPT_FloorTile_SamplingPlane"));
	SpawnStaticMeshActor(World, Cube, ConcreteMaterial, FVector(850.0f, 0.0f, 160.0f), FRotator::ZeroRotator, FVector(0.35f, 17.0f, 3.2f), TEXT("CPT_Concrete_BackWall"));
	SpawnStaticMeshActor(World, Cube, ConcreteMaterial, FVector(-850.0f, 0.0f, 160.0f), FRotator::ZeroRotator, FVector(0.35f, 17.0f, 3.2f), TEXT("CPT_Concrete_FrontWall"));
	SpawnStaticMeshActor(World, Cube, GreenMaterial, FVector(100.0f, -620.0f, 120.0f), FRotator(0.0f, 14.0f, 0.0f), FVector(5.0f, 0.35f, 2.4f), TEXT("CPT_GreenPanel_Left"));
	SpawnStaticMeshActor(World, Cube, GreenMaterial, FVector(160.0f, 620.0f, 120.0f), FRotator(0.0f, -10.0f, 0.0f), FVector(5.5f, 0.35f, 2.4f), TEXT("CPT_GreenPanel_Right"));
	SpawnStaticMeshActor(World, Cube, WoodMaterial, FVector(-290.0f, -210.0f, 75.0f), FRotator(0.0f, 22.0f, 0.0f), FVector(1.6f, 1.6f, 1.5f), TEXT("CPT_WoodCrate_A"));
	SpawnStaticMeshActor(World, Cube, WoodMaterial, FVector(360.0f, 180.0f, 55.0f), FRotator(0.0f, -18.0f, 0.0f), FVector(1.2f, 1.2f, 1.1f), TEXT("CPT_WoodCrate_B"));
	SpawnStaticMeshActor(World, Cube, ConcreteMaterial, FVector(0.0f, 0.0f, 45.0f), FRotator(0.0f, 45.0f, 0.0f), FVector(2.4f, 0.35f, 0.9f), TEXT("CPT_LowConcreteDivider"));

	APlayerStart* PlayerStart = World->SpawnActor<APlayerStart>(FVector(-520.0f, 0.0f, 100.0f), FRotator(0.0f, 0.0f, 0.0f));
	if (PlayerStart)
	{
		PlayerStart->SetActorLabel(TEXT("CPT_PlayerStart"));
	}

	ADirectionalLight* Sun = World->SpawnActor<ADirectionalLight>(FVector(0.0f, 0.0f, 700.0f), FRotator(-42.0f, -36.0f, 0.0f));
	if (Sun && Sun->GetLightComponent())
	{
		Sun->SetActorLabel(TEXT("CPT_DirectionalLight"));
		Sun->GetLightComponent()->SetIntensity(4.0f);
	}

	ASkyLight* SkyLight = World->SpawnActor<ASkyLight>(FVector::ZeroVector, FRotator::ZeroRotator);
	if (SkyLight && SkyLight->GetLightComponent())
	{
		SkyLight->SetActorLabel(TEXT("CPT_SkyLight"));
		SkyLight->GetLightComponent()->SetIntensity(0.75f);
	}

	const FTransform PreviewBodyTransform(FRotator(0.0f, -40.0f, 0.0f), FVector(480.0f, -360.0f, 10.0f));
	AChameleonHiderBodyActor* PreviewBody = World->SpawnActorDeferred<AChameleonHiderBodyActor>(AChameleonHiderBodyActor::StaticClass(), PreviewBodyTransform);
	if (PreviewBody)
	{
		PreviewBody->SetActorLabel(TEXT("CPT_PreviewPaintableHiderBody"));
		if (PreviewBody->BodyComponent)
		{
			PreviewBody->BodyComponent->BodyMaterial = LoadAssetByPackageName<UMaterial>(MaterialPath / TEXT("M_CPT_HiderPaint"));
			PreviewBody->BodyComponent->bBuildQueryCollision = true;
		}
		if (PreviewBody->PaintComponent)
		{
			PreviewBody->PaintComponent->bApplyOnRegister = false;
		}
		PreviewBody->FinishSpawning(PreviewBodyTransform);
	}

	const FString LevelPackage = MapPath / TEXT("L_ChameleonPainter_Test");
	if (!UEditorLoadingAndSavingUtils::SaveMap(World, LevelPackage))
	{
		return false;
	}

	UGameMapsSettings* MapsSettings = UGameMapsSettings::GetGameMapsSettings();
	const FString LevelObjectPath = ToObjectPath(LevelPackage);
	MapsSettings->EditorStartupMap = FSoftObjectPath(LevelObjectPath);
	UGameMapsSettings::SetGameDefaultMap(LevelPackage);
	GConfig->SetString(
		TEXT("/Script/EngineSettings.GameMapsSettings"),
		TEXT("EditorStartupMap"),
		*LevelObjectPath,
		GEngineIni);
	GConfig->SetString(
		TEXT("/Script/EngineSettings.GameMapsSettings"),
		TEXT("GameDefaultMap"),
		*LevelObjectPath,
		GEngineIni);
	GConfig->Flush(false, GEngineIni);
	MapsSettings->SaveConfig();

	return true;
}
}

UChameleonPainterBuildTestContentCommandlet::UChameleonPainterBuildTestContentCommandlet()
{
	IsClient = false;
	IsEditor = true;
	IsServer = false;
	LogToConsole = true;
}

int32 UChameleonPainterBuildTestContentCommandlet::Main(const FString& Params)
{
	UE_LOG(LogChameleonPainterContent, Display, TEXT("Building Chameleon Painter test content. Params: %s"), *Params);

	const TMap<FName, UTexture2D*> Textures = ImportTextures();
	UMaterial* HiderMaterial = CreateHiderPaintMaterial();
	UMaterial* ConcreteMaterial = CreateSurfaceMaterial(
		TEXT("M_CPT_Concrete"),
		Textures.FindRef(TEXT("T_CPT_Concrete")),
		FLinearColor(0.60f, 0.61f, 0.57f, 1.0f));
	UMaterial* WoodMaterial = CreateSurfaceMaterial(
		TEXT("M_CPT_Wood"),
		Textures.FindRef(TEXT("T_CPT_Wood")),
		FLinearColor(0.53f, 0.33f, 0.18f, 1.0f));
	UMaterial* GreenMaterial = CreateSurfaceMaterial(
		TEXT("M_CPT_GreenPanel"),
		Textures.FindRef(TEXT("T_CPT_GreenPanel")),
		FLinearColor(0.18f, 0.39f, 0.20f, 1.0f));
	UMaterial* FloorMaterial = CreateSurfaceMaterial(
		TEXT("M_CPT_FloorTile"),
		Textures.FindRef(TEXT("T_CPT_FloorTile")),
		FLinearColor(0.30f, 0.33f, 0.34f, 1.0f));

	UChameleonPainterInputConfig* InputConfig = CreateInputAssets();
	UClass* GameModeClass = nullptr;
	ConfigureBlueprints(InputConfig, HiderMaterial, GameModeClass);

	const bool bCreatedLevel = CreateTestLevel(ConcreteMaterial, WoodMaterial, GreenMaterial, FloorMaterial, GameModeClass);
	UEditorLoadingAndSavingUtils::SaveDirtyPackages(false, true);

	if (!bCreatedLevel)
	{
		UE_LOG(LogChameleonPainterContent, Error, TEXT("Failed to create Chameleon Painter test level."));
		return 1;
	}

	UE_LOG(LogChameleonPainterContent, Display, TEXT("Chameleon Painter test content is ready at %s"), *RootPath);
	return 0;
}

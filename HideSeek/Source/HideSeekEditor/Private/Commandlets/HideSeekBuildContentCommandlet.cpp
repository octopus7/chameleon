#include "Commandlets/HideSeekBuildContentCommandlet.h"

#include "AI/HideSeekAIController.h"
#include "Actors/ChameleonPaintSprayActor.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Character/HideSeekParticipantCharacter.h"
#include "Components/ChameleonMetaballBodyComponent.h"
#include "Components/ChameleonPaintComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Data/HideSeekRoundSettings.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/DirectionalLight.h"
#include "Engine/LevelStreamingDynamic.h"
#include "Engine/SkyLight.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/TargetPoint.h"
#include "Engine/World.h"
#include "Engine/WorldSettings.h"
#include "FileHelpers.h"
#include "Game/HideSeekGameMode.h"
#include "Game/HideSeekGameState.h"
#include "Game/HideSeekPlayerController.h"
#include "Game/HideSeekPlayerState.h"
#include "GameFramework/PlayerStart.h"
#include "GameMapsSettings.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Materials/MaterialInterface.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/PackageName.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "NiagaraSystem.h"
#include "UI/ChameleonBrushCursorWidget.h"
#include "UI/ChameleonColorPickerWidget.h"
#include "UI/HideSeekTransitionWidget.h"
#include "UObject/SavePackage.h"

DEFINE_LOG_CATEGORY_STATIC(LogHideSeekBuildContent, Log, All);

namespace
{
const FString RootPath = TEXT("/Game/HideSeek");
const FString DataPath = RootPath / TEXT("Data");
const FString BlueprintPath = RootPath / TEXT("Blueprints");
const FString MapPath = RootPath / TEXT("Maps");

FString ToObjectPath(const FString& PackageName)
{
	return PackageName + TEXT(".") + FPackageName::GetLongPackageAssetName(PackageName);
}

template <typename T>
T* LoadAssetByPackageName(const FString& PackageName)
{
	const FString PackageFilename = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
	if (!FPaths::FileExists(PackageFilename))
	{
		return nullptr;
	}

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

UBlueprint* FindOrCreateBlueprint(const FString& PackageName, UClass* ParentClass)
{
	if (UBlueprint* ExistingBlueprint = LoadAssetByPackageName<UBlueprint>(PackageName))
	{
		if (ExistingBlueprint->ParentClass != ParentClass)
		{
			FBlueprintEditorUtils::ChangeBlueprintParentClass(ExistingBlueprint, ParentClass);
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
	if (Blueprint)
	{
		FAssetRegistryModule::AssetCreated(Blueprint);
		FKismetEditorUtilities::CompileBlueprint(Blueprint);
		Blueprint->MarkPackageDirty();
	}
	return Blueprint;
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

void ConfigureParticipantBlueprint(UBlueprint* ParticipantBlueprint)
{
	if (!ParticipantBlueprint || !ParticipantBlueprint->GeneratedClass)
	{
		return;
	}

	AHideSeekParticipantCharacter* CharacterCDO = Cast<AHideSeekParticipantCharacter>(ParticipantBlueprint->GeneratedClass->GetDefaultObject());
	if (!CharacterCDO)
	{
		return;
	}

	UMaterialInterface* HiderMaterial = LoadAssetByPackageName<UMaterialInterface>(TEXT("/Game/ChameleonPainterTest/Materials/M_CPT_HiderPaint"));
	UMaterialInterface* PaintSprayMaterial = LoadAssetByPackageName<UMaterialInterface>(TEXT("/Game/ChameleonPainterTest/Materials/M_CPT_PaintSpray"));
	UClass* ColorPickerClass = StaticLoadClass(
		UChameleonColorPickerWidget::StaticClass(),
		nullptr,
		TEXT("/Game/ChameleonPainterTest/UI/WBP_ChameleonColorPicker.WBP_ChameleonColorPicker_C"));
	UClass* BrushCursorClass = StaticLoadClass(
		UChameleonBrushCursorWidget::StaticClass(),
		nullptr,
		TEXT("/Game/ChameleonPainterTest/UI/WBP_ChameleonBrushCursor.WBP_ChameleonBrushCursor_C"));

	if (CharacterCDO->BodyComponent && HiderMaterial)
	{
		CharacterCDO->BodyComponent->BodyMaterial = HiderMaterial;
		CharacterCDO->BodyComponent->bBuildQueryCollision = true;
		CharacterCDO->BodyComponent->CamouflageBaseColor = FLinearColor(0.36f, 0.48f, 0.31f, 1.0f);
		CharacterCDO->BodyComponent->CamouflageBaseRoughness = 0.84f;
		CharacterCDO->BodyComponent->CamouflageBaseMetallic = 0.0f;
	}
	if (CharacterCDO->PaintComponent)
	{
		CharacterCDO->PaintComponent->PaintColor = FLinearColor(1.0f, 0.02f, 0.02f, 1.0f);
		CharacterCDO->PaintComponent->bApplyOnRegister = false;
		CharacterCDO->PaintComponent->bApplyOnBeginPlay = false;
		CharacterCDO->PaintComponent->bApplyOnTargetComponentChange = false;
	}
	if (ColorPickerClass)
	{
		CharacterCDO->ColorPickerWidgetClass = TSubclassOf<UChameleonColorPickerWidget>(ColorPickerClass);
	}
	if (BrushCursorClass)
	{
		CharacterCDO->BrushCursorWidgetClass = TSubclassOf<UChameleonBrushCursorWidget>(BrushCursorClass);
	}
	CharacterCDO->PaintSprayEffectClass = AChameleonPaintSprayActor::StaticClass();
	CharacterCDO->PaintSprayMaterial = PaintSprayMaterial;
	CharacterCDO->MarkPackageDirty();
	FBlueprintEditorUtils::MarkBlueprintAsModified(ParticipantBlueprint);
	FKismetEditorUtilities::CompileBlueprint(ParticipantBlueprint);
}

UHideSeekRoundSettings* ConfigureRoundSettings(UBlueprint* ParticipantBlueprint)
{
	UHideSeekRoundSettings* Settings = FindOrCreateAsset<UHideSeekRoundSettings>(DataPath / TEXT("DA_HideSeekRoundSettings"));
	if (!Settings)
	{
		return nullptr;
	}

	Settings->PlayRoomLevel = TSoftObjectPtr<UWorld>(FSoftObjectPath(TEXT("/Game/HideSeek/Maps/L_HideSeek_PlayRoom.L_HideSeek_PlayRoom")));
	Settings->PlayRoomStreamLevelName = TEXT("L_HideSeek_PlayRoom");
	Settings->ParticipantPawnClass = ParticipantBlueprint && ParticipantBlueprint->GeneratedClass
		? TSubclassOf<APawn>(ParticipantBlueprint->GeneratedClass.Get())
		: AHideSeekParticipantCharacter::StaticClass();
	Settings->TargetParticipantCount = 4;
	Settings->bAutoSpawnAI = true;
	Settings->bAutoStartRound = true;
	Settings->bLoopRounds = true;
	Settings->SeekerSelectionDelaySeconds = 1.5f;
	Settings->SeekerLobbyWaitSeconds = 5.0f;
	Settings->LoadingHoldSeconds = 1.5f;
	Settings->HideTimeSeconds = 35.0f;
	Settings->SeekTimeSeconds = 90.0f;
	Settings->RoundResetDelaySeconds = 5.0f;
	Settings->CaptureRadiusCm = 160.0f;
	Settings->TransitionWidgetClass = UHideSeekTransitionWidget::StaticClass();
	Settings->TransitionNiagaraSystem = TSoftObjectPtr<UNiagaraSystem>(FSoftObjectPath(TEXT("/Niagara/DefaultAssets/Templates/Systems/RadialBurst.RadialBurst")));
	Settings->MarkPackageDirty();
	SavePackageForObject(Settings);
	return Settings;
}

UBlueprint* ConfigureGameModeBlueprint(UBlueprint* ParticipantBlueprint, UHideSeekRoundSettings* Settings)
{
	UBlueprint* GameModeBlueprint = FindOrCreateBlueprint(BlueprintPath / TEXT("BP_HideSeekGameMode"), AHideSeekGameMode::StaticClass());
	if (!GameModeBlueprint || !GameModeBlueprint->GeneratedClass)
	{
		return GameModeBlueprint;
	}

	AHideSeekGameMode* GameModeCDO = Cast<AHideSeekGameMode>(GameModeBlueprint->GeneratedClass->GetDefaultObject());
	if (GameModeCDO)
	{
		GameModeCDO->DefaultPawnClass = ParticipantBlueprint && ParticipantBlueprint->GeneratedClass
			? ParticipantBlueprint->GeneratedClass.Get()
			: AHideSeekParticipantCharacter::StaticClass();
		GameModeCDO->GameStateClass = AHideSeekGameState::StaticClass();
		GameModeCDO->PlayerControllerClass = AHideSeekPlayerController::StaticClass();
		GameModeCDO->PlayerStateClass = AHideSeekPlayerState::StaticClass();
		GameModeCDO->HideSeekAIControllerClass = AHideSeekAIController::StaticClass();
		GameModeCDO->RoundSettingsAsset = Settings;
		GameModeCDO->MarkPackageDirty();
		FBlueprintEditorUtils::MarkBlueprintAsModified(GameModeBlueprint);
	}

	FKismetEditorUtilities::CompileBlueprint(GameModeBlueprint);
	SavePackageForObject(GameModeBlueprint);
	return GameModeBlueprint;
}

AStaticMeshActor* SpawnStaticMeshActor(
	UWorld* World,
	UStaticMesh* Mesh,
	UMaterialInterface* Material,
	const FVector& Location,
	const FRotator& Rotation,
	const FVector& Scale,
	const FString& Label,
	FName Tag = NAME_None)
{
	AStaticMeshActor* Actor = World ? World->SpawnActor<AStaticMeshActor>(Location, Rotation) : nullptr;
	if (!Actor)
	{
		return nullptr;
	}

	Actor->SetActorLabel(Label);
	Actor->SetActorScale3D(Scale);
	if (!Tag.IsNone())
	{
		Actor->Tags.AddUnique(Tag);
	}

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

void SpawnPlayerStart(UWorld* World, const FVector& Location, const FRotator& Rotation, FName Tag, const FString& Label)
{
	APlayerStart* PlayerStart = World ? World->SpawnActor<APlayerStart>(Location, Rotation) : nullptr;
	if (PlayerStart)
	{
		PlayerStart->SetActorLabel(Label);
		PlayerStart->Tags.AddUnique(Tag);
	}
}

void SpawnTargetPoint(UWorld* World, const FVector& Location, const FRotator& Rotation, FName Tag, const FString& Label)
{
	ATargetPoint* TargetPoint = World ? World->SpawnActor<ATargetPoint>(Location, Rotation) : nullptr;
	if (TargetPoint)
	{
		TargetPoint->SetActorLabel(Label);
		TargetPoint->Tags.AddUnique(Tag);
	}
}

void SpawnLighting(UWorld* World)
{
	ADirectionalLight* Sun = World ? World->SpawnActor<ADirectionalLight>(FVector(0.0f, 0.0f, 700.0f), FRotator(-42.0f, -32.0f, 0.0f)) : nullptr;
	if (Sun && Sun->GetLightComponent())
	{
		Sun->SetActorLabel(TEXT("HS_DirectionalLight"));
		Sun->GetLightComponent()->SetIntensity(3.5f);
		Sun->GetComponent()->SetAtmosphereSunLight(true);
		Sun->GetComponent()->SetAtmosphereSunLightIndex(0);
	}

	ASkyLight* SkyLight = World ? World->SpawnActor<ASkyLight>(FVector::ZeroVector, FRotator::ZeroRotator) : nullptr;
	if (SkyLight && SkyLight->GetLightComponent())
	{
		SkyLight->SetActorLabel(TEXT("HS_SkyLight"));
		SkyLight->GetLightComponent()->SetIntensity(0.8f);
	}

	ASkyAtmosphere* SkyAtmosphere = World ? World->SpawnActor<ASkyAtmosphere>(FVector::ZeroVector, FRotator::ZeroRotator) : nullptr;
	if (SkyAtmosphere)
	{
		SkyAtmosphere->SetActorLabel(TEXT("HS_SkyAtmosphere"));
	}
}

bool CreatePlayRoomLevel(UClass* GameModeClass)
{
	UWorld* World = UEditorLoadingAndSavingUtils::NewBlankMap(false);
	if (!World)
	{
		return false;
	}

	if (AWorldSettings* WorldSettings = World->GetWorldSettings())
	{
		WorldSettings->DefaultGameMode = GameModeClass ? GameModeClass : AHideSeekGameMode::StaticClass();
	}

	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	UStaticMesh* Plane = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
	UMaterialInterface* FloorMaterial = LoadAssetByPackageName<UMaterialInterface>(TEXT("/Game/ChameleonPainterTest/Materials/M_CPT_FloorTile"));
	UMaterialInterface* WallMaterial = LoadAssetByPackageName<UMaterialInterface>(TEXT("/Game/ChameleonPainterTest/Materials/M_CPT_Concrete"));
	UMaterialInterface* WoodMaterial = LoadAssetByPackageName<UMaterialInterface>(TEXT("/Game/ChameleonPainterTest/Materials/M_CPT_Wood"));
	UMaterialInterface* GreenMaterial = LoadAssetByPackageName<UMaterialInterface>(TEXT("/Game/ChameleonPainterTest/Materials/M_CPT_GreenPanel"));

	const FVector RoomOrigin(6000.0f, 0.0f, 0.0f);
	SpawnStaticMeshActor(World, Plane, FloorMaterial, RoomOrigin, FRotator::ZeroRotator, FVector(24.0f, 24.0f, 1.0f), TEXT("HS_PlayRoom_Floor"));
	SpawnStaticMeshActor(World, Cube, WallMaterial, RoomOrigin + FVector(0.0f, -1200.0f, 180.0f), FRotator::ZeroRotator, FVector(24.0f, 0.35f, 3.6f), TEXT("HS_PlayRoom_NorthWall"));
	SpawnStaticMeshActor(World, Cube, WallMaterial, RoomOrigin + FVector(0.0f, 1200.0f, 180.0f), FRotator::ZeroRotator, FVector(24.0f, 0.35f, 3.6f), TEXT("HS_PlayRoom_SouthWall"));
	SpawnStaticMeshActor(World, Cube, WallMaterial, RoomOrigin + FVector(-1200.0f, 0.0f, 180.0f), FRotator::ZeroRotator, FVector(0.35f, 24.0f, 3.6f), TEXT("HS_PlayRoom_WestWall"));
	SpawnStaticMeshActor(World, Cube, WallMaterial, RoomOrigin + FVector(1200.0f, 0.0f, 180.0f), FRotator::ZeroRotator, FVector(0.35f, 24.0f, 3.6f), TEXT("HS_PlayRoom_EastWall"));

	SpawnStaticMeshActor(World, Cube, WoodMaterial, RoomOrigin + FVector(-420.0f, -360.0f, 80.0f), FRotator(0.0f, 22.0f, 0.0f), FVector(2.2f, 2.0f, 1.6f), TEXT("HS_PlayRoom_CrateA"));
	SpawnStaticMeshActor(World, Cube, WoodMaterial, RoomOrigin + FVector(420.0f, 260.0f, 60.0f), FRotator(0.0f, -18.0f, 0.0f), FVector(1.8f, 1.7f, 1.2f), TEXT("HS_PlayRoom_CrateB"));
	SpawnStaticMeshActor(World, Cube, GreenMaterial, RoomOrigin + FVector(0.0f, -280.0f, 120.0f), FRotator(0.0f, 18.0f, 0.0f), FVector(4.5f, 0.35f, 2.4f), TEXT("HS_PlayRoom_ScreenWallA"));
	SpawnStaticMeshActor(World, Cube, GreenMaterial, RoomOrigin + FVector(-140.0f, 400.0f, 120.0f), FRotator(0.0f, -15.0f, 0.0f), FVector(5.0f, 0.35f, 2.4f), TEXT("HS_PlayRoom_ScreenWallB"));
	SpawnStaticMeshActor(World, Cube, WallMaterial, RoomOrigin + FVector(360.0f, -520.0f, 45.0f), FRotator(0.0f, 90.0f, 0.0f), FVector(2.8f, 0.35f, 0.9f), TEXT("HS_PlayRoom_LowDivider"));

	const TArray<FVector> HiderSpawnLocations = {
		RoomOrigin + FVector(-760.0f, -720.0f, 100.0f),
		RoomOrigin + FVector(740.0f, -680.0f, 100.0f),
		RoomOrigin + FVector(-720.0f, 650.0f, 100.0f),
		RoomOrigin + FVector(680.0f, 690.0f, 100.0f),
		RoomOrigin + FVector(80.0f, 760.0f, 100.0f)
	};
	for (int32 Index = 0; Index < HiderSpawnLocations.Num(); ++Index)
	{
		SpawnPlayerStart(
			World,
			HiderSpawnLocations[Index],
			FRotator(0.0f, 180.0f + Index * 31.0f, 0.0f),
			TEXT("PlayRoomHiderSpawn"),
			FString::Printf(TEXT("HS_PlayRoom_HiderSpawn_%d"), Index + 1));
	}

	SpawnPlayerStart(World, RoomOrigin + FVector(-980.0f, 0.0f, 100.0f), FRotator(0.0f, 0.0f, 0.0f), TEXT("PlayRoomSeekerSpawn"), TEXT("HS_PlayRoom_SeekerSpawn_A"));
	SpawnPlayerStart(World, RoomOrigin + FVector(980.0f, 0.0f, 100.0f), FRotator(0.0f, 180.0f, 0.0f), TEXT("PlayRoomSeekerSpawn"), TEXT("HS_PlayRoom_SeekerSpawn_B"));

	ANavMeshBoundsVolume* NavBounds = World->SpawnActor<ANavMeshBoundsVolume>(RoomOrigin + FVector(0.0f, 0.0f, 120.0f), FRotator::ZeroRotator);
	if (NavBounds)
	{
		NavBounds->SetActorLabel(TEXT("HS_PlayRoom_NavMeshBounds"));
		NavBounds->SetActorScale3D(FVector(25.0f, 25.0f, 4.0f));
	}

	return UEditorLoadingAndSavingUtils::SaveMap(World, MapPath / TEXT("L_HideSeek_PlayRoom"));
}

bool CreateLobbyLevel(UClass* GameModeClass)
{
	UWorld* World = UEditorLoadingAndSavingUtils::NewBlankMap(false);
	if (!World)
	{
		return false;
	}

	if (AWorldSettings* WorldSettings = World->GetWorldSettings())
	{
		WorldSettings->DefaultGameMode = GameModeClass ? GameModeClass : AHideSeekGameMode::StaticClass();
	}

	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	UStaticMesh* Plane = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
	UStaticMesh* Cylinder = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	UMaterialInterface* FloorMaterial = LoadAssetByPackageName<UMaterialInterface>(TEXT("/Game/ChameleonPainterTest/Materials/M_CPT_FloorTile"));
	UMaterialInterface* WallMaterial = LoadAssetByPackageName<UMaterialInterface>(TEXT("/Game/ChameleonPainterTest/Materials/M_CPT_Concrete"));
	UMaterialInterface* GreenMaterial = LoadAssetByPackageName<UMaterialInterface>(TEXT("/Game/ChameleonPainterTest/Materials/M_CPT_GreenPanel"));

	SpawnLighting(World);
	SpawnStaticMeshActor(World, Plane, FloorMaterial, FVector::ZeroVector, FRotator::ZeroRotator, FVector(16.0f, 16.0f, 1.0f), TEXT("HS_Lobby_Floor"));
	SpawnStaticMeshActor(World, Cube, WallMaterial, FVector(0.0f, -800.0f, 160.0f), FRotator::ZeroRotator, FVector(16.0f, 0.35f, 3.2f), TEXT("HS_Lobby_NorthWall"));
	SpawnStaticMeshActor(World, Cube, WallMaterial, FVector(0.0f, 800.0f, 160.0f), FRotator::ZeroRotator, FVector(16.0f, 0.35f, 3.2f), TEXT("HS_Lobby_SouthWall"));
	SpawnStaticMeshActor(World, Cube, WallMaterial, FVector(-800.0f, 0.0f, 160.0f), FRotator::ZeroRotator, FVector(0.35f, 16.0f, 3.2f), TEXT("HS_Lobby_WestWall"));
	SpawnStaticMeshActor(World, Cube, WallMaterial, FVector(800.0f, 0.0f, 160.0f), FRotator::ZeroRotator, FVector(0.35f, 16.0f, 3.2f), TEXT("HS_Lobby_EastWall"));
	SpawnStaticMeshActor(World, Cylinder, GreenMaterial, FVector(0.0f, 0.0f, 18.0f), FRotator::ZeroRotator, FVector(2.1f, 2.1f, 0.18f), TEXT("HS_Lobby_SeekerPad"), TEXT("LobbyCenter"));
	SpawnTargetPoint(World, FVector(0.0f, 0.0f, 100.0f), FRotator::ZeroRotator, TEXT("LobbyCenter"), TEXT("HS_Lobby_CenterMarker"));

	const TArray<FVector> LobbySpawns = {
		FVector(-520.0f, -420.0f, 100.0f),
		FVector(520.0f, -420.0f, 100.0f),
		FVector(-520.0f, 420.0f, 100.0f),
		FVector(520.0f, 420.0f, 100.0f),
		FVector(0.0f, 590.0f, 100.0f)
	};
	for (int32 Index = 0; Index < LobbySpawns.Num(); ++Index)
	{
		SpawnPlayerStart(
			World,
			LobbySpawns[Index],
			FRotator(0.0f, -90.0f + Index * 35.0f, 0.0f),
			TEXT("LobbySpawn"),
			FString::Printf(TEXT("HS_Lobby_Spawn_%d"), Index + 1));
	}

	ULevelStreamingDynamic* PlayRoomStreamingLevel = NewObject<ULevelStreamingDynamic>(World, ULevelStreamingDynamic::StaticClass(), TEXT("L_HideSeek_PlayRoom_Streaming"), RF_Transactional);
	if (PlayRoomStreamingLevel)
	{
		const FString PlayRoomPackageName = MapPath / TEXT("L_HideSeek_PlayRoom");
		PlayRoomStreamingLevel->SetWorldAssetByPackageName(FName(*PlayRoomPackageName));
		PlayRoomStreamingLevel->PackageNameToLoad = FName(*PlayRoomPackageName);
		PlayRoomStreamingLevel->SetShouldBeLoaded(false);
		PlayRoomStreamingLevel->SetShouldBeVisible(false);
		PlayRoomStreamingLevel->bShouldBlockOnLoad = false;
		World->AddStreamingLevel(PlayRoomStreamingLevel);
	}

	const bool bSavedMap = UEditorLoadingAndSavingUtils::SaveMap(World, MapPath / TEXT("L_HideSeek_Lobby"));
	if (bSavedMap)
	{
		UGameMapsSettings* MapsSettings = UGameMapsSettings::GetGameMapsSettings();
		const FString LobbyObjectPath = ToObjectPath(MapPath / TEXT("L_HideSeek_Lobby"));
		MapsSettings->EditorStartupMap = FSoftObjectPath(LobbyObjectPath);
		UGameMapsSettings::SetGameDefaultMap(MapPath / TEXT("L_HideSeek_Lobby"));
		GConfig->SetString(TEXT("/Script/EngineSettings.GameMapsSettings"), TEXT("EditorStartupMap"), *LobbyObjectPath, GEngineIni);
		GConfig->SetString(TEXT("/Script/EngineSettings.GameMapsSettings"), TEXT("GameDefaultMap"), *(MapPath / TEXT("L_HideSeek_Lobby")), GEngineIni);
		if (GameModeClass)
		{
			UGameMapsSettings::SetGlobalDefaultGameMode(GameModeClass->GetPathName());
			GConfig->SetString(TEXT("/Script/EngineSettings.GameMapsSettings"), TEXT("GlobalDefaultGameMode"), *GameModeClass->GetPathName(), GEngineIni);
		}
		GConfig->Flush(false, GEngineIni);
		MapsSettings->SaveConfig();
	}

	return bSavedMap;
}
}

UHideSeekBuildContentCommandlet::UHideSeekBuildContentCommandlet()
{
	IsClient = false;
	IsEditor = true;
	IsServer = false;
	LogToConsole = true;
}

int32 UHideSeekBuildContentCommandlet::Main(const FString& Params)
{
	UE_LOG(LogHideSeekBuildContent, Display, TEXT("Building HideSeek content. Params: %s"), *Params);

	UBlueprint* ParticipantBlueprint = FindOrCreateBlueprint(BlueprintPath / TEXT("BP_HideSeekParticipantCharacter"), AHideSeekParticipantCharacter::StaticClass());
	ConfigureParticipantBlueprint(ParticipantBlueprint);
	UHideSeekRoundSettings* Settings = ConfigureRoundSettings(ParticipantBlueprint);
	UBlueprint* GameModeBlueprint = ConfigureGameModeBlueprint(ParticipantBlueprint, Settings);
	UClass* GameModeClass = GameModeBlueprint && GameModeBlueprint->GeneratedClass
		? static_cast<UClass*>(GameModeBlueprint->GeneratedClass.Get())
		: AHideSeekGameMode::StaticClass();

	SaveAssets({ ParticipantBlueprint, Settings, GameModeBlueprint });

	const bool bCreatedPlayRoom = CreatePlayRoomLevel(GameModeClass);
	const bool bCreatedLobby = bCreatedPlayRoom && CreateLobbyLevel(GameModeClass);
	UEditorLoadingAndSavingUtils::SaveDirtyPackages(false, true);

	if (!bCreatedPlayRoom || !bCreatedLobby)
	{
		UE_LOG(LogHideSeekBuildContent, Error, TEXT("Failed to create HideSeek levels."));
		return 1;
	}

	UE_LOG(LogHideSeekBuildContent, Display, TEXT("HideSeek lobby and play room content is ready under %s."), *RootPath);
	return 0;
}

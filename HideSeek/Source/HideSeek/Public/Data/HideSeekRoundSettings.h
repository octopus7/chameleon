#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "HideSeekRoundSettings.generated.h"

class APawn;
class UNiagaraSystem;
class UUserWidget;
class UWorld;

UCLASS(BlueprintType)
class HIDESEEK_API UHideSeekRoundSettings : public UDataAsset
{
	GENERATED_BODY()

public:
	UHideSeekRoundSettings();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HideSeek|Levels")
	TSoftObjectPtr<UWorld> PlayRoomLevel;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HideSeek|Levels")
	FName PlayRoomStreamLevelName = TEXT("L_HideSeek_PlayRoom");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HideSeek|Participants")
	TSubclassOf<APawn> ParticipantPawnClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HideSeek|Participants", meta = (ClampMin = "1"))
	int32 TargetParticipantCount = 4;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HideSeek|Participants")
	bool bAutoSpawnAI = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HideSeek|Flow")
	bool bAutoStartRound = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HideSeek|Flow")
	bool bLoopRounds = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HideSeek|Flow")
	bool bSeekerCanLookWhileWaiting = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HideSeek|Timing", meta = (ClampMin = "0.0"))
	float AutoStartDelaySeconds = 2.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HideSeek|Timing", meta = (ClampMin = "0.0"))
	float SeekerSelectionDelaySeconds = 1.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HideSeek|Timing", meta = (ClampMin = "0.0"))
	float SeekerLobbyWaitSeconds = 5.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HideSeek|Timing", meta = (ClampMin = "0.0"))
	float LoadingHoldSeconds = 1.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HideSeek|Timing", meta = (ClampMin = "1.0"))
	float HideTimeSeconds = 35.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HideSeek|Timing", meta = (ClampMin = "1.0"))
	float SeekTimeSeconds = 90.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HideSeek|Timing", meta = (ClampMin = "0.0"))
	float RoundResetDelaySeconds = 5.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HideSeek|Rules", meta = (ClampMin = "10.0"))
	float CaptureRadiusCm = 160.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HideSeek|Rules", meta = (ClampMin = "0.05"))
	float CaptureCheckIntervalSeconds = 0.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HideSeek|AI", meta = (ClampMin = "100.0"))
	float AIWanderRadiusCm = 900.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HideSeek|AI", meta = (ClampMin = "0.1"))
	float AIDecisionIntervalSeconds = 1.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HideSeek|AI", meta = (ClampMin = "10.0"))
	float AIMoveAcceptanceRadiusCm = 120.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HideSeek|Spawns")
	FName LobbySpawnTag = TEXT("LobbySpawn");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HideSeek|Spawns")
	FName LobbyCenterTag = TEXT("LobbyCenter");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HideSeek|Spawns")
	FName PlayRoomHiderSpawnTag = TEXT("PlayRoomHiderSpawn");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HideSeek|Spawns")
	FName PlayRoomSeekerSpawnTag = TEXT("PlayRoomSeekerSpawn");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HideSeek|Transition", meta = (ClampMin = "0.0"))
	float TransitionFadeInSeconds = 0.45f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HideSeek|Transition", meta = (ClampMin = "0.0"))
	float TransitionHoldSeconds = 0.65f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HideSeek|Transition", meta = (ClampMin = "0.0"))
	float TransitionFadeOutSeconds = 0.45f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HideSeek|Transition")
	TSoftObjectPtr<UNiagaraSystem> TransitionNiagaraSystem;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HideSeek|Transition")
	TSubclassOf<UUserWidget> TransitionWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HideSeek|Transition")
	FVector TransitionNiagaraLocalOffset = FVector(80.0f, 0.0f, 0.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HideSeek|Transition")
	FVector TransitionNiagaraScale = FVector(1.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HideSeek|Transition", meta = (ClampMin = "0.0"))
	float TransitionNiagaraLifetimeSeconds = 2.0f;
};

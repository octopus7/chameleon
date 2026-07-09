#pragma once

#include "CoreMinimal.h"
#include "Game/HideSeekTypes.h"
#include "GameFramework/GameModeBase.h"
#include "HideSeekGameMode.generated.h"

class AHideSeekAIController;
class AHideSeekGameState;
class AHideSeekPlayerController;
class AHideSeekPlayerState;
class UHideSeekRoleComponent;
class UHideSeekRoundSettings;

UCLASS(BlueprintType, Blueprintable)
class HIDESEEK_API AHideSeekGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AHideSeekGameMode();

	virtual void BeginPlay() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;

	UFUNCTION(BlueprintCallable, Category = "HideSeek")
	void StartHideSeekRound();

	UFUNCTION(BlueprintPure, Category = "HideSeek")
	const UHideSeekRoundSettings* GetRoundSettings() const { return RoundSettings; }

	APawn* FindNearestUnfoundHider(const APawn* SeekerPawn) const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HideSeek")
	TSoftObjectPtr<UHideSeekRoundSettings> RoundSettingsAsset;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HideSeek")
	TObjectPtr<UHideSeekRoundSettings> RoundSettingsOverride;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HideSeek")
	TSubclassOf<AHideSeekAIController> HideSeekAIControllerClass;

protected:
	UFUNCTION()
	void HandlePlayRoomLoaded();

private:
	void LoadRoundSettings();
	void ScheduleAutoStartIfNeeded();
	void SelectSeeker();
	void BeginLoadingPlayRoom();
	void BeginHideTime();
	void BeginSeekTime();
	void CheckCaptureState();
	void EndRound(EHideSeekEndReason Reason);
	void ResetRoundToLobby();
	void UpdateRemainingClock();
	void SetPhase(EHideSeekPhase NewPhase, float DurationSeconds);
	void PlayTransitionForAll(const FText& Message) const;
	void LoadPlayRoomLevel();
	void UnloadPlayRoomLevel();
	void EnsureParticipants();
	void CollectParticipants();
	void SpawnAIParticipantsIfNeeded();
	void AssignParticipantIdsAndDefaults();
	void SetParticipantRole(APawn* Pawn, EHideSeekRole Role);
	void SetParticipantFound(APawn* Pawn, bool bFound);
	void MoveParticipantToTaggedSpawn(APawn* Pawn, FName SpawnTag) const;
	void ApplyControlStates();
	void ApplyControlState(APawn* Pawn) const;
	UHideSeekRoleComponent* EnsureRoleComponent(APawn* Pawn) const;
	UHideSeekRoleComponent* GetRoleComponent(const APawn* Pawn) const;
	FTransform ChooseTaggedSpawnTransform(FName SpawnTag, const FTransform& FallbackTransform) const;
	TArray<AActor*> FindActorsWithTag(FName Tag) const;
	UClass* ResolveParticipantPawnClass() const;

	UPROPERTY(Transient)
	TObjectPtr<UHideSeekRoundSettings> RoundSettings;

	UPROPERTY(Transient)
	TArray<TObjectPtr<APawn>> Participants;

	UPROPERTY(Transient)
	TArray<TObjectPtr<APawn>> SpawnedAIPawns;

	FTimerHandle AutoStartTimerHandle;
	FTimerHandle PhaseTimerHandle;
	FTimerHandle ClockTimerHandle;
	FTimerHandle CaptureTimerHandle;

	mutable FRandomStream RoundRandom;
	float PhaseEndTimeSeconds = 0.0f;
	float LoadingStartedTimeSeconds = 0.0f;
	int32 LatentActionUUID = 1000;
};

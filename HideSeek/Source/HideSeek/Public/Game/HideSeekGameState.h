#pragma once

#include "CoreMinimal.h"
#include "Game/HideSeekTypes.h"
#include "GameFramework/GameStateBase.h"
#include "HideSeekGameState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHideSeekPhaseChangedSignature, EHideSeekPhase, NewPhase);

UCLASS()
class HIDESEEK_API AHideSeekGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "HideSeek")
	EHideSeekPhase GetRoundPhase() const { return RoundPhase; }

	UFUNCTION(BlueprintPure, Category = "HideSeek")
	EHideSeekEndReason GetEndReason() const { return EndReason; }

	UFUNCTION(BlueprintPure, Category = "HideSeek")
	float GetRemainingSeconds() const { return RemainingSeconds; }

	UFUNCTION(BlueprintPure, Category = "HideSeek")
	int32 GetRoundNumber() const { return RoundNumber; }

	void SetRoundPhaseAuthority(EHideSeekPhase NewPhase);
	void SetEndReasonAuthority(EHideSeekEndReason NewEndReason);
	void SetRemainingSecondsAuthority(float NewRemainingSeconds);
	void SetRoundNumberAuthority(int32 NewRoundNumber);

	UPROPERTY(BlueprintAssignable, Category = "HideSeek")
	FHideSeekPhaseChangedSignature OnRoundPhaseChanged;

private:
	UFUNCTION()
	void OnRep_RoundPhase(EHideSeekPhase PreviousPhase);

	UPROPERTY(ReplicatedUsing = OnRep_RoundPhase)
	EHideSeekPhase RoundPhase = EHideSeekPhase::WaitingForPlayers;

	UPROPERTY(Replicated)
	EHideSeekEndReason EndReason = EHideSeekEndReason::None;

	UPROPERTY(Replicated)
	float RemainingSeconds = 0.0f;

	UPROPERTY(Replicated)
	int32 RoundNumber = 0;
};

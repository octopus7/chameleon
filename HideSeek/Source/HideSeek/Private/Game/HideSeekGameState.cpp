#include "Game/HideSeekGameState.h"

#include "Net/UnrealNetwork.h"

void AHideSeekGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AHideSeekGameState, RoundPhase);
	DOREPLIFETIME(AHideSeekGameState, EndReason);
	DOREPLIFETIME(AHideSeekGameState, RemainingSeconds);
	DOREPLIFETIME(AHideSeekGameState, RoundNumber);
}

void AHideSeekGameState::SetRoundPhaseAuthority(EHideSeekPhase NewPhase)
{
	if (!HasAuthority() || RoundPhase == NewPhase)
	{
		return;
	}

	const EHideSeekPhase PreviousPhase = RoundPhase;
	RoundPhase = NewPhase;
	OnRep_RoundPhase(PreviousPhase);
}

void AHideSeekGameState::SetEndReasonAuthority(EHideSeekEndReason NewEndReason)
{
	if (HasAuthority())
	{
		EndReason = NewEndReason;
	}
}

void AHideSeekGameState::SetRemainingSecondsAuthority(float NewRemainingSeconds)
{
	if (HasAuthority())
	{
		RemainingSeconds = FMath::Max(0.0f, NewRemainingSeconds);
	}
}

void AHideSeekGameState::SetRoundNumberAuthority(int32 NewRoundNumber)
{
	if (HasAuthority())
	{
		RoundNumber = NewRoundNumber;
	}
}

void AHideSeekGameState::OnRep_RoundPhase(EHideSeekPhase PreviousPhase)
{
	if (PreviousPhase != RoundPhase)
	{
		OnRoundPhaseChanged.Broadcast(RoundPhase);
	}
}

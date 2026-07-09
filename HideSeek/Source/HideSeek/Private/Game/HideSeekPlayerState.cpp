#include "Game/HideSeekPlayerState.h"

#include "Net/UnrealNetwork.h"

void AHideSeekPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AHideSeekPlayerState, HideSeekRole);
	DOREPLIFETIME(AHideSeekPlayerState, bFound);
	DOREPLIFETIME(AHideSeekPlayerState, ParticipantId);
}

void AHideSeekPlayerState::SetHideSeekRoleAuthority(EHideSeekRole NewRole)
{
	if (HasAuthority())
	{
		HideSeekRole = NewRole;
	}
}

void AHideSeekPlayerState::SetFoundAuthority(bool bNewFound)
{
	if (HasAuthority())
	{
		bFound = bNewFound;
	}
}

void AHideSeekPlayerState::SetParticipantIdAuthority(int32 NewParticipantId)
{
	if (HasAuthority())
	{
		ParticipantId = NewParticipantId;
	}
}

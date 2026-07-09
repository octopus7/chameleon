#include "Components/HideSeekRoleComponent.h"

#include "Net/UnrealNetwork.h"

UHideSeekRoleComponent::UHideSeekRoleComponent()
{
	SetIsReplicatedByDefault(true);
}

void UHideSeekRoleComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UHideSeekRoleComponent, Role);
	DOREPLIFETIME(UHideSeekRoleComponent, bFound);
	DOREPLIFETIME(UHideSeekRoleComponent, bAIControlledParticipant);
	DOREPLIFETIME(UHideSeekRoleComponent, ParticipantId);
}

void UHideSeekRoleComponent::SetRoleAuthority(EHideSeekRole NewRole)
{
	if (GetOwnerRole() != ROLE_Authority || Role == NewRole)
	{
		return;
	}

	const EHideSeekRole PreviousRole = Role;
	Role = NewRole;
	OnRep_Role(PreviousRole);
}

void UHideSeekRoleComponent::SetFoundAuthority(bool bNewFound)
{
	if (GetOwnerRole() != ROLE_Authority || bFound == bNewFound)
	{
		return;
	}

	bFound = bNewFound;
	OnRep_Found();
}

void UHideSeekRoleComponent::SetAIControlledParticipantAuthority(bool bNewAIControlledParticipant)
{
	if (GetOwnerRole() == ROLE_Authority)
	{
		bAIControlledParticipant = bNewAIControlledParticipant;
	}
}

void UHideSeekRoleComponent::SetParticipantIdAuthority(int32 NewParticipantId)
{
	if (GetOwnerRole() == ROLE_Authority)
	{
		ParticipantId = NewParticipantId;
	}
}

void UHideSeekRoleComponent::OnRep_Role(EHideSeekRole PreviousRole)
{
	if (PreviousRole != Role)
	{
		OnRoleChanged.Broadcast(Role);
	}
}

void UHideSeekRoleComponent::OnRep_Found()
{
	OnFoundChanged.Broadcast(bFound);
}

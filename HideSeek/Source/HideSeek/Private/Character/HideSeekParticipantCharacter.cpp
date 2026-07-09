#include "Character/HideSeekParticipantCharacter.h"

#include "Components/HideSeekRoleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

AHideSeekParticipantCharacter::AHideSeekParticipantCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bReplicates = true;
	HideSeekRoleComponent = CreateDefaultSubobject<UHideSeekRoleComponent>(TEXT("HideSeekRole"));
}

void AHideSeekParticipantCharacter::SetHideSeekMovementEnabled(bool bEnabled)
{
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (!Movement)
	{
		return;
	}

	if (bEnabled)
	{
		if (Movement->MovementMode == MOVE_None)
		{
			Movement->SetMovementMode(MOVE_Walking);
		}
	}
	else
	{
		Movement->StopMovementImmediately();
		Movement->DisableMovement();
	}
}

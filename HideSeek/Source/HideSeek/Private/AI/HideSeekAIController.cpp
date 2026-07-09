#include "AI/HideSeekAIController.h"

#include "Components/HideSeekRoleComponent.h"
#include "Data/HideSeekRoundSettings.h"
#include "Game/HideSeekGameMode.h"
#include "Game/HideSeekGameState.h"
#include "NavigationSystem.h"

AHideSeekAIController::AHideSeekAIController()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AHideSeekAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UWorld* World = GetWorld();
	if (!World || World->GetTimeSeconds() < NextDecisionTimeSeconds)
	{
		return;
	}

	UpdateHideSeekBehavior();
}

void AHideSeekAIController::UpdateHideSeekBehavior()
{
	UWorld* World = GetWorld();
	APawn* ControlledPawn = GetPawn();
	AHideSeekGameMode* GameMode = World ? World->GetAuthGameMode<AHideSeekGameMode>() : nullptr;
	AHideSeekGameState* GameState = World ? World->GetGameState<AHideSeekGameState>() : nullptr;
	UHideSeekRoleComponent* RoleComponent = ControlledPawn ? ControlledPawn->FindComponentByClass<UHideSeekRoleComponent>() : nullptr;
	const UHideSeekRoundSettings* Settings = GameMode ? GameMode->GetRoundSettings() : nullptr;
	if (!World || !ControlledPawn || !GameMode || !GameState || !RoleComponent || !Settings)
	{
		NextDecisionTimeSeconds = World ? World->GetTimeSeconds() + 1.0f : 0.0f;
		StopMovement();
		return;
	}

	NextDecisionTimeSeconds = World->GetTimeSeconds() + FMath::Max(0.1f, Settings->AIDecisionIntervalSeconds);

	if (RoleComponent->IsFound())
	{
		StopMovement();
		return;
	}

	const EHideSeekPhase Phase = GameState->GetRoundPhase();
	const EHideSeekRole Role = RoleComponent->GetRole();

	if (Phase == EHideSeekPhase::HideTime && Role == EHideSeekRole::Hider)
	{
		MoveToRandomReachablePoint(Settings->AIWanderRadiusCm);
		return;
	}

	if (Phase == EHideSeekPhase::SeekTime)
	{
		if (Role == EHideSeekRole::Seeker)
		{
			if (APawn* Target = GameMode->FindNearestUnfoundHider(ControlledPawn))
			{
				MoveToActor(Target, Settings->AIMoveAcceptanceRadiusCm, true, true, true);
			}
			else
			{
				MoveToRandomReachablePoint(Settings->AIWanderRadiusCm);
			}
			return;
		}

		if (Role == EHideSeekRole::Hider)
		{
			MoveToRandomReachablePoint(Settings->AIWanderRadiusCm);
			return;
		}
	}

	StopMovement();
}

bool AHideSeekAIController::MoveToRandomReachablePoint(float RadiusCm)
{
	APawn* ControlledPawn = GetPawn();
	UWorld* World = GetWorld();
	UNavigationSystemV1* NavigationSystem = World ? FNavigationSystem::GetCurrent<UNavigationSystemV1>(World) : nullptr;
	if (!ControlledPawn || !NavigationSystem)
	{
		return false;
	}

	FNavLocation RandomLocation;
	if (!NavigationSystem->GetRandomReachablePointInRadius(ControlledPawn->GetActorLocation(), RadiusCm, RandomLocation))
	{
		return false;
	}

	MoveToLocation(RandomLocation.Location, 90.0f, true, true, true, false);
	return true;
}

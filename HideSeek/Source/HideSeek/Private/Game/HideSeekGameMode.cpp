#include "Game/HideSeekGameMode.h"

#include "AI/HideSeekAIController.h"
#include "Character/HideSeekParticipantCharacter.h"
#include "Components/HideSeekRoleComponent.h"
#include "Data/HideSeekRoundSettings.h"
#include "Engine/LatentActionManager.h"
#include "EngineUtils.h"
#include "Game/HideSeekGameState.h"
#include "Game/HideSeekPlayerController.h"
#include "Game/HideSeekPlayerState.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

AHideSeekGameMode::AHideSeekGameMode()
{
	GameStateClass = AHideSeekGameState::StaticClass();
	PlayerControllerClass = AHideSeekPlayerController::StaticClass();
	PlayerStateClass = AHideSeekPlayerState::StaticClass();
	DefaultPawnClass = AHideSeekParticipantCharacter::StaticClass();
	HideSeekAIControllerClass = AHideSeekAIController::StaticClass();
	RoundSettingsAsset = TSoftObjectPtr<UHideSeekRoundSettings>(FSoftObjectPath(TEXT("/Game/HideSeek/Data/DA_HideSeekRoundSettings.DA_HideSeekRoundSettings")));
}

void AHideSeekGameMode::BeginPlay()
{
	Super::BeginPlay();

	LoadRoundSettings();
	if (AHideSeekGameState* HideSeekGameState = GetGameState<AHideSeekGameState>())
	{
		HideSeekGameState->SetRoundPhaseAuthority(EHideSeekPhase::WaitingForPlayers);
		HideSeekGameState->SetEndReasonAuthority(EHideSeekEndReason::None);
		HideSeekGameState->SetRemainingSecondsAuthority(0.0f);
	}

	UnloadPlayRoomLevel();
	EnsureParticipants();
	ScheduleAutoStartIfNeeded();
}

void AHideSeekGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	FTimerHandle RegisterTimerHandle;
	GetWorldTimerManager().SetTimer(
		RegisterTimerHandle,
		this,
		&AHideSeekGameMode::EnsureParticipants,
		0.1f,
		false);
}

UClass* AHideSeekGameMode::GetDefaultPawnClassForController_Implementation(AController* InController)
{
	LoadRoundSettings();
	if (UClass* ParticipantClass = ResolveParticipantPawnClass())
	{
		return ParticipantClass;
	}

	return Super::GetDefaultPawnClassForController_Implementation(InController);
}

void AHideSeekGameMode::StartHideSeekRound()
{
	if (!HasAuthority())
	{
		return;
	}

	LoadRoundSettings();
	EnsureParticipants();
	if (Participants.IsEmpty())
	{
		ScheduleAutoStartIfNeeded();
		return;
	}

	GetWorldTimerManager().ClearTimer(AutoStartTimerHandle);
	GetWorldTimerManager().ClearTimer(PhaseTimerHandle);
	GetWorldTimerManager().ClearTimer(CaptureTimerHandle);

	RoundRandom.Initialize(FMath::Rand());
	if (AHideSeekGameState* HideSeekGameState = GetGameState<AHideSeekGameState>())
	{
		HideSeekGameState->SetRoundNumberAuthority(HideSeekGameState->GetRoundNumber() + 1);
		HideSeekGameState->SetEndReasonAuthority(EHideSeekEndReason::None);
	}

	AssignParticipantIdsAndDefaults();
	SetPhase(EHideSeekPhase::SelectingSeeker, RoundSettings ? RoundSettings->SeekerSelectionDelaySeconds : 1.0f);
	PlayTransitionForAll(FText::FromString(TEXT("Selecting Seeker")));
	GetWorldTimerManager().SetTimer(
		PhaseTimerHandle,
		this,
		&AHideSeekGameMode::SelectSeeker,
		RoundSettings ? RoundSettings->SeekerSelectionDelaySeconds : 1.0f,
		false);
}

APawn* AHideSeekGameMode::FindNearestUnfoundHider(const APawn* SeekerPawn) const
{
	if (!SeekerPawn)
	{
		return nullptr;
	}

	APawn* BestPawn = nullptr;
	double BestDistanceSquared = TNumericLimits<double>::Max();
	for (APawn* Participant : Participants)
	{
		const UHideSeekRoleComponent* RoleComponent = GetRoleComponent(Participant);
		if (!Participant || !RoleComponent || RoleComponent->GetRole() != EHideSeekRole::Hider || RoleComponent->IsFound())
		{
			continue;
		}

		const double DistanceSquared = FVector::DistSquared(SeekerPawn->GetActorLocation(), Participant->GetActorLocation());
		if (DistanceSquared < BestDistanceSquared)
		{
			BestDistanceSquared = DistanceSquared;
			BestPawn = Participant;
		}
	}

	return BestPawn;
}

void AHideSeekGameMode::HandlePlayRoomLoaded()
{
	const float HoldSeconds = RoundSettings ? RoundSettings->LoadingHoldSeconds : 0.0f;
	const float ElapsedSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() - LoadingStartedTimeSeconds : HoldSeconds;
	const float RemainingHoldSeconds = FMath::Max(0.0f, HoldSeconds - ElapsedSeconds);
	GetWorldTimerManager().SetTimer(
		PhaseTimerHandle,
		this,
		&AHideSeekGameMode::BeginHideTime,
		RemainingHoldSeconds,
		false);
}

void AHideSeekGameMode::LoadRoundSettings()
{
	if (RoundSettingsOverride)
	{
		RoundSettings = RoundSettingsOverride;
		return;
	}

	if (!RoundSettings && !RoundSettingsAsset.IsNull())
	{
		RoundSettings = Cast<UHideSeekRoundSettings>(RoundSettingsAsset.TryLoad());
	}

	if (!RoundSettings)
	{
		RoundSettings = NewObject<UHideSeekRoundSettings>(this, TEXT("RuntimeHideSeekRoundSettings"));
	}
}

void AHideSeekGameMode::ScheduleAutoStartIfNeeded()
{
	if (!RoundSettings || !RoundSettings->bAutoStartRound)
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(AutoStartTimerHandle);
	GetWorldTimerManager().SetTimer(
		AutoStartTimerHandle,
		this,
		&AHideSeekGameMode::StartHideSeekRound,
		RoundSettings->AutoStartDelaySeconds,
		false);
}

void AHideSeekGameMode::SelectSeeker()
{
	EnsureParticipants();
	if (Participants.IsEmpty())
	{
		ScheduleAutoStartIfNeeded();
		return;
	}

	const int32 SeekerIndex = RoundRandom.RandRange(0, Participants.Num() - 1);
	for (int32 Index = 0; Index < Participants.Num(); ++Index)
	{
		APawn* Participant = Participants[Index];
		const bool bIsSeeker = Index == SeekerIndex;
		SetParticipantRole(Participant, bIsSeeker ? EHideSeekRole::Seeker : EHideSeekRole::Hider);
		SetParticipantFound(Participant, false);
		MoveParticipantToTaggedSpawn(Participant, bIsSeeker ? RoundSettings->LobbyCenterTag : RoundSettings->LobbySpawnTag);
	}

	SetPhase(EHideSeekPhase::SeekerLobbyWait, RoundSettings->SeekerLobbyWaitSeconds);
	ApplyControlStates();
	GetWorldTimerManager().SetTimer(
		PhaseTimerHandle,
		this,
		&AHideSeekGameMode::BeginLoadingPlayRoom,
		RoundSettings->SeekerLobbyWaitSeconds,
		false);
}

void AHideSeekGameMode::BeginLoadingPlayRoom()
{
	LoadingStartedTimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	SetPhase(EHideSeekPhase::LoadingPlayRoom, RoundSettings->LoadingHoldSeconds);
	ApplyControlStates();
	PlayTransitionForAll(FText::FromString(TEXT("Loading Play Room")));
	LoadPlayRoomLevel();
}

void AHideSeekGameMode::BeginHideTime()
{
	for (APawn* Participant : Participants)
	{
		if (const UHideSeekRoleComponent* RoleComponent = GetRoleComponent(Participant))
		{
			if (RoleComponent->GetRole() == EHideSeekRole::Hider)
			{
				MoveParticipantToTaggedSpawn(Participant, RoundSettings->PlayRoomHiderSpawnTag);
			}
			else if (RoleComponent->GetRole() == EHideSeekRole::Seeker)
			{
				MoveParticipantToTaggedSpawn(Participant, RoundSettings->LobbyCenterTag);
			}
		}
	}

	SetPhase(EHideSeekPhase::HideTime, RoundSettings->HideTimeSeconds);
	ApplyControlStates();
	GetWorldTimerManager().SetTimer(
		PhaseTimerHandle,
		this,
		&AHideSeekGameMode::BeginSeekTime,
		RoundSettings->HideTimeSeconds,
		false);
}

void AHideSeekGameMode::BeginSeekTime()
{
	for (APawn* Participant : Participants)
	{
		if (const UHideSeekRoleComponent* RoleComponent = GetRoleComponent(Participant))
		{
			if (RoleComponent->GetRole() == EHideSeekRole::Seeker)
			{
				MoveParticipantToTaggedSpawn(Participant, RoundSettings->PlayRoomSeekerSpawnTag);
			}
		}
	}

	SetPhase(EHideSeekPhase::SeekTime, RoundSettings->SeekTimeSeconds);
	ApplyControlStates();
	PlayTransitionForAll(FText::FromString(TEXT("Seek Time")));

	GetWorldTimerManager().SetTimer(
		CaptureTimerHandle,
		this,
		&AHideSeekGameMode::CheckCaptureState,
		RoundSettings->CaptureCheckIntervalSeconds,
		true);
	GetWorldTimerManager().SetTimer(
		PhaseTimerHandle,
		FTimerDelegate::CreateUObject(this, &AHideSeekGameMode::EndRound, EHideSeekEndReason::TimeExpired),
		RoundSettings->SeekTimeSeconds,
		false);
}

void AHideSeekGameMode::CheckCaptureState()
{
	if (!RoundSettings)
	{
		return;
	}

	TArray<APawn*> Seekers;
	TArray<APawn*> Hiders;
	for (APawn* Participant : Participants)
	{
		const UHideSeekRoleComponent* RoleComponent = GetRoleComponent(Participant);
		if (!Participant || !RoleComponent)
		{
			continue;
		}

		if (RoleComponent->GetRole() == EHideSeekRole::Seeker)
		{
			Seekers.Add(Participant);
		}
		else if (RoleComponent->GetRole() == EHideSeekRole::Hider && !RoleComponent->IsFound())
		{
			Hiders.Add(Participant);
		}
	}

	if (Hiders.IsEmpty())
	{
		EndRound(EHideSeekEndReason::AllHidersFound);
		return;
	}

	const double CaptureDistanceSquared = FMath::Square(static_cast<double>(RoundSettings->CaptureRadiusCm));
	for (APawn* Seeker : Seekers)
	{
		if (!Seeker)
		{
			continue;
		}

		for (APawn* Hider : Hiders)
		{
			UHideSeekRoleComponent* HiderRoleComponent = GetRoleComponent(Hider);
			if (!Hider || !HiderRoleComponent || HiderRoleComponent->IsFound())
			{
				continue;
			}

			if (FVector::DistSquared(Seeker->GetActorLocation(), Hider->GetActorLocation()) <= CaptureDistanceSquared)
			{
				SetParticipantFound(Hider, true);
				MoveParticipantToTaggedSpawn(Hider, RoundSettings->LobbySpawnTag);
				ApplyControlState(Hider);
			}
		}
	}

	for (APawn* Hider : Hiders)
	{
		const UHideSeekRoleComponent* HiderRoleComponent = GetRoleComponent(Hider);
		if (HiderRoleComponent && !HiderRoleComponent->IsFound())
		{
			return;
		}
	}

	EndRound(EHideSeekEndReason::AllHidersFound);
}

void AHideSeekGameMode::EndRound(EHideSeekEndReason Reason)
{
	GetWorldTimerManager().ClearTimer(PhaseTimerHandle);
	GetWorldTimerManager().ClearTimer(CaptureTimerHandle);

	if (AHideSeekGameState* HideSeekGameState = GetGameState<AHideSeekGameState>())
	{
		HideSeekGameState->SetEndReasonAuthority(Reason);
	}

	SetPhase(EHideSeekPhase::RoundEnded, RoundSettings ? RoundSettings->RoundResetDelaySeconds : 3.0f);
	ApplyControlStates();
	PlayTransitionForAll(Reason == EHideSeekEndReason::AllHidersFound
		? FText::FromString(TEXT("All Hiders Found"))
		: FText::FromString(TEXT("Time Expired")));

	GetWorldTimerManager().SetTimer(
		PhaseTimerHandle,
		this,
		&AHideSeekGameMode::ResetRoundToLobby,
		RoundSettings ? RoundSettings->RoundResetDelaySeconds : 3.0f,
		false);
}

void AHideSeekGameMode::ResetRoundToLobby()
{
	for (APawn* Participant : Participants)
	{
		SetParticipantRole(Participant, EHideSeekRole::None);
		SetParticipantFound(Participant, false);
		MoveParticipantToTaggedSpawn(Participant, RoundSettings ? RoundSettings->LobbySpawnTag : NAME_None);
	}

	UnloadPlayRoomLevel();
	SetPhase(EHideSeekPhase::WaitingForPlayers, 0.0f);
	ApplyControlStates();

	if (RoundSettings && RoundSettings->bLoopRounds)
	{
		ScheduleAutoStartIfNeeded();
	}
}

void AHideSeekGameMode::UpdateRemainingClock()
{
	AHideSeekGameState* HideSeekGameState = GetGameState<AHideSeekGameState>();
	UWorld* World = GetWorld();
	if (!HideSeekGameState || !World)
	{
		return;
	}

	if (PhaseEndTimeSeconds <= 0.0f)
	{
		HideSeekGameState->SetRemainingSecondsAuthority(0.0f);
		return;
	}

	HideSeekGameState->SetRemainingSecondsAuthority(PhaseEndTimeSeconds - World->GetTimeSeconds());
}

void AHideSeekGameMode::SetPhase(EHideSeekPhase NewPhase, float DurationSeconds)
{
	UWorld* World = GetWorld();
	if (AHideSeekGameState* HideSeekGameState = GetGameState<AHideSeekGameState>())
	{
		HideSeekGameState->SetRoundPhaseAuthority(NewPhase);
		HideSeekGameState->SetRemainingSecondsAuthority(DurationSeconds);
	}

	PhaseEndTimeSeconds = World && DurationSeconds > 0.0f ? World->GetTimeSeconds() + DurationSeconds : 0.0f;
	if (DurationSeconds > 0.0f)
	{
		GetWorldTimerManager().SetTimer(
			ClockTimerHandle,
			this,
			&AHideSeekGameMode::UpdateRemainingClock,
			0.2f,
			true);
	}
	else
	{
		GetWorldTimerManager().ClearTimer(ClockTimerHandle);
	}
}

void AHideSeekGameMode::PlayTransitionForAll(const FText& Message) const
{
	if (!RoundSettings)
	{
		return;
	}

	FSoftClassPath WidgetClassPath;
	if (RoundSettings->TransitionWidgetClass)
	{
		WidgetClassPath = FSoftClassPath(RoundSettings->TransitionWidgetClass->GetPathName());
	}

	const FSoftObjectPath NiagaraSystemPath = RoundSettings->TransitionNiagaraSystem.ToSoftObjectPath();
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (AHideSeekPlayerController* HideSeekPlayerController = Cast<AHideSeekPlayerController>(It->Get()))
		{
			HideSeekPlayerController->ClientPlayHideSeekTransition(
				Message,
				RoundSettings->TransitionFadeInSeconds,
				RoundSettings->TransitionHoldSeconds,
				RoundSettings->TransitionFadeOutSeconds,
				WidgetClassPath,
				NiagaraSystemPath,
				RoundSettings->TransitionNiagaraLocalOffset,
				RoundSettings->TransitionNiagaraScale,
				RoundSettings->TransitionNiagaraLifetimeSeconds);
		}
	}
}

void AHideSeekGameMode::LoadPlayRoomLevel()
{
	if (!RoundSettings || RoundSettings->PlayRoomStreamLevelName.IsNone())
	{
		HandlePlayRoomLoaded();
		return;
	}

	FLatentActionInfo LatentInfo;
	LatentInfo.CallbackTarget = this;
	LatentInfo.ExecutionFunction = GET_FUNCTION_NAME_CHECKED(AHideSeekGameMode, HandlePlayRoomLoaded);
	LatentInfo.Linkage = 0;
	LatentInfo.UUID = ++LatentActionUUID;
	UGameplayStatics::LoadStreamLevel(this, RoundSettings->PlayRoomStreamLevelName, true, false, LatentInfo);
}

void AHideSeekGameMode::UnloadPlayRoomLevel()
{
	if (!RoundSettings)
	{
		LoadRoundSettings();
	}

	if (!RoundSettings || RoundSettings->PlayRoomStreamLevelName.IsNone())
	{
		return;
	}

	FLatentActionInfo LatentInfo;
	LatentInfo.CallbackTarget = this;
	LatentInfo.UUID = ++LatentActionUUID;
	UGameplayStatics::UnloadStreamLevel(this, RoundSettings->PlayRoomStreamLevelName, LatentInfo, false);
}

void AHideSeekGameMode::EnsureParticipants()
{
	LoadRoundSettings();
	CollectParticipants();
	SpawnAIParticipantsIfNeeded();
	CollectParticipants();
	AssignParticipantIdsAndDefaults();
	ApplyControlStates();
}

void AHideSeekGameMode::CollectParticipants()
{
	Participants.Reset();
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		if (APlayerController* PlayerController = It->Get())
		{
			if (APawn* Pawn = PlayerController->GetPawn())
			{
				EnsureRoleComponent(Pawn);
				Participants.AddUnique(Pawn);
			}
		}
	}

	for (int32 Index = SpawnedAIPawns.Num() - 1; Index >= 0; --Index)
	{
		APawn* Pawn = SpawnedAIPawns[Index];
		if (!IsValid(Pawn))
		{
			SpawnedAIPawns.RemoveAtSwap(Index);
			continue;
		}

		EnsureRoleComponent(Pawn);
		Participants.AddUnique(Pawn);
	}
}

void AHideSeekGameMode::SpawnAIParticipantsIfNeeded()
{
	if (!RoundSettings || !RoundSettings->bAutoSpawnAI)
	{
		return;
	}

	UWorld* World = GetWorld();
	UClass* PawnClass = ResolveParticipantPawnClass();
	if (!World || !PawnClass || !HideSeekAIControllerClass)
	{
		return;
	}

	while (Participants.Num() < RoundSettings->TargetParticipantCount)
	{
		const FTransform SpawnTransform = ChooseTaggedSpawnTransform(RoundSettings->LobbySpawnTag, FTransform::Identity);
		FActorSpawnParameters PawnSpawnParameters;
		PawnSpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		APawn* AIPawn = World->SpawnActor<APawn>(PawnClass, SpawnTransform, PawnSpawnParameters);
		if (!AIPawn)
		{
			break;
		}

		if (UHideSeekRoleComponent* RoleComponent = EnsureRoleComponent(AIPawn))
		{
			RoleComponent->SetAIControlledParticipantAuthority(true);
		}

		FActorSpawnParameters ControllerSpawnParameters;
		ControllerSpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AHideSeekAIController* AIController = World->SpawnActor<AHideSeekAIController>(
			HideSeekAIControllerClass,
			SpawnTransform.GetLocation(),
			SpawnTransform.GetRotation().Rotator(),
			ControllerSpawnParameters);
		if (AIController)
		{
			AIController->Possess(AIPawn);
		}

		SpawnedAIPawns.Add(AIPawn);
		Participants.Add(AIPawn);
	}
}

void AHideSeekGameMode::AssignParticipantIdsAndDefaults()
{
	for (int32 Index = 0; Index < Participants.Num(); ++Index)
	{
		APawn* Participant = Participants[Index];
		if (UHideSeekRoleComponent* RoleComponent = EnsureRoleComponent(Participant))
		{
			RoleComponent->SetParticipantIdAuthority(Index);
			RoleComponent->SetAIControlledParticipantAuthority(Participant && !Cast<APlayerController>(Participant->GetController()));
		}

		if (Participant)
		{
			if (AHideSeekPlayerState* HideSeekPlayerState = Participant->GetPlayerState<AHideSeekPlayerState>())
			{
				HideSeekPlayerState->SetParticipantIdAuthority(Index);
			}
		}
	}
}

void AHideSeekGameMode::SetParticipantRole(APawn* Pawn, EHideSeekRole Role)
{
	if (UHideSeekRoleComponent* RoleComponent = EnsureRoleComponent(Pawn))
	{
		RoleComponent->SetRoleAuthority(Role);
	}

	if (AHideSeekPlayerState* HideSeekPlayerState = Pawn ? Pawn->GetPlayerState<AHideSeekPlayerState>() : nullptr)
	{
		HideSeekPlayerState->SetHideSeekRoleAuthority(Role);
	}
}

void AHideSeekGameMode::SetParticipantFound(APawn* Pawn, bool bFound)
{
	if (UHideSeekRoleComponent* RoleComponent = EnsureRoleComponent(Pawn))
	{
		RoleComponent->SetFoundAuthority(bFound);
	}

	if (AHideSeekPlayerState* HideSeekPlayerState = Pawn ? Pawn->GetPlayerState<AHideSeekPlayerState>() : nullptr)
	{
		HideSeekPlayerState->SetFoundAuthority(bFound);
	}
}

void AHideSeekGameMode::MoveParticipantToTaggedSpawn(APawn* Pawn, FName SpawnTag) const
{
	if (!Pawn)
	{
		return;
	}

	const FTransform SpawnTransform = ChooseTaggedSpawnTransform(SpawnTag, Pawn->GetActorTransform());
	Pawn->SetActorLocationAndRotation(
		SpawnTransform.GetLocation(),
		SpawnTransform.GetRotation(),
		false,
		nullptr,
		ETeleportType::TeleportPhysics);
	if (AController* Controller = Pawn->GetController())
	{
		Controller->SetControlRotation(SpawnTransform.GetRotation().Rotator());
	}
}

void AHideSeekGameMode::ApplyControlStates()
{
	for (APawn* Participant : Participants)
	{
		ApplyControlState(Participant);
	}
}

void AHideSeekGameMode::ApplyControlState(APawn* Pawn) const
{
	if (!Pawn)
	{
		return;
	}

	const AHideSeekGameState* HideSeekGameState = GetGameState<AHideSeekGameState>();
	const UHideSeekRoleComponent* RoleComponent = GetRoleComponent(Pawn);
	const EHideSeekPhase Phase = HideSeekGameState ? HideSeekGameState->GetRoundPhase() : EHideSeekPhase::WaitingForPlayers;
	const EHideSeekRole Role = RoleComponent ? RoleComponent->GetRole() : EHideSeekRole::None;
	const bool bFound = RoleComponent && RoleComponent->IsFound();
	const bool bIsSeekerWaiting = Role == EHideSeekRole::Seeker
		&& (Phase == EHideSeekPhase::SeekerLobbyWait || Phase == EHideSeekPhase::HideTime);

	bool bMovementEnabled = true;
	bool bLookEnabled = true;
	switch (Phase)
	{
	case EHideSeekPhase::SelectingSeeker:
	case EHideSeekPhase::LoadingPlayRoom:
	case EHideSeekPhase::RoundEnded:
		bMovementEnabled = false;
		break;
	case EHideSeekPhase::SeekerLobbyWait:
	case EHideSeekPhase::HideTime:
		bMovementEnabled = !bIsSeekerWaiting && !bFound;
		bLookEnabled = !bIsSeekerWaiting || (RoundSettings && RoundSettings->bSeekerCanLookWhileWaiting);
		break;
	case EHideSeekPhase::SeekTime:
		bMovementEnabled = !bFound;
		break;
	case EHideSeekPhase::WaitingForPlayers:
	default:
		bMovementEnabled = !bFound;
		break;
	}

	if (AHideSeekParticipantCharacter* ParticipantCharacter = Cast<AHideSeekParticipantCharacter>(Pawn))
	{
		ParticipantCharacter->SetHideSeekMovementEnabled(bMovementEnabled);
	}
	else if (ACharacter* Character = Cast<ACharacter>(Pawn))
	{
		if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
		{
			if (bMovementEnabled)
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
	}

	if (AController* Controller = Pawn->GetController())
	{
		Controller->SetIgnoreMoveInput(!bMovementEnabled);
		Controller->SetIgnoreLookInput(!bLookEnabled);
		if (!bMovementEnabled)
		{
			Controller->StopMovement();
		}
		if (AHideSeekPlayerController* HideSeekPlayerController = Cast<AHideSeekPlayerController>(Controller))
		{
			HideSeekPlayerController->ClientSetHideSeekInputEnabled(bMovementEnabled, bLookEnabled);
		}
	}
}

UHideSeekRoleComponent* AHideSeekGameMode::EnsureRoleComponent(APawn* Pawn) const
{
	if (!Pawn)
	{
		return nullptr;
	}

	if (UHideSeekRoleComponent* ExistingComponent = Pawn->FindComponentByClass<UHideSeekRoleComponent>())
	{
		return ExistingComponent;
	}

	UHideSeekRoleComponent* RoleComponent = NewObject<UHideSeekRoleComponent>(Pawn, TEXT("HideSeekRole"));
	if (RoleComponent)
	{
		Pawn->AddInstanceComponent(RoleComponent);
		RoleComponent->RegisterComponent();
		RoleComponent->SetIsReplicated(true);
	}
	return RoleComponent;
}

UHideSeekRoleComponent* AHideSeekGameMode::GetRoleComponent(const APawn* Pawn) const
{
	return Pawn ? Pawn->FindComponentByClass<UHideSeekRoleComponent>() : nullptr;
}

FTransform AHideSeekGameMode::ChooseTaggedSpawnTransform(FName SpawnTag, const FTransform& FallbackTransform) const
{
	const TArray<AActor*> SpawnActors = FindActorsWithTag(SpawnTag);
	if (SpawnActors.IsEmpty())
	{
		return FallbackTransform;
	}

	const int32 ChosenIndex = RoundRandom.RandRange(0, SpawnActors.Num() - 1);
	return SpawnActors[ChosenIndex] ? SpawnActors[ChosenIndex]->GetActorTransform() : FallbackTransform;
}

TArray<AActor*> AHideSeekGameMode::FindActorsWithTag(FName Tag) const
{
	TArray<AActor*> Actors;
	if (Tag.IsNone() || !GetWorld())
	{
		return Actors;
	}

	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		AActor* Actor = *It;
		if (Actor && Actor->ActorHasTag(Tag))
		{
			Actors.Add(Actor);
		}
	}

	return Actors;
}

UClass* AHideSeekGameMode::ResolveParticipantPawnClass() const
{
	if (RoundSettings && RoundSettings->ParticipantPawnClass)
	{
		return RoundSettings->ParticipantPawnClass.Get();
	}

	return DefaultPawnClass;
}

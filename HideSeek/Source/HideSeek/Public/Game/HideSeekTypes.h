#pragma once

#include "CoreMinimal.h"
#include "HideSeekTypes.generated.h"

UENUM(BlueprintType)
enum class EHideSeekRole : uint8
{
	None,
	Hider,
	Seeker
};

UENUM(BlueprintType)
enum class EHideSeekPhase : uint8
{
	WaitingForPlayers,
	SelectingSeeker,
	SeekerLobbyWait,
	LoadingPlayRoom,
	HideTime,
	SeekTime,
	RoundEnded
};

UENUM(BlueprintType)
enum class EHideSeekEndReason : uint8
{
	None,
	TimeExpired,
	AllHidersFound
};

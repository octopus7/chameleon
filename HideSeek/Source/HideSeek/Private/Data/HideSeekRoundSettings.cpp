#include "Data/HideSeekRoundSettings.h"

UHideSeekRoundSettings::UHideSeekRoundSettings()
{
	PlayRoomLevel = TSoftObjectPtr<UWorld>(FSoftObjectPath(TEXT("/Game/HideSeek/Maps/L_HideSeek_PlayRoom.L_HideSeek_PlayRoom")));
	TransitionNiagaraSystem = TSoftObjectPtr<UNiagaraSystem>(FSoftObjectPath(TEXT("/Niagara/DefaultAssets/Templates/Systems/RadialBurst.RadialBurst")));
}

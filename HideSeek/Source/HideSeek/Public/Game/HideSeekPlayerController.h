#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "HideSeekPlayerController.generated.h"

class UHideSeekTransitionWidget;
class UNiagaraComponent;

UCLASS()
class HIDESEEK_API AHideSeekPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	UFUNCTION(Client, Reliable)
	void ClientSetHideSeekInputEnabled(bool bMovementEnabled, bool bLookEnabled);

	UFUNCTION(Client, Reliable)
	void ClientPlayHideSeekTransition(
		FText Message,
		float FadeInSeconds,
		float HoldSeconds,
		float FadeOutSeconds,
		FSoftClassPath WidgetClassPath,
		FSoftObjectPath NiagaraSystemPath,
		FVector NiagaraLocalOffset,
		FVector NiagaraScale,
		float NiagaraLifetimeSeconds);

private:
	void DeactivateTransitionNiagara();

	UPROPERTY(Transient)
	TObjectPtr<UHideSeekTransitionWidget> TransitionWidget;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> TransitionNiagaraComponent;

	FTimerHandle TransitionNiagaraTimerHandle;
};

#include "Game/HideSeekPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "Camera/CameraComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "TimerManager.h"
#include "UI/HideSeekTransitionWidget.h"

void AHideSeekPlayerController::ClientSetHideSeekInputEnabled_Implementation(bool bMovementEnabled, bool bLookEnabled)
{
	SetIgnoreMoveInput(!bMovementEnabled);
	SetIgnoreLookInput(!bLookEnabled);
}

void AHideSeekPlayerController::ClientPlayHideSeekTransition_Implementation(
	FText Message,
	float FadeInSeconds,
	float HoldSeconds,
	float FadeOutSeconds,
	FSoftClassPath WidgetClassPath,
	FSoftObjectPath NiagaraSystemPath,
	FVector NiagaraLocalOffset,
	FVector NiagaraScale,
	float NiagaraLifetimeSeconds)
{
	TSubclassOf<UHideSeekTransitionWidget> WidgetClass = UHideSeekTransitionWidget::StaticClass();
	if (WidgetClassPath.IsValid())
	{
		if (UClass* LoadedWidgetClass = WidgetClassPath.TryLoadClass())
		{
			if (LoadedWidgetClass->IsChildOf(UHideSeekTransitionWidget::StaticClass()))
			{
				WidgetClass = LoadedWidgetClass;
			}
		}
	}

	if (!TransitionWidget || TransitionWidget->GetClass() != WidgetClass)
	{
		TransitionWidget = CreateWidget<UHideSeekTransitionWidget>(this, WidgetClass);
	}
	if (TransitionWidget && !TransitionWidget->IsInViewport())
	{
		TransitionWidget->AddToViewport(10000);
	}
	if (TransitionWidget)
	{
		TransitionWidget->StartTransition(Message, FadeInSeconds, HoldSeconds, FadeOutSeconds);
	}

	DeactivateTransitionNiagara();
	if (NiagaraSystemPath.IsValid())
	{
		UNiagaraSystem* NiagaraSystem = Cast<UNiagaraSystem>(NiagaraSystemPath.TryLoad());
		if (NiagaraSystem)
		{
			USceneComponent* AttachComponent = nullptr;
			if (APawn* ControlledPawn = GetPawn())
			{
				AttachComponent = ControlledPawn->FindComponentByClass<UCameraComponent>();
				if (!AttachComponent)
				{
					AttachComponent = ControlledPawn->GetRootComponent();
				}
			}

			if (AttachComponent)
			{
				TransitionNiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
					NiagaraSystem,
					AttachComponent,
					NAME_None,
					NiagaraLocalOffset,
					FRotator::ZeroRotator,
					NiagaraScale,
					EAttachLocation::KeepRelativeOffset,
					true,
					ENCPoolMethod::None,
					true,
					false);
			}
			else if (PlayerCameraManager)
			{
				TransitionNiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
					GetWorld(),
					NiagaraSystem,
					PlayerCameraManager->GetCameraLocation() + PlayerCameraManager->GetCameraRotation().RotateVector(NiagaraLocalOffset),
					PlayerCameraManager->GetCameraRotation(),
					NiagaraScale,
					true,
					true,
					ENCPoolMethod::None,
					false);
			}

			if (TransitionNiagaraComponent && NiagaraLifetimeSeconds > 0.0f)
			{
				GetWorldTimerManager().SetTimer(
					TransitionNiagaraTimerHandle,
					this,
					&AHideSeekPlayerController::DeactivateTransitionNiagara,
					NiagaraLifetimeSeconds,
					false);
			}
		}
	}
}

void AHideSeekPlayerController::DeactivateTransitionNiagara()
{
	if (TransitionNiagaraTimerHandle.IsValid())
	{
		GetWorldTimerManager().ClearTimer(TransitionNiagaraTimerHandle);
	}

	if (TransitionNiagaraComponent)
	{
		TransitionNiagaraComponent->Deactivate();
		TransitionNiagaraComponent = nullptr;
	}
}

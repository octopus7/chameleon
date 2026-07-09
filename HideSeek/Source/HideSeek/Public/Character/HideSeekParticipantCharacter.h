#pragma once

#include "CoreMinimal.h"
#include "Character/ChameleonHiderCharacter.h"
#include "HideSeekParticipantCharacter.generated.h"

class UHideSeekRoleComponent;

UCLASS(BlueprintType, Blueprintable)
class HIDESEEK_API AHideSeekParticipantCharacter : public AChameleonHiderCharacter
{
	GENERATED_BODY()

public:
	AHideSeekParticipantCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HideSeek")
	TObjectPtr<UHideSeekRoleComponent> HideSeekRoleComponent;

	UFUNCTION(BlueprintCallable, Category = "HideSeek")
	void SetHideSeekMovementEnabled(bool bEnabled);
};

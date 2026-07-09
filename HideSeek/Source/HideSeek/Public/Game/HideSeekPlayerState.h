#pragma once

#include "CoreMinimal.h"
#include "Game/HideSeekTypes.h"
#include "GameFramework/PlayerState.h"
#include "HideSeekPlayerState.generated.h"

UCLASS()
class HIDESEEK_API AHideSeekPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "HideSeek")
	EHideSeekRole GetHideSeekRole() const { return HideSeekRole; }

	UFUNCTION(BlueprintPure, Category = "HideSeek")
	bool IsFound() const { return bFound; }

	UFUNCTION(BlueprintPure, Category = "HideSeek")
	int32 GetParticipantId() const { return ParticipantId; }

	void SetHideSeekRoleAuthority(EHideSeekRole NewRole);
	void SetFoundAuthority(bool bNewFound);
	void SetParticipantIdAuthority(int32 NewParticipantId);

private:
	UPROPERTY(Replicated)
	EHideSeekRole HideSeekRole = EHideSeekRole::None;

	UPROPERTY(Replicated)
	bool bFound = false;

	UPROPERTY(Replicated)
	int32 ParticipantId = INDEX_NONE;
};

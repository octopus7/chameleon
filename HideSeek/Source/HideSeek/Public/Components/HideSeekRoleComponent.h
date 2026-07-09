#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Game/HideSeekTypes.h"
#include "HideSeekRoleComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHideSeekRoleChangedSignature, EHideSeekRole, NewRole);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHideSeekFoundChangedSignature, bool, bNewFound);

UCLASS(ClassGroup = (HideSeek), meta = (BlueprintSpawnableComponent))
class HIDESEEK_API UHideSeekRoleComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHideSeekRoleComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "HideSeek")
	EHideSeekRole GetRole() const { return Role; }

	UFUNCTION(BlueprintPure, Category = "HideSeek")
	bool IsFound() const { return bFound; }

	UFUNCTION(BlueprintPure, Category = "HideSeek")
	bool IsAIControlledParticipant() const { return bAIControlledParticipant; }

	UFUNCTION(BlueprintPure, Category = "HideSeek")
	int32 GetParticipantId() const { return ParticipantId; }

	void SetRoleAuthority(EHideSeekRole NewRole);
	void SetFoundAuthority(bool bNewFound);
	void SetAIControlledParticipantAuthority(bool bNewAIControlledParticipant);
	void SetParticipantIdAuthority(int32 NewParticipantId);

	UPROPERTY(BlueprintAssignable, Category = "HideSeek")
	FHideSeekRoleChangedSignature OnRoleChanged;

	UPROPERTY(BlueprintAssignable, Category = "HideSeek")
	FHideSeekFoundChangedSignature OnFoundChanged;

private:
	UFUNCTION()
	void OnRep_Role(EHideSeekRole PreviousRole);

	UFUNCTION()
	void OnRep_Found();

	UPROPERTY(ReplicatedUsing = OnRep_Role)
	EHideSeekRole Role = EHideSeekRole::None;

	UPROPERTY(ReplicatedUsing = OnRep_Found)
	bool bFound = false;

	UPROPERTY(Replicated)
	bool bAIControlledParticipant = false;

	UPROPERTY(Replicated)
	int32 ParticipantId = INDEX_NONE;
};

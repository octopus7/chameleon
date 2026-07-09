#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "HideSeekAIController.generated.h"

UCLASS()
class HIDESEEK_API AHideSeekAIController : public AAIController
{
	GENERATED_BODY()

public:
	AHideSeekAIController();

	virtual void Tick(float DeltaSeconds) override;

private:
	void UpdateHideSeekBehavior();
	bool MoveToRandomReachablePoint(float RadiusCm);

	float NextDecisionTimeSeconds = 0.0f;
};

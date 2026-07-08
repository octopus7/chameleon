#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ChameleonHiderBodyActor.generated.h"

class UChameleonMetaballBodyComponent;
class UChameleonPaintComponent;

UCLASS()
class CHAMELEONPAINTER_API AChameleonHiderBodyActor : public AActor
{
	GENERATED_BODY()

public:
	AChameleonHiderBodyActor();

	virtual void OnConstruction(const FTransform& Transform) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Chameleon")
	TObjectPtr<UChameleonMetaballBodyComponent> BodyComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Chameleon")
	TObjectPtr<UChameleonPaintComponent> PaintComponent;
};

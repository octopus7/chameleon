#include "Actors/ChameleonHiderBodyActor.h"

#include "Components/ChameleonMetaballBodyComponent.h"
#include "Components/ChameleonPaintComponent.h"

AChameleonHiderBodyActor::AChameleonHiderBodyActor()
{
	PrimaryActorTick.bCanEverTick = false;

	BodyComponent = CreateDefaultSubobject<UChameleonMetaballBodyComponent>(TEXT("BodyComponent"));
	SetRootComponent(BodyComponent);

	PaintComponent = CreateDefaultSubobject<UChameleonPaintComponent>(TEXT("PaintComponent"));
	PaintComponent->TargetComponent = BodyComponent;
}

void AChameleonHiderBodyActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (BodyComponent)
	{
		BodyComponent->GenerateBody();
	}

	if (PaintComponent)
	{
		PaintComponent->SetTargetComponent(BodyComponent);
	}
}

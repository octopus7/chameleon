#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ChameleonPaintSprayActor.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
class UProceduralMeshComponent;

UCLASS()
class CHAMELEONPAINTER_API AChameleonPaintSprayActor : public AActor
{
	GENERATED_BODY()

public:
	AChameleonPaintSprayActor();

	virtual void Tick(float DeltaSeconds) override;

	void InitializeSpray(const FVector& SurfaceNormal, FLinearColor SprayColor, float RadiusCm, UMaterialInterface* SprayMaterial, float InLifetimeSeconds, int32 InParticleCount);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Chameleon|Paint")
	TObjectPtr<UProceduralMeshComponent> SprayMesh;

private:
	void BuildSprayMesh(const FVector& SurfaceNormal, FLinearColor SprayColor, float RadiusCm, int32 InParticleCount);

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DynamicMaterial;

	FVector DriftDirection = FVector::UpVector;
	float LifetimeSeconds = 0.35f;
	float AgeSeconds = 0.0f;
};

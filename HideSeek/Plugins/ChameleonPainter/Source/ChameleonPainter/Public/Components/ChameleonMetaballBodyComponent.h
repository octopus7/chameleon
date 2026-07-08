#pragma once

#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h"
#include "ChameleonMetaballBodyComponent.generated.h"

class UMaterialInterface;

USTRUCT(BlueprintType)
struct FChameleonProceduralBone
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Chameleon|Body|Skeleton")
	FName Name = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Chameleon|Body|Skeleton")
	int32 ParentIndex = INDEX_NONE;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Chameleon|Body|Skeleton")
	FVector RestLocationCm = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Chameleon|Body|Skeleton")
	FVector PoseLocationCm = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Chameleon|Body|Skeleton")
	FRotator PoseRotation = FRotator::ZeroRotator;
};

USTRUCT(BlueprintType)
struct FChameleonPaintStroke
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chameleon|Paint")
	FVector LocalPositionCm = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chameleon|Paint")
	FVector LocalNormal = FVector::UpVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chameleon|Paint", meta = (ClampMin = "0.1"))
	float RadiusCm = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chameleon|Paint", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Strength = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chameleon|Paint", meta = (ClampMin = "0.1"))
	float Falloff = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chameleon|Paint")
	FLinearColor Color = FLinearColor::White;
};

struct FChameleonVertexSkinWeights
{
	int32 BoneIndices[4] = { 0, 0, 0, 0 };
	float Weights[4] = { 1.0f, 0.0f, 0.0f, 0.0f };
};

UCLASS(ClassGroup = (Chameleon), meta = (BlueprintSpawnableComponent))
class CHAMELEONPAINTER_API UChameleonMetaballBodyComponent : public UProceduralMeshComponent
{
	GENERATED_BODY()

public:
	UChameleonMetaballBodyComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void OnRegister() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	UFUNCTION(BlueprintCallable, Category = "Chameleon|Body")
	void GenerateBody();

	UFUNCTION(BlueprintCallable, Category = "Chameleon|Paint")
	void SetCamouflageBaseColor(FLinearColor NewColor);

	UFUNCTION(BlueprintCallable, Category = "Chameleon|Paint")
	void ClearPaint();

	UFUNCTION(BlueprintCallable, Category = "Chameleon|Paint")
	bool ApplyPaintStrokeLocal(const FChameleonPaintStroke& Stroke);

	UFUNCTION(BlueprintCallable, Category = "Chameleon|Paint")
	bool ApplyPaintStrokeWorld(FVector WorldPosition, FVector WorldNormal, float RadiusCm, FLinearColor Color, float Strength = 1.0f);

	UFUNCTION(BlueprintCallable, Category = "Chameleon|Paint")
	bool ApplyPaintStrokeFromHit(const FHitResult& Hit, float RadiusCm, FLinearColor Color, float Strength = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Chameleon|Paint")
	FLinearColor GetCamouflageBaseColor() const { return CamouflageBaseColor; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chameleon|Body", meta = (ClampMin = "8", ClampMax = "96"))
	FIntVector GridResolution;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chameleon|Body", meta = (ClampMin = "1.0"))
	float ScaleToCentimeters = 58.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chameleon|Body")
	bool bAutoGenerateOnRegister = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chameleon|Body")
	bool bBuildQueryCollision = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chameleon|Paint")
	FLinearColor CamouflageBaseColor = FLinearColor(0.84f, 0.82f, 0.76f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chameleon|Paint")
	TArray<FName> PaintColorParameterNames;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chameleon|Material")
	TObjectPtr<UMaterialInterface> BodyMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chameleon|Body|Animation")
	bool bEnableProceduralWalkAnimation = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chameleon|Body|Animation", meta = (ClampMin = "1.0"))
	float WalkFullSpeedCmPerSecond = 450.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chameleon|Body|Animation", meta = (ClampMin = "0.1"))
	float WalkCycleRate = 7.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chameleon|Body|Animation", meta = (ClampMin = "0.0"))
	float WalkStrideAngleDegrees = 28.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chameleon|Body|Animation", meta = (ClampMin = "0.0"))
	float WalkKneeAngleDegrees = 34.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chameleon|Body|Animation", meta = (ClampMin = "0.0"))
	float WalkArmSwingAngleDegrees = 24.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chameleon|Body|Animation", meta = (ClampMin = "0.0"))
	float WalkBodyBobCm = 3.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Chameleon|Body|Skeleton", meta = (AllowPrivateAccess = "true"))
	TArray<FChameleonProceduralBone> ProceduralBones;

private:
	void ApplyMaterialPaintParameters();
	void RebuildVertexColors();
	void UpdatePaintedMeshSection();
	void BuildProceduralSkeleton();
	void BuildSkinWeights();
	void UpdateProceduralWalkAnimation(float DeltaTime);
	void ApplyProceduralSkinning();
	void BuildGlobalBoneTransforms(const TArray<FTransform>& LocalTransforms, TArray<FTransform>& OutGlobalTransforms) const;
	bool TryResolveAnimatedLocalPositionToRest(FVector AnimatedLocalPosition, FVector& OutRestPosition, FVector& OutRestNormal) const;

	TArray<FVector> CachedVertices;
	TArray<int32> CachedTriangles;
	TArray<FVector> CachedNormals;
	TArray<FVector2D> CachedUV0;
	TArray<FLinearColor> CachedVertexColors;
	TArray<FProcMeshTangent> CachedTangents;
	TArray<FVector> AnimatedVertices;
	TArray<FVector> AnimatedNormals;
	TArray<FProcMeshTangent> AnimatedTangents;
	TArray<FChameleonVertexSkinWeights> CachedSkinWeights;
	TArray<FTransform> RestLocalBoneTransforms;
	TArray<FTransform> PoseLocalBoneTransforms;
	TArray<FTransform> RestGlobalBoneTransforms;
	TArray<FTransform> PoseGlobalBoneTransforms;
	TArray<FChameleonPaintStroke> PaintStrokes;

	float WalkPhaseRadians = 0.0f;
	float WalkBlendAlpha = 0.0f;
};

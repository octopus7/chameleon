#include "Actors/ChameleonPaintSprayActor.h"

#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"

AChameleonPaintSprayActor::AChameleonPaintSprayActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	SprayMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("SprayMesh"));
	SetRootComponent(SprayMesh);
	SprayMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SprayMesh->SetCastShadow(false);
	SprayMesh->bReceivesDecals = false;
}

void AChameleonPaintSprayActor::InitializeSpray(const FVector& SurfaceNormal, FLinearColor SprayColor, float RadiusCm, UMaterialInterface* SprayMaterial, float InLifetimeSeconds, int32 InParticleCount)
{
	DriftDirection = SurfaceNormal.GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
	LifetimeSeconds = FMath::Max(InLifetimeSeconds, 0.05f);
	AgeSeconds = 0.0f;

	BuildSprayMesh(DriftDirection, SprayColor, RadiusCm, InParticleCount);

	if (SprayMaterial && SprayMesh)
	{
		DynamicMaterial = UMaterialInstanceDynamic::Create(SprayMaterial, this);
		if (DynamicMaterial)
		{
			DynamicMaterial->SetScalarParameterValue(TEXT("PaintSprayOpacity"), 1.0f);
			SprayMesh->SetMaterial(0, DynamicMaterial);
		}
		else
		{
			SprayMesh->SetMaterial(0, SprayMaterial);
		}
	}
}

void AChameleonPaintSprayActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	AgeSeconds += DeltaSeconds;
	if (AgeSeconds >= LifetimeSeconds)
	{
		Destroy();
		return;
	}

	const float NormalizedAge = FMath::Clamp(AgeSeconds / LifetimeSeconds, 0.0f, 1.0f);
	const float Opacity = FMath::Square(1.0f - NormalizedAge);
	if (DynamicMaterial)
	{
		DynamicMaterial->SetScalarParameterValue(TEXT("PaintSprayOpacity"), Opacity);
	}

	SetActorScale3D(FVector(1.0f + NormalizedAge * 0.18f));
	AddActorWorldOffset(DriftDirection * (DeltaSeconds * 5.0f), false);
}

void AChameleonPaintSprayActor::BuildSprayMesh(const FVector& SurfaceNormal, FLinearColor SprayColor, float RadiusCm, int32 InParticleCount)
{
	if (!SprayMesh)
	{
		return;
	}

	const FVector Normal = SurfaceNormal.GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
	FVector Tangent = FVector::CrossProduct(Normal, FVector::UpVector);
	if (Tangent.SizeSquared() < UE_SMALL_NUMBER)
	{
		Tangent = FVector::CrossProduct(Normal, FVector::RightVector);
	}
	Tangent.Normalize();
	const FVector Bitangent = FVector::CrossProduct(Normal, Tangent).GetSafeNormal();

	const int32 ParticleCount = FMath::Clamp(InParticleCount, 1, 128);
	const float SprayRadius = FMath::Clamp(RadiusCm, 4.0f, 120.0f);
	FRandomStream RandomStream(FMath::Rand());

	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FLinearColor> Colors;
	TArray<FProcMeshTangent> Tangents;
	Vertices.Reserve(ParticleCount * 4);
	Triangles.Reserve(ParticleCount * 6);
	Normals.Reserve(ParticleCount * 4);
	UVs.Reserve(ParticleCount * 4);
	Colors.Reserve(ParticleCount * 4);
	Tangents.Reserve(ParticleCount * 4);

	SprayColor.A = 1.0f;
	for (int32 Index = 0; Index < ParticleCount; ++Index)
	{
		const float Angle = RandomStream.FRandRange(0.0f, UE_TWO_PI);
		const float Distance = FMath::Sqrt(RandomStream.FRand()) * SprayRadius;
		const FVector Center = (Tangent * FMath::Cos(Angle) + Bitangent * FMath::Sin(Angle)) * Distance + Normal * RandomStream.FRandRange(0.6f, 2.0f);

		const float ParticleAngle = RandomStream.FRandRange(0.0f, UE_TWO_PI);
		const FVector ParticleU = (Tangent * FMath::Cos(ParticleAngle) + Bitangent * FMath::Sin(ParticleAngle)).GetSafeNormal();
		const FVector ParticleV = FVector::CrossProduct(Normal, ParticleU).GetSafeNormal();
		const float HalfSize = FMath::Clamp(SprayRadius * RandomStream.FRandRange(0.035f, 0.09f), 0.8f, 5.0f);

		const int32 BaseVertex = Vertices.Num();
		Vertices.Add(Center - ParticleU * HalfSize - ParticleV * HalfSize);
		Vertices.Add(Center + ParticleU * HalfSize - ParticleV * HalfSize);
		Vertices.Add(Center + ParticleU * HalfSize + ParticleV * HalfSize);
		Vertices.Add(Center - ParticleU * HalfSize + ParticleV * HalfSize);

		Triangles.Add(BaseVertex + 0);
		Triangles.Add(BaseVertex + 1);
		Triangles.Add(BaseVertex + 2);
		Triangles.Add(BaseVertex + 0);
		Triangles.Add(BaseVertex + 2);
		Triangles.Add(BaseVertex + 3);

		const float Alpha = RandomStream.FRandRange(0.45f, 0.95f);
		FLinearColor ParticleColor = SprayColor;
		ParticleColor.A = Alpha;
		for (int32 Corner = 0; Corner < 4; ++Corner)
		{
			Normals.Add(Normal);
			Colors.Add(ParticleColor);
			Tangents.Add(FProcMeshTangent(ParticleU, false));
		}

		UVs.Add(FVector2D(0.0f, 0.0f));
		UVs.Add(FVector2D(1.0f, 0.0f));
		UVs.Add(FVector2D(1.0f, 1.0f));
		UVs.Add(FVector2D(0.0f, 1.0f));
	}

	SprayMesh->ClearAllMeshSections();
	SprayMesh->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UVs, Colors, Tangents, false, false);
}

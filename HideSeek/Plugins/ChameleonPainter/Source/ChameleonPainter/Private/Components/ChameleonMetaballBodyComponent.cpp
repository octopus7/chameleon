#include "Components/ChameleonMetaballBodyComponent.h"

#include "Materials/MaterialInstanceDynamic.h"

namespace
{
constexpr double FieldThreshold = 0.62;

struct FBodyBlob
{
	FVector Center;
	FVector Radius;
	double Strength;
};

struct FFieldSample
{
	FVector Position;
	double Value = 0.0;
};

struct FImplicitTriangle
{
	FVector Points[3];
};

const FBodyBlob BodyBlobs[] = {
	{ FVector(0.0, 0.0, 2.58), FVector(0.62, 0.62, 0.64), 1.05 },
	{ FVector(0.0, 0.0, 2.05), FVector(0.35, 0.35, 0.35), 0.66 },
	{ FVector(0.0, 0.0, 1.48), FVector(0.58, 0.50, 0.82), 1.00 },
	{ FVector(0.03, 0.0, 1.10), FVector(0.55, 0.48, 0.52), 0.78 },
	{ FVector(0.0, 0.0, 0.82), FVector(0.50, 0.43, 0.34), 0.62 },

	{ FVector(0.0, -0.52, 1.70), FVector(0.28, 0.30, 0.34), 0.54 },
	{ FVector(0.0, -0.77, 1.35), FVector(0.25, 0.34, 0.42), 0.55 },
	{ FVector(0.0, -0.83, 0.98), FVector(0.32, 0.29, 0.28), 0.52 },
	{ FVector(0.0, 0.52, 1.70), FVector(0.28, 0.30, 0.34), 0.54 },
	{ FVector(0.0, 0.77, 1.35), FVector(0.25, 0.34, 0.42), 0.55 },
	{ FVector(0.0, 0.83, 0.98), FVector(0.32, 0.29, 0.28), 0.52 },

	{ FVector(0.0, -0.27, 0.48), FVector(0.30, 0.25, 0.54), 0.66 },
	{ FVector(0.13, -0.29, 0.18), FVector(0.42, 0.24, 0.18), 0.50 },
	{ FVector(0.0, 0.27, 0.48), FVector(0.30, 0.25, 0.54), 0.66 },
	{ FVector(0.13, 0.29, 0.18), FVector(0.42, 0.24, 0.18), 0.50 },
};

const FVector BoundsMin(-0.95, -1.18, -0.10);
const FVector BoundsMax(0.98, 1.18, 3.20);

const int32 CubeCorners[8][3] = {
	{ 0, 0, 0 },
	{ 1, 0, 0 },
	{ 1, 1, 0 },
	{ 0, 1, 0 },
	{ 0, 0, 1 },
	{ 1, 0, 1 },
	{ 1, 1, 1 },
	{ 0, 1, 1 },
};

const int32 Tetrahedra[6][4] = {
	{ 0, 5, 1, 6 },
	{ 0, 1, 2, 6 },
	{ 0, 2, 3, 6 },
	{ 0, 3, 7, 6 },
	{ 0, 7, 4, 6 },
	{ 0, 4, 5, 6 },
};

double FieldValueAt(const FVector& Position)
{
	double Value = 0.0;

	for (const FBodyBlob& Blob : BodyBlobs)
	{
		const double X = (Position.X - Blob.Center.X) / Blob.Radius.X;
		const double Y = (Position.Y - Blob.Center.Y) / Blob.Radius.Y;
		const double Z = (Position.Z - Blob.Center.Z) / Blob.Radius.Z;
		Value += Blob.Strength * FMath::Exp(-(X * X + Y * Y + Z * Z));
	}

	return Value;
}

FVector FieldGradientAt(const FVector& Position)
{
	FVector Gradient = FVector::ZeroVector;

	for (const FBodyBlob& Blob : BodyBlobs)
	{
		const FVector Delta = Position - Blob.Center;
		const double X = Delta.X / Blob.Radius.X;
		const double Y = Delta.Y / Blob.Radius.Y;
		const double Z = Delta.Z / Blob.Radius.Z;
		const double E = Blob.Strength * FMath::Exp(-(X * X + Y * Y + Z * Z));

		Gradient.X += E * -2.0 * Delta.X / (Blob.Radius.X * Blob.Radius.X);
		Gradient.Y += E * -2.0 * Delta.Y / (Blob.Radius.Y * Blob.Radius.Y);
		Gradient.Z += E * -2.0 * Delta.Z / (Blob.Radius.Z * Blob.Radius.Z);
	}

	return (-Gradient).GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
}

FVector InterpolateSample(const FFieldSample& A, const FFieldSample& B)
{
	const double Denominator = B.Value - A.Value;
	const double T = FMath::IsNearlyZero(Denominator) ? 0.5 : (FieldThreshold - A.Value) / Denominator;
	return FMath::Lerp(A.Position, B.Position, T);
}

FImplicitTriangle MakeOrientedTriangle(const FVector& A, const FVector& B, const FVector& C)
{
	FImplicitTriangle Triangle = { { A, B, C } };
	const FVector FaceNormal = FVector::CrossProduct(B - A, C - A);
	const FVector Center = (A + B + C) / 3.0;

	if (FVector::DotProduct(FaceNormal, FieldGradientAt(Center)) < 0.0)
	{
		Triangle.Points[1] = C;
		Triangle.Points[2] = B;
	}

	return Triangle;
}

void EmitTetra(const FFieldSample& A, const FFieldSample& B, const FFieldSample& C, const FFieldSample& D, TArray<FImplicitTriangle>& OutTriangles)
{
	const FFieldSample Samples[4] = { A, B, C, D };
	const FFieldSample* Inside[4] = { nullptr, nullptr, nullptr, nullptr };
	const FFieldSample* Outside[4] = { nullptr, nullptr, nullptr, nullptr };
	int32 InsideCount = 0;
	int32 OutsideCount = 0;

	for (const FFieldSample& Sample : Samples)
	{
		if (Sample.Value >= FieldThreshold)
		{
			Inside[InsideCount++] = &Sample;
		}
		else
		{
			Outside[OutsideCount++] = &Sample;
		}
	}

	if (InsideCount == 0 || InsideCount == 4)
	{
		return;
	}

	if (InsideCount == 1 || InsideCount == 3)
	{
		const FFieldSample* Pivot = InsideCount == 1 ? Inside[0] : Outside[0];
		const FFieldSample** Others = InsideCount == 1 ? Outside : Inside;
		OutTriangles.Add(MakeOrientedTriangle(
			InterpolateSample(*Pivot, *Others[0]),
			InterpolateSample(*Pivot, *Others[1]),
			InterpolateSample(*Pivot, *Others[2])));
		return;
	}

	const FVector AC = InterpolateSample(*Inside[0], *Outside[0]);
	const FVector AD = InterpolateSample(*Inside[0], *Outside[1]);
	const FVector BC = InterpolateSample(*Inside[1], *Outside[0]);
	const FVector BD = InterpolateSample(*Inside[1], *Outside[1]);

	OutTriangles.Add(MakeOrientedTriangle(AC, BC, BD));
	OutTriangles.Add(MakeOrientedTriangle(AC, BD, AD));
}

FVector PointAt(int32 X, int32 Y, int32 Z, const FIntVector& Resolution)
{
	return FVector(
		BoundsMin.X + ((BoundsMax.X - BoundsMin.X) * X) / Resolution.X,
		BoundsMin.Y + ((BoundsMax.Y - BoundsMin.Y) * Y) / Resolution.Y,
		BoundsMin.Z + ((BoundsMax.Z - BoundsMin.Z) * Z) / Resolution.Z);
}

FProcMeshTangent MakeTangentFromNormal(const FVector& Normal)
{
	FVector Basis = FVector::ForwardVector;
	if (FMath::Abs(FVector::DotProduct(Basis, Normal)) > 0.95)
	{
		Basis = FVector::RightVector;
	}

	const FVector Tangent = (Basis - Normal * FVector::DotProduct(Basis, Normal)).GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector);
	return FProcMeshTangent(Tangent, false);
}
}

UChameleonMetaballBodyComponent::UChameleonMetaballBodyComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;
	GridResolution = FIntVector(32, 40, 54);
	bUseAsyncCooking = true;
	bUseComplexAsSimpleCollision = true;
	SetCollisionEnabled(ECollisionEnabled::NoCollision);

	PaintColorParameterNames = {
		TEXT("PaintColor"),
		TEXT("BodyColor"),
		TEXT("BaseColor"),
		TEXT("Color")
	};
}

void UChameleonMetaballBodyComponent::OnRegister()
{
	Super::OnRegister();

	if (bAutoGenerateOnRegister && CachedVertices.IsEmpty())
	{
		GenerateBody();
	}
}

#if WITH_EDITOR
void UChameleonMetaballBodyComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (!IsTemplate())
	{
		GenerateBody();
	}
}
#endif

void UChameleonMetaballBodyComponent::GenerateBody()
{
	const FIntVector Resolution(
		FMath::Clamp(GridResolution.X, 8, 96),
		FMath::Clamp(GridResolution.Y, 8, 96),
		FMath::Clamp(GridResolution.Z, 8, 128));
	const double MeshScaleToCentimeters = FMath::Max(static_cast<double>(ScaleToCentimeters), 1.0);

	TArray<FImplicitTriangle> ImplicitTriangles;
	ImplicitTriangles.Reserve(Resolution.X * Resolution.Y * Resolution.Z / 2);

	for (int32 Z = 0; Z < Resolution.Z; ++Z)
	{
		for (int32 Y = 0; Y < Resolution.Y; ++Y)
		{
			for (int32 X = 0; X < Resolution.X; ++X)
			{
				FFieldSample Cube[8];
				for (int32 CornerIndex = 0; CornerIndex < 8; ++CornerIndex)
				{
					Cube[CornerIndex].Position = PointAt(
						X + CubeCorners[CornerIndex][0],
						Y + CubeCorners[CornerIndex][1],
						Z + CubeCorners[CornerIndex][2],
						Resolution);
					Cube[CornerIndex].Value = FieldValueAt(Cube[CornerIndex].Position);
				}

				for (const auto& Tetrahedron : Tetrahedra)
				{
					EmitTetra(
						Cube[Tetrahedron[0]],
						Cube[Tetrahedron[1]],
						Cube[Tetrahedron[2]],
						Cube[Tetrahedron[3]],
						ImplicitTriangles);
				}
			}
		}
	}

	double MinZ = TNumericLimits<double>::Max();
	double MaxZ = TNumericLimits<double>::Lowest();
	for (const FImplicitTriangle& Triangle : ImplicitTriangles)
	{
		for (const FVector& Point : Triangle.Points)
		{
			MinZ = FMath::Min(MinZ, Point.Z);
			MaxZ = FMath::Max(MaxZ, Point.Z);
		}
	}

	const double Height = FMath::Max(MaxZ - MinZ, 0.001);

	CachedVertices.Reset();
	CachedTriangles.Reset();
	CachedNormals.Reset();
	CachedUV0.Reset();
	CachedVertexColors.Reset();
	CachedTangents.Reset();

	CachedTriangles.Reserve(ImplicitTriangles.Num() * 3);
	TMap<FString, int32> VertexIndices;
	VertexIndices.Reserve(ImplicitTriangles.Num());

	auto AddVertex = [&](const FVector& UnitPoint) -> int32
	{
		const FVector ShiftedUnit(UnitPoint.X, UnitPoint.Y, UnitPoint.Z - MinZ);
		const FVector PositionCm = ShiftedUnit * MeshScaleToCentimeters;
		const FString Key = FString::Printf(TEXT("%.3f,%.3f,%.3f"), PositionCm.X, PositionCm.Y, PositionCm.Z);

		if (const int32* ExistingIndex = VertexIndices.Find(Key))
		{
			return *ExistingIndex;
		}

		const FVector Normal = FieldGradientAt(UnitPoint);
		const double U = FMath::Atan2(UnitPoint.Y, UnitPoint.X) / (2.0 * UE_DOUBLE_PI) + 0.5;
		const double V = (UnitPoint.Z - MinZ) / Height;
		const int32 NewIndex = CachedVertices.Num();

		VertexIndices.Add(Key, NewIndex);
		CachedVertices.Add(PositionCm);
		CachedNormals.Add(Normal);
		CachedUV0.Add(FVector2D(U, V));
		CachedVertexColors.Add(CamouflageBaseColor);
		CachedTangents.Add(MakeTangentFromNormal(Normal));
		return NewIndex;
	};

	for (const FImplicitTriangle& Triangle : ImplicitTriangles)
	{
		CachedTriangles.Add(AddVertex(Triangle.Points[0]));
		CachedTriangles.Add(AddVertex(Triangle.Points[1]));
		CachedTriangles.Add(AddVertex(Triangle.Points[2]));
	}

	RebuildVertexColors();
	ClearAllMeshSections();
	CreateMeshSection_LinearColor(0, CachedVertices, CachedTriangles, CachedNormals, CachedUV0, CachedVertexColors, CachedTangents, bBuildQueryCollision, false);

	if (BodyMaterial)
	{
		SetMaterial(0, BodyMaterial);
	}

	SetCollisionEnabled(bBuildQueryCollision ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
	if (bBuildQueryCollision)
	{
		SetCollisionResponseToAllChannels(ECR_Ignore);
		SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	}

	ApplyMaterialPaintParameters();
}

void UChameleonMetaballBodyComponent::SetCamouflageBaseColor(FLinearColor NewColor)
{
	NewColor.A = 1.0f;
	CamouflageBaseColor = NewColor;
	RebuildVertexColors();
	UpdatePaintedMeshSection();
	ApplyMaterialPaintParameters();
}

void UChameleonMetaballBodyComponent::ClearPaint()
{
	PaintStrokes.Reset();
	RebuildVertexColors();
	UpdatePaintedMeshSection();
	ApplyMaterialPaintParameters();
}

bool UChameleonMetaballBodyComponent::ApplyPaintStrokeLocal(const FChameleonPaintStroke& Stroke)
{
	if (CachedVertices.IsEmpty() || Stroke.RadiusCm <= 0.0f)
	{
		return false;
	}

	FChameleonPaintStroke SanitizedStroke = Stroke;
	SanitizedStroke.Color.A = 1.0f;
	SanitizedStroke.Strength = FMath::Clamp(SanitizedStroke.Strength, 0.0f, 1.0f);
	SanitizedStroke.Falloff = FMath::Max(SanitizedStroke.Falloff, 0.1f);
	PaintStrokes.Add(SanitizedStroke);

	RebuildVertexColors();
	UpdatePaintedMeshSection();
	return true;
}

bool UChameleonMetaballBodyComponent::ApplyPaintStrokeWorld(FVector WorldPosition, FVector WorldNormal, float RadiusCm, FLinearColor Color, float Strength)
{
	FChameleonPaintStroke Stroke;
	Stroke.LocalPositionCm = GetComponentTransform().InverseTransformPosition(WorldPosition);
	Stroke.LocalNormal = GetComponentTransform().InverseTransformVectorNoScale(WorldNormal).GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
	Stroke.RadiusCm = RadiusCm;
	Stroke.Color = Color;
	Stroke.Strength = Strength;
	return ApplyPaintStrokeLocal(Stroke);
}

bool UChameleonMetaballBodyComponent::ApplyPaintStrokeFromHit(const FHitResult& Hit, float RadiusCm, FLinearColor Color, float Strength)
{
	if (Hit.Component.Get() != this)
	{
		return false;
	}

	return ApplyPaintStrokeWorld(Hit.ImpactPoint, Hit.ImpactNormal, RadiusCm, Color, Strength);
}

void UChameleonMetaballBodyComponent::ApplyMaterialPaintParameters()
{
	UMaterialInstanceDynamic* DynamicMaterial = Cast<UMaterialInstanceDynamic>(GetMaterial(0));
	if (!DynamicMaterial)
	{
		UMaterialInterface* SourceMaterial = BodyMaterial ? BodyMaterial.Get() : GetMaterial(0);
		if (SourceMaterial)
		{
			DynamicMaterial = CreateDynamicMaterialInstance(0, SourceMaterial);
		}
	}

	if (!DynamicMaterial)
	{
		return;
	}

	for (const FName& ParameterName : PaintColorParameterNames)
	{
		if (!ParameterName.IsNone())
		{
			DynamicMaterial->SetVectorParameterValue(ParameterName, CamouflageBaseColor);
		}
	}
}

void UChameleonMetaballBodyComponent::RebuildVertexColors()
{
	CachedVertexColors.Reset(CachedVertices.Num());
	for (int32 VertexIndex = 0; VertexIndex < CachedVertices.Num(); ++VertexIndex)
	{
		FLinearColor FinalColor = CamouflageBaseColor;

		for (const FChameleonPaintStroke& Stroke : PaintStrokes)
		{
			const float Distance = FVector::Dist(CachedVertices[VertexIndex], Stroke.LocalPositionCm);
			if (Distance > Stroke.RadiusCm)
			{
				continue;
			}

			const float NormalizedDistance = Distance / Stroke.RadiusCm;
			const float Alpha = FMath::Clamp(Stroke.Strength * FMath::Pow(1.0f - NormalizedDistance, Stroke.Falloff), 0.0f, 1.0f);
			FinalColor = FLinearColor::LerpUsingHSV(FinalColor, Stroke.Color, Alpha);
			FinalColor.A = 1.0f;
		}

		CachedVertexColors.Add(FinalColor);
	}
}

void UChameleonMetaballBodyComponent::UpdatePaintedMeshSection()
{
	if (!CachedVertices.IsEmpty())
	{
		UpdateMeshSection_LinearColor(0, CachedVertices, CachedNormals, CachedUV0, CachedVertexColors, CachedTangents, false);
	}
}

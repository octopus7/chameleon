#include "Components/ChameleonMetaballBodyComponent.h"

#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
constexpr double FieldThreshold = 0.62;
constexpr int32 MaxSkinInfluences = 4;

enum class EChameleonProceduralBoneSlot : uint8
{
	Root,
	LowerTorso,
	UpperTorso,
	Neck,
	Head,
	LeftUpperArm,
	LeftLowerArm,
	RightUpperArm,
	RightLowerArm,
	LeftUpperLeg,
	LeftLowerLeg,
	RightUpperLeg,
	RightLowerLeg,
	Count
};

int32 BoneIndex(EChameleonProceduralBoneSlot Bone)
{
	return static_cast<int32>(Bone);
}

struct FBodyBlob
{
	FVector Center;
	FVector Radius;
	double Strength;
	EChameleonProceduralBoneSlot Bone;
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
	{ FVector(0.0, 0.0, 2.66), FVector(0.42, 0.42, 0.44), 1.06, EChameleonProceduralBoneSlot::Head },
	{ FVector(0.0, 0.0, 2.22), FVector(0.20, 0.24, 0.28), 0.82, EChameleonProceduralBoneSlot::Neck },
	{ FVector(0.0, 0.0, 1.72), FVector(0.43, 0.42, 0.58), 1.00, EChameleonProceduralBoneSlot::UpperTorso },
	{ FVector(0.02, 0.0, 1.18), FVector(0.42, 0.36, 0.50), 0.90, EChameleonProceduralBoneSlot::LowerTorso },
	{ FVector(0.0, 0.0, 0.78), FVector(0.42, 0.44, 0.30), 0.82, EChameleonProceduralBoneSlot::Root },

	{ FVector(0.0, -0.48, 1.88), FVector(0.22, 0.28, 0.23), 0.76, EChameleonProceduralBoneSlot::LeftUpperArm },
	{ FVector(0.0, -0.76, 1.88), FVector(0.18, 0.34, 0.18), 0.92, EChameleonProceduralBoneSlot::LeftUpperArm },
	{ FVector(0.0, -1.08, 1.88), FVector(0.18, 0.32, 0.18), 0.92, EChameleonProceduralBoneSlot::LeftLowerArm },
	{ FVector(0.0, -1.39, 1.88), FVector(0.17, 0.34, 0.17), 0.90, EChameleonProceduralBoneSlot::LeftLowerArm },
	{ FVector(0.0, -1.64, 1.88), FVector(0.17, 0.22, 0.17), 0.78, EChameleonProceduralBoneSlot::LeftLowerArm },
	{ FVector(0.0, 0.48, 1.88), FVector(0.22, 0.28, 0.23), 0.76, EChameleonProceduralBoneSlot::RightUpperArm },
	{ FVector(0.0, 0.76, 1.88), FVector(0.18, 0.34, 0.18), 0.92, EChameleonProceduralBoneSlot::RightUpperArm },
	{ FVector(0.0, 1.08, 1.88), FVector(0.18, 0.32, 0.18), 0.92, EChameleonProceduralBoneSlot::RightLowerArm },
	{ FVector(0.0, 1.39, 1.88), FVector(0.17, 0.34, 0.17), 0.90, EChameleonProceduralBoneSlot::RightLowerArm },
	{ FVector(0.0, 1.64, 1.88), FVector(0.17, 0.22, 0.17), 0.78, EChameleonProceduralBoneSlot::RightLowerArm },

	{ FVector(0.0, -0.24, 0.48), FVector(0.24, 0.22, 0.46), 0.92, EChameleonProceduralBoneSlot::LeftUpperLeg },
	{ FVector(0.0, -0.30, 0.12), FVector(0.21, 0.19, 0.38), 0.88, EChameleonProceduralBoneSlot::LeftLowerLeg },
	{ FVector(0.13, -0.31, -0.05), FVector(0.34, 0.17, 0.14), 0.72, EChameleonProceduralBoneSlot::LeftLowerLeg },
	{ FVector(0.0, 0.24, 0.48), FVector(0.24, 0.22, 0.46), 0.92, EChameleonProceduralBoneSlot::RightUpperLeg },
	{ FVector(0.0, 0.30, 0.12), FVector(0.21, 0.19, 0.38), 0.88, EChameleonProceduralBoneSlot::RightLowerLeg },
	{ FVector(0.13, 0.31, -0.05), FVector(0.34, 0.17, 0.14), 0.72, EChameleonProceduralBoneSlot::RightLowerLeg },
};

const FVector BoundsMin(-0.80, -1.88, -0.22);
const FVector BoundsMax(0.92, 1.88, 3.10);

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

double BlobValueAt(const FBodyBlob& Blob, const FVector& Position)
{
	const double X = (Position.X - Blob.Center.X) / Blob.Radius.X;
	const double Y = (Position.Y - Blob.Center.Y) / Blob.Radius.Y;
	const double Z = (Position.Z - Blob.Center.Z) / Blob.Radius.Z;
	return Blob.Strength * FMath::Exp(-(X * X + Y * Y + Z * Z));
}

double FieldValueAt(const FVector& Position)
{
	double Value = 0.0;

	for (const FBodyBlob& Blob : BodyBlobs)
	{
		Value += BlobValueAt(Blob, Position);
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
		const double E = BlobValueAt(Blob, Position);

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
	const FVector RightHandedFaceNormal = FVector::CrossProduct(B - A, C - A);
	const FVector Center = (A + B + C) / 3.0;

	// Unreal meshes use left-handed coordinates with counter-clockwise front faces.
	// Keep vertex normals outward, but wind triangles for UE's front-face convention.
	if (FVector::DotProduct(RightHandedFaceNormal, FieldGradientAt(Center)) > 0.0)
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

void ClearSkinWeights(FChameleonVertexSkinWeights& SkinWeights)
{
	for (int32 InfluenceIndex = 0; InfluenceIndex < MaxSkinInfluences; ++InfluenceIndex)
	{
		SkinWeights.BoneIndices[InfluenceIndex] = BoneIndex(EChameleonProceduralBoneSlot::Root);
		SkinWeights.Weights[InfluenceIndex] = 0.0f;
	}
}

void AddSkinWeight(FChameleonVertexSkinWeights& SkinWeights, int32 BoneIndexToAdd, float Weight)
{
	if (Weight <= UE_SMALL_NUMBER)
	{
		return;
	}

	int32 EmptyInfluenceIndex = INDEX_NONE;
	int32 LightestInfluenceIndex = 0;
	float LightestWeight = TNumericLimits<float>::Max();

	for (int32 InfluenceIndex = 0; InfluenceIndex < MaxSkinInfluences; ++InfluenceIndex)
	{
		if (SkinWeights.BoneIndices[InfluenceIndex] == BoneIndexToAdd)
		{
			SkinWeights.Weights[InfluenceIndex] += Weight;
			return;
		}

		if (SkinWeights.Weights[InfluenceIndex] <= UE_SMALL_NUMBER && EmptyInfluenceIndex == INDEX_NONE)
		{
			EmptyInfluenceIndex = InfluenceIndex;
		}

		if (SkinWeights.Weights[InfluenceIndex] < LightestWeight)
		{
			LightestWeight = SkinWeights.Weights[InfluenceIndex];
			LightestInfluenceIndex = InfluenceIndex;
		}
	}

	const int32 InfluenceIndex = EmptyInfluenceIndex != INDEX_NONE ? EmptyInfluenceIndex : LightestInfluenceIndex;
	if (EmptyInfluenceIndex != INDEX_NONE || Weight > SkinWeights.Weights[InfluenceIndex])
	{
		SkinWeights.BoneIndices[InfluenceIndex] = BoneIndexToAdd;
		SkinWeights.Weights[InfluenceIndex] = Weight;
	}
}

void NormalizeSkinWeights(FChameleonVertexSkinWeights& SkinWeights)
{
	float TotalWeight = 0.0f;
	for (int32 InfluenceIndex = 0; InfluenceIndex < MaxSkinInfluences; ++InfluenceIndex)
	{
		TotalWeight += SkinWeights.Weights[InfluenceIndex];
	}

	if (TotalWeight <= UE_SMALL_NUMBER)
	{
		ClearSkinWeights(SkinWeights);
		SkinWeights.BoneIndices[0] = BoneIndex(EChameleonProceduralBoneSlot::Root);
		SkinWeights.Weights[0] = 1.0f;
		return;
	}

	for (int32 InfluenceIndex = 0; InfluenceIndex < MaxSkinInfluences; ++InfluenceIndex)
	{
		SkinWeights.Weights[InfluenceIndex] /= TotalWeight;
	}
}

float SmoothStep01(float Value)
{
	const float ClampedValue = FMath::Clamp(Value, 0.0f, 1.0f);
	return ClampedValue * ClampedValue * (3.0f - 2.0f * ClampedValue);
}

bool IsLeftLimbBone(EChameleonProceduralBoneSlot Bone)
{
	return Bone == EChameleonProceduralBoneSlot::LeftUpperArm
		|| Bone == EChameleonProceduralBoneSlot::LeftLowerArm
		|| Bone == EChameleonProceduralBoneSlot::LeftUpperLeg
		|| Bone == EChameleonProceduralBoneSlot::LeftLowerLeg;
}

bool IsRightLimbBone(EChameleonProceduralBoneSlot Bone)
{
	return Bone == EChameleonProceduralBoneSlot::RightUpperArm
		|| Bone == EChameleonProceduralBoneSlot::RightLowerArm
		|| Bone == EChameleonProceduralBoneSlot::RightUpperLeg
		|| Bone == EChameleonProceduralBoneSlot::RightLowerLeg;
}

bool IsArmBone(EChameleonProceduralBoneSlot Bone)
{
	return Bone == EChameleonProceduralBoneSlot::LeftUpperArm
		|| Bone == EChameleonProceduralBoneSlot::LeftLowerArm
		|| Bone == EChameleonProceduralBoneSlot::RightUpperArm
		|| Bone == EChameleonProceduralBoneSlot::RightLowerArm;
}

bool IsLegBone(EChameleonProceduralBoneSlot Bone)
{
	return Bone == EChameleonProceduralBoneSlot::LeftUpperLeg
		|| Bone == EChameleonProceduralBoneSlot::LeftLowerLeg
		|| Bone == EChameleonProceduralBoneSlot::RightUpperLeg
		|| Bone == EChameleonProceduralBoneSlot::RightLowerLeg;
}

EChameleonProceduralBoneSlot GetAttachmentBone(EChameleonProceduralBoneSlot Bone)
{
	if (IsArmBone(Bone))
	{
		return EChameleonProceduralBoneSlot::UpperTorso;
	}

	if (IsLegBone(Bone))
	{
		return EChameleonProceduralBoneSlot::Root;
	}

	return Bone;
}

float GetSameSideGate(EChameleonProceduralBoneSlot Bone, const FVector& UnitPoint)
{
	const float Side = static_cast<float>(UnitPoint.Y);
	const float AbsSide = FMath::Abs(Side);

	if ((IsLeftLimbBone(Bone) && Side > -0.015f) || (IsRightLimbBone(Bone) && Side < 0.015f))
	{
		return 0.0f;
	}

	if (IsArmBone(Bone))
	{
		return SmoothStep01((AbsSide - 0.25f) / 0.20f);
	}

	if (IsLegBone(Bone))
	{
		return SmoothStep01((AbsSide - 0.08f) / 0.16f);
	}

	return 1.0f;
}

FChameleonVertexSkinWeights MakeSkinWeightsForFieldPoint(const FVector& UnitPoint)
{
	FChameleonVertexSkinWeights SkinWeights;
	ClearSkinWeights(SkinWeights);

	for (const FBodyBlob& Blob : BodyBlobs)
	{
		const float Gate = GetSameSideGate(Blob.Bone, UnitPoint);
		if (Gate <= UE_SMALL_NUMBER)
		{
			continue;
		}

		const float Contribution = static_cast<float>(BlobValueAt(Blob, UnitPoint)) * Gate;
		if (Contribution <= UE_SMALL_NUMBER)
		{
			continue;
		}

		if (IsArmBone(Blob.Bone) || IsLegBone(Blob.Bone))
		{
			const float AbsSide = FMath::Abs(static_cast<float>(UnitPoint.Y));
			const float JointBlend = IsArmBone(Blob.Bone)
				? 0.38f * (1.0f - SmoothStep01((AbsSide - 0.36f) / 0.24f))
				: 0.42f * (1.0f - SmoothStep01((AbsSide - 0.16f) / 0.16f));
			const float ClampedJointBlend = FMath::Clamp(JointBlend, 0.0f, 0.75f);
			AddSkinWeight(SkinWeights, BoneIndex(Blob.Bone), Contribution * (1.0f - ClampedJointBlend));
			AddSkinWeight(SkinWeights, BoneIndex(GetAttachmentBone(Blob.Bone)), Contribution * ClampedJointBlend);
		}
		else
		{
			AddSkinWeight(SkinWeights, BoneIndex(Blob.Bone), Contribution);
		}
	}

	NormalizeSkinWeights(SkinWeights);
	return SkinWeights;
}
}

UChameleonMetaballBodyComponent::UChameleonMetaballBodyComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	GridResolution = FIntVector(32, 78, 56);
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

	SetComponentTickEnabled(bEnableProceduralWalkAnimation);
}

void UChameleonMetaballBodyComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (CachedVertices.IsEmpty() || CachedSkinWeights.Num() != CachedVertices.Num())
	{
		return;
	}

	UpdateProceduralWalkAnimation(DeltaTime);
	ApplyProceduralSkinning();
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
	AnimatedVertices.Reset();
	AnimatedNormals.Reset();
	AnimatedTangents.Reset();
	CachedSkinWeights.Reset();

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
		CachedSkinWeights.Add(MakeSkinWeightsForFieldPoint(UnitPoint));
		return NewIndex;
	};

	for (const FImplicitTriangle& Triangle : ImplicitTriangles)
	{
		CachedTriangles.Add(AddVertex(Triangle.Points[0]));
		CachedTriangles.Add(AddVertex(Triangle.Points[1]));
		CachedTriangles.Add(AddVertex(Triangle.Points[2]));
	}

	AnimatedVertices = CachedVertices;
	AnimatedNormals = CachedNormals;
	AnimatedTangents = CachedTangents;
	WalkPhaseRadians = 0.0f;
	WalkBlendAlpha = 0.0f;
	BuildProceduralSkeleton();
	BuildSkinWeights();
	RebuildVertexColors();
	ClearAllMeshSections();
	CreateMeshSection_LinearColor(0, AnimatedVertices, CachedTriangles, AnimatedNormals, CachedUV0, CachedVertexColors, AnimatedTangents, bBuildQueryCollision, false);

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
	const FTransform& ComponentTransform = GetComponentTransform();
	const FVector AnimatedLocalPosition = ComponentTransform.InverseTransformPosition(WorldPosition);
	const FVector AnimatedLocalNormal = ComponentTransform.InverseTransformVectorNoScale(WorldNormal).GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
	FVector RestPosition = AnimatedLocalPosition;
	FVector RestNormal = AnimatedLocalNormal;
	TryResolveAnimatedLocalPositionToRest(AnimatedLocalPosition, RestPosition, RestNormal);

	FChameleonPaintStroke Stroke;
	Stroke.LocalPositionCm = RestPosition;
	Stroke.LocalNormal = RestNormal;
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

void UChameleonMetaballBodyComponent::BuildProceduralSkeleton()
{
	ProceduralBones.Reset();
	RestLocalBoneTransforms.Reset();
	PoseLocalBoneTransforms.Reset();
	RestGlobalBoneTransforms.Reset();
	PoseGlobalBoneTransforms.Reset();

	if (CachedVertices.IsEmpty())
	{
		return;
	}

	FBox MeshBounds(ForceInit);
	for (const FVector& Vertex : CachedVertices)
	{
		MeshBounds += Vertex;
	}

	const FVector BoundsCenter = MeshBounds.GetCenter();
	const FVector BoundsExtent = MeshBounds.GetExtent();
	const float Height = FMath::Max(MeshBounds.GetSize().Z, 1.0f);
	const float SideExtent = FMath::Max(BoundsExtent.Y, 1.0f);
	const float MinZ = MeshBounds.Min.Z;
	const auto ZAt = [MinZ, Height](float Ratio)
	{
		return MinZ + Height * Ratio;
	};

	const int32 BoneCount = BoneIndex(EChameleonProceduralBoneSlot::Count);
	ProceduralBones.SetNum(BoneCount);
	RestLocalBoneTransforms.SetNum(BoneCount);
	PoseLocalBoneTransforms.SetNum(BoneCount);
	RestGlobalBoneTransforms.SetNum(BoneCount);
	PoseGlobalBoneTransforms.SetNum(BoneCount);

	const auto SetBone = [this](EChameleonProceduralBoneSlot Bone, const TCHAR* Name, EChameleonProceduralBoneSlot Parent, FVector RestLocation)
	{
		const int32 Index = BoneIndex(Bone);
		const int32 ParentIndex = Parent == EChameleonProceduralBoneSlot::Count ? INDEX_NONE : BoneIndex(Parent);
		ProceduralBones[Index].Name = FName(Name);
		ProceduralBones[Index].ParentIndex = ParentIndex;
		ProceduralBones[Index].RestLocationCm = RestLocation;
		ProceduralBones[Index].PoseLocationCm = RestLocation;
		ProceduralBones[Index].PoseRotation = FRotator::ZeroRotator;

		const FVector ParentLocation = ParentIndex == INDEX_NONE ? FVector::ZeroVector : ProceduralBones[ParentIndex].RestLocationCm;
		const FVector LocalTranslation = ParentIndex == INDEX_NONE ? RestLocation : RestLocation - ParentLocation;
		RestLocalBoneTransforms[Index] = FTransform(FQuat::Identity, LocalTranslation);
		PoseLocalBoneTransforms[Index] = RestLocalBoneTransforms[Index];
	};

	const FVector CenterLine(BoundsCenter.X, BoundsCenter.Y, 0.0f);
	const float LeftY = BoundsCenter.Y - SideExtent * 0.32f;
	const float LeftElbowY = BoundsCenter.Y - SideExtent * 0.66f;
	const float RightY = BoundsCenter.Y + SideExtent * 0.32f;
	const float RightElbowY = BoundsCenter.Y + SideExtent * 0.66f;
	const float LeftHipY = BoundsCenter.Y - SideExtent * 0.24f;
	const float LeftKneeY = BoundsCenter.Y - SideExtent * 0.28f;
	const float RightHipY = BoundsCenter.Y + SideExtent * 0.24f;
	const float RightKneeY = BoundsCenter.Y + SideExtent * 0.28f;

	SetBone(EChameleonProceduralBoneSlot::Root, TEXT("Root"), EChameleonProceduralBoneSlot::Count, FVector(CenterLine.X, CenterLine.Y, ZAt(0.32f)));
	SetBone(EChameleonProceduralBoneSlot::LowerTorso, TEXT("LowerTorso"), EChameleonProceduralBoneSlot::Root, FVector(CenterLine.X, CenterLine.Y, ZAt(0.46f)));
	SetBone(EChameleonProceduralBoneSlot::UpperTorso, TEXT("UpperTorso"), EChameleonProceduralBoneSlot::LowerTorso, FVector(CenterLine.X, CenterLine.Y, ZAt(0.64f)));
	SetBone(EChameleonProceduralBoneSlot::Neck, TEXT("Neck"), EChameleonProceduralBoneSlot::UpperTorso, FVector(CenterLine.X, CenterLine.Y, ZAt(0.78f)));
	SetBone(EChameleonProceduralBoneSlot::Head, TEXT("Head"), EChameleonProceduralBoneSlot::Neck, FVector(CenterLine.X, CenterLine.Y, ZAt(0.90f)));

	SetBone(EChameleonProceduralBoneSlot::LeftUpperArm, TEXT("LeftUpperArm"), EChameleonProceduralBoneSlot::UpperTorso, FVector(CenterLine.X, LeftY, ZAt(0.64f)));
	SetBone(EChameleonProceduralBoneSlot::LeftLowerArm, TEXT("LeftLowerArm"), EChameleonProceduralBoneSlot::LeftUpperArm, FVector(CenterLine.X, LeftElbowY, ZAt(0.64f)));
	SetBone(EChameleonProceduralBoneSlot::RightUpperArm, TEXT("RightUpperArm"), EChameleonProceduralBoneSlot::UpperTorso, FVector(CenterLine.X, RightY, ZAt(0.64f)));
	SetBone(EChameleonProceduralBoneSlot::RightLowerArm, TEXT("RightLowerArm"), EChameleonProceduralBoneSlot::RightUpperArm, FVector(CenterLine.X, RightElbowY, ZAt(0.64f)));

	SetBone(EChameleonProceduralBoneSlot::LeftUpperLeg, TEXT("LeftUpperLeg"), EChameleonProceduralBoneSlot::Root, FVector(CenterLine.X, LeftHipY, ZAt(0.30f)));
	SetBone(EChameleonProceduralBoneSlot::LeftLowerLeg, TEXT("LeftLowerLeg"), EChameleonProceduralBoneSlot::LeftUpperLeg, FVector(CenterLine.X, LeftKneeY, ZAt(0.12f)));
	SetBone(EChameleonProceduralBoneSlot::RightUpperLeg, TEXT("RightUpperLeg"), EChameleonProceduralBoneSlot::Root, FVector(CenterLine.X, RightHipY, ZAt(0.30f)));
	SetBone(EChameleonProceduralBoneSlot::RightLowerLeg, TEXT("RightLowerLeg"), EChameleonProceduralBoneSlot::RightUpperLeg, FVector(CenterLine.X, RightKneeY, ZAt(0.12f)));

	BuildGlobalBoneTransforms(RestLocalBoneTransforms, RestGlobalBoneTransforms);
	BuildGlobalBoneTransforms(PoseLocalBoneTransforms, PoseGlobalBoneTransforms);
}

void UChameleonMetaballBodyComponent::BuildSkinWeights()
{
	if (CachedSkinWeights.Num() == CachedVertices.Num())
	{
		return;
	}

	CachedSkinWeights.Reset(CachedVertices.Num());
	if (CachedVertices.IsEmpty())
	{
		return;
	}

	FBox MeshBounds(ForceInit);
	for (const FVector& Vertex : CachedVertices)
	{
		MeshBounds += Vertex;
	}

	const FVector BoundsCenter = MeshBounds.GetCenter();
	const FVector BoundsExtent = MeshBounds.GetExtent();
	const float Height = FMath::Max(MeshBounds.GetSize().Z, 1.0f);
	const float SideExtent = FMath::Max(BoundsExtent.Y, 1.0f);
	const float MinZ = MeshBounds.Min.Z;

	CachedSkinWeights.SetNum(CachedVertices.Num());
	for (int32 VertexIndex = 0; VertexIndex < CachedVertices.Num(); ++VertexIndex)
	{
		FChameleonVertexSkinWeights& SkinWeights = CachedSkinWeights[VertexIndex];
		ClearSkinWeights(SkinWeights);

		const FVector& Vertex = CachedVertices[VertexIndex];
		const float NormalizedZ = FMath::Clamp((Vertex.Z - MinZ) / Height, 0.0f, 1.0f);
		const float SideAmount = FMath::Abs(Vertex.Y - BoundsCenter.Y) / SideExtent;
		const bool bLeftSide = Vertex.Y < BoundsCenter.Y;

		if (NormalizedZ >= 0.84f)
		{
			const float HeadWeight = SmoothStep01((NormalizedZ - 0.84f) / 0.10f);
			AddSkinWeight(SkinWeights, BoneIndex(EChameleonProceduralBoneSlot::Neck), 1.0f - HeadWeight);
			AddSkinWeight(SkinWeights, BoneIndex(EChameleonProceduralBoneSlot::Head), HeadWeight);
		}
		else if (NormalizedZ >= 0.76f)
		{
			const float NeckWeight = SmoothStep01((NormalizedZ - 0.76f) / 0.08f);
			AddSkinWeight(SkinWeights, BoneIndex(EChameleonProceduralBoneSlot::UpperTorso), 1.0f - NeckWeight);
			AddSkinWeight(SkinWeights, BoneIndex(EChameleonProceduralBoneSlot::Neck), NeckWeight);
		}
		else if (NormalizedZ > 0.52f && NormalizedZ < 0.74f && SideAmount > 0.20f)
		{
			const float LowerArmWeight = SmoothStep01((SideAmount - 0.42f) / 0.42f);
			const int32 UpperArmBone = BoneIndex(bLeftSide ? EChameleonProceduralBoneSlot::LeftUpperArm : EChameleonProceduralBoneSlot::RightUpperArm);
			const int32 LowerArmBone = BoneIndex(bLeftSide ? EChameleonProceduralBoneSlot::LeftLowerArm : EChameleonProceduralBoneSlot::RightLowerArm);
			AddSkinWeight(SkinWeights, UpperArmBone, 1.0f - LowerArmWeight);
			AddSkinWeight(SkinWeights, LowerArmBone, LowerArmWeight);
			AddSkinWeight(SkinWeights, BoneIndex(EChameleonProceduralBoneSlot::UpperTorso), 0.20f * (1.0f - SmoothStep01((SideAmount - 0.20f) / 0.20f)));
		}
		else if (NormalizedZ <= 0.38f && SideAmount > 0.08f)
		{
			const float LowerLegWeight = SmoothStep01((0.26f - NormalizedZ) / 0.18f);
			const int32 UpperLegBone = BoneIndex(bLeftSide ? EChameleonProceduralBoneSlot::LeftUpperLeg : EChameleonProceduralBoneSlot::RightUpperLeg);
			const int32 LowerLegBone = BoneIndex(bLeftSide ? EChameleonProceduralBoneSlot::LeftLowerLeg : EChameleonProceduralBoneSlot::RightLowerLeg);
			AddSkinWeight(SkinWeights, UpperLegBone, 1.0f - LowerLegWeight);
			AddSkinWeight(SkinWeights, LowerLegBone, LowerLegWeight);
			AddSkinWeight(SkinWeights, BoneIndex(EChameleonProceduralBoneSlot::Root), 0.12f * (1.0f - SmoothStep01((SideAmount - 0.08f) / 0.14f)));
		}
		else
		{
			const float UpperTorsoWeight = SmoothStep01((NormalizedZ - 0.48f) / 0.22f);
			AddSkinWeight(SkinWeights, BoneIndex(EChameleonProceduralBoneSlot::LowerTorso), 1.0f - UpperTorsoWeight);
			AddSkinWeight(SkinWeights, BoneIndex(EChameleonProceduralBoneSlot::UpperTorso), UpperTorsoWeight);
		}

		NormalizeSkinWeights(SkinWeights);
	}
}

void UChameleonMetaballBodyComponent::UpdateProceduralWalkAnimation(float DeltaTime)
{
	if (ProceduralBones.Num() != BoneIndex(EChameleonProceduralBoneSlot::Count) || RestLocalBoneTransforms.Num() != ProceduralBones.Num())
	{
		return;
	}

	const AActor* Owner = GetOwner();
	const float Speed = Owner ? Owner->GetVelocity().Size2D() : 0.0f;
	const float TargetWalkBlend = bEnableProceduralWalkAnimation
		? FMath::Clamp(Speed / FMath::Max(WalkFullSpeedCmPerSecond, 1.0f), 0.0f, 1.0f)
		: 0.0f;

	WalkBlendAlpha = FMath::FInterpTo(WalkBlendAlpha, TargetWalkBlend, DeltaTime, 8.0f);
	if (TargetWalkBlend > 0.01f || WalkBlendAlpha > 0.01f)
	{
		const float PhaseSpeed = WalkCycleRate * FMath::Lerp(0.55f, 1.35f, FMath::Clamp(TargetWalkBlend, 0.0f, 1.0f));
		WalkPhaseRadians = FMath::Fmod(WalkPhaseRadians + DeltaTime * PhaseSpeed, static_cast<float>(UE_DOUBLE_PI));
	}

	PoseLocalBoneTransforms = RestLocalBoneTransforms;

	const float PhaseSin = FMath::Sin(WalkPhaseRadians);
	const float PhaseCos = FMath::Cos(WalkPhaseRadians);
	const float DoublePhaseSin = FMath::Sin(WalkPhaseRadians * 2.0f);
	const float DoublePhaseAbs = FMath::Abs(DoublePhaseSin);
	const float Blend = WalkBlendAlpha;
	const float StrideAngle = WalkStrideAngleDegrees * Blend;
	const float ArmAngle = WalkArmSwingAngleDegrees * Blend;
	const float KneeAngle = WalkKneeAngleDegrees * Blend;
	const float BodyRoll = 4.0f * DoublePhaseSin * Blend;
	const float BodyBob = WalkBodyBobCm * DoublePhaseAbs * Blend;
	const float BodySway = 2.0f * PhaseCos * Blend;

	const auto SetPose = [this](EChameleonProceduralBoneSlot Bone, const FRotator& Rotation, const FVector& TranslationOffset = FVector::ZeroVector)
	{
		const int32 Index = BoneIndex(Bone);
		const FVector RestTranslation = RestLocalBoneTransforms[Index].GetLocation();
		PoseLocalBoneTransforms[Index] = FTransform(Rotation.Quaternion(), RestTranslation + TranslationOffset);
		ProceduralBones[Index].PoseRotation = Rotation;
	};

	SetPose(EChameleonProceduralBoneSlot::Root, FRotator(-2.0f * Blend, 0.0f, BodyRoll * 0.20f), FVector(0.0f, BodySway, BodyBob));
	SetPose(EChameleonProceduralBoneSlot::LowerTorso, FRotator(-1.5f * Blend, 0.0f, -BodyRoll * 0.35f));
	SetPose(EChameleonProceduralBoneSlot::UpperTorso, FRotator(-3.0f * Blend, 0.0f, BodyRoll * 0.75f));
	SetPose(EChameleonProceduralBoneSlot::Neck, FRotator(2.0f * Blend, 0.0f, -BodyRoll * 0.55f));
	SetPose(EChameleonProceduralBoneSlot::Head, FRotator(0.5f * Blend, 0.0f, -BodyRoll * 0.45f));

	const float LeftLegSwing = PhaseSin * StrideAngle;
	const float RightLegSwing = -PhaseSin * StrideAngle;
	const float LeftKneeBend = FMath::Max(0.0f, -PhaseSin) * KneeAngle;
	const float RightKneeBend = FMath::Max(0.0f, PhaseSin) * KneeAngle;
	SetPose(EChameleonProceduralBoneSlot::LeftUpperLeg, FRotator(LeftLegSwing, 0.0f, -3.0f * Blend));
	SetPose(EChameleonProceduralBoneSlot::LeftLowerLeg, FRotator(-LeftKneeBend, 0.0f, 0.0f));
	SetPose(EChameleonProceduralBoneSlot::RightUpperLeg, FRotator(RightLegSwing, 0.0f, 3.0f * Blend));
	SetPose(EChameleonProceduralBoneSlot::RightLowerLeg, FRotator(-RightKneeBend, 0.0f, 0.0f));

	SetPose(EChameleonProceduralBoneSlot::LeftUpperArm, FRotator(-PhaseSin * ArmAngle, 0.0f, -6.0f * Blend));
	SetPose(EChameleonProceduralBoneSlot::LeftLowerArm, FRotator(FMath::Max(0.0f, PhaseSin) * ArmAngle * 0.45f, 0.0f, 0.0f));
	SetPose(EChameleonProceduralBoneSlot::RightUpperArm, FRotator(PhaseSin * ArmAngle, 0.0f, 6.0f * Blend));
	SetPose(EChameleonProceduralBoneSlot::RightLowerArm, FRotator(FMath::Max(0.0f, -PhaseSin) * ArmAngle * 0.45f, 0.0f, 0.0f));

	BuildGlobalBoneTransforms(PoseLocalBoneTransforms, PoseGlobalBoneTransforms);
	for (int32 BoneIndexValue = 0; BoneIndexValue < ProceduralBones.Num(); ++BoneIndexValue)
	{
		ProceduralBones[BoneIndexValue].PoseLocationCm = PoseGlobalBoneTransforms[BoneIndexValue].GetLocation();
	}
}

void UChameleonMetaballBodyComponent::ApplyProceduralSkinning()
{
	if (CachedVertices.IsEmpty() || CachedSkinWeights.Num() != CachedVertices.Num() || PoseGlobalBoneTransforms.Num() != ProceduralBones.Num())
	{
		return;
	}

	AnimatedVertices.SetNum(CachedVertices.Num());
	AnimatedNormals.SetNum(CachedNormals.Num());
	AnimatedTangents.SetNum(CachedTangents.Num());

	for (int32 VertexIndex = 0; VertexIndex < CachedVertices.Num(); ++VertexIndex)
	{
		const FVector& RestPosition = CachedVertices[VertexIndex];
		const FVector& RestNormal = CachedNormals.IsValidIndex(VertexIndex) ? CachedNormals[VertexIndex] : FVector::UpVector;
		const FChameleonVertexSkinWeights& SkinWeights = CachedSkinWeights[VertexIndex];

		FVector SkinnedPosition = FVector::ZeroVector;
		FVector SkinnedNormal = FVector::ZeroVector;
		for (int32 InfluenceIndex = 0; InfluenceIndex < MaxSkinInfluences; ++InfluenceIndex)
		{
			const float Weight = SkinWeights.Weights[InfluenceIndex];
			const int32 BoneIndexValue = SkinWeights.BoneIndices[InfluenceIndex];
			if (Weight <= UE_SMALL_NUMBER || !RestGlobalBoneTransforms.IsValidIndex(BoneIndexValue) || !PoseGlobalBoneTransforms.IsValidIndex(BoneIndexValue))
			{
				continue;
			}

			const FVector BoneLocalPosition = RestGlobalBoneTransforms[BoneIndexValue].InverseTransformPosition(RestPosition);
			const FVector BoneLocalNormal = RestGlobalBoneTransforms[BoneIndexValue].InverseTransformVectorNoScale(RestNormal);
			SkinnedPosition += PoseGlobalBoneTransforms[BoneIndexValue].TransformPosition(BoneLocalPosition) * Weight;
			SkinnedNormal += PoseGlobalBoneTransforms[BoneIndexValue].TransformVectorNoScale(BoneLocalNormal) * Weight;
		}

		AnimatedVertices[VertexIndex] = SkinnedPosition;
		AnimatedNormals[VertexIndex] = SkinnedNormal.GetSafeNormal(UE_SMALL_NUMBER, RestNormal);
		AnimatedTangents[VertexIndex] = MakeTangentFromNormal(AnimatedNormals[VertexIndex]);
	}

	UpdatePaintedMeshSection();
}

void UChameleonMetaballBodyComponent::BuildGlobalBoneTransforms(const TArray<FTransform>& LocalTransforms, TArray<FTransform>& OutGlobalTransforms) const
{
	OutGlobalTransforms.SetNum(LocalTransforms.Num());
	for (int32 BoneIndexValue = 0; BoneIndexValue < LocalTransforms.Num(); ++BoneIndexValue)
	{
		const int32 ParentIndex = ProceduralBones.IsValidIndex(BoneIndexValue) ? ProceduralBones[BoneIndexValue].ParentIndex : INDEX_NONE;
		OutGlobalTransforms[BoneIndexValue] = ParentIndex == INDEX_NONE || !OutGlobalTransforms.IsValidIndex(ParentIndex)
			? LocalTransforms[BoneIndexValue]
			: LocalTransforms[BoneIndexValue] * OutGlobalTransforms[ParentIndex];
	}
}

bool UChameleonMetaballBodyComponent::TryResolveAnimatedLocalPositionToRest(FVector AnimatedLocalPosition, FVector& OutRestPosition, FVector& OutRestNormal) const
{
	if (AnimatedVertices.Num() != CachedVertices.Num() || AnimatedVertices.IsEmpty())
	{
		return false;
	}

	int32 BestVertexIndex = INDEX_NONE;
	double BestDistanceSquared = TNumericLimits<double>::Max();
	for (int32 VertexIndex = 0; VertexIndex < AnimatedVertices.Num(); ++VertexIndex)
	{
		const double DistanceSquared = FVector::DistSquared(AnimatedVertices[VertexIndex], AnimatedLocalPosition);
		if (DistanceSquared < BestDistanceSquared)
		{
			BestDistanceSquared = DistanceSquared;
			BestVertexIndex = VertexIndex;
		}
	}

	if (BestVertexIndex == INDEX_NONE)
	{
		return false;
	}

	OutRestPosition = CachedVertices[BestVertexIndex];
	OutRestNormal = CachedNormals.IsValidIndex(BestVertexIndex) ? CachedNormals[BestVertexIndex] : FVector::UpVector;
	return true;
}

void UChameleonMetaballBodyComponent::UpdatePaintedMeshSection()
{
	if (!CachedVertices.IsEmpty())
	{
		const TArray<FVector>& VerticesToUse = AnimatedVertices.Num() == CachedVertices.Num() ? AnimatedVertices : CachedVertices;
		const TArray<FVector>& NormalsToUse = AnimatedNormals.Num() == CachedNormals.Num() ? AnimatedNormals : CachedNormals;
		const TArray<FProcMeshTangent>& TangentsToUse = AnimatedTangents.Num() == CachedTangents.Num() ? AnimatedTangents : CachedTangents;
		UpdateMeshSection_LinearColor(0, VerticesToUse, NormalsToUse, CachedUV0, CachedVertexColors, TangentsToUse, false);
	}
}

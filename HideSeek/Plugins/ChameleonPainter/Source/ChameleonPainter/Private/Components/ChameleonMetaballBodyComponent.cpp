#include "Components/ChameleonMetaballBodyComponent.h"

#include "Engine/Texture2D.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TextureResource.h"

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

enum class EChameleonUvIsland : uint8
{
	HeadFront,
	HeadBack,
	HeadLeft,
	HeadRight,
	TorsoFront,
	TorsoBack,
	TorsoLeft,
	TorsoRight,
	LeftArmFront,
	LeftArmBack,
	LeftArmTop,
	LeftArmBottom,
	RightArmFront,
	RightArmBack,
	RightArmTop,
	RightArmBottom,
	LeftLegFront,
	LeftLegBack,
	LeftLegOuter,
	LeftLegInner,
	RightLegFront,
	RightLegBack,
	RightLegOuter,
	RightLegInner,
	Count
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

EChameleonProceduralBoneSlot GetDominantBoneForFieldPoint(const FVector& UnitPoint)
{
	EChameleonProceduralBoneSlot DominantBone = EChameleonProceduralBoneSlot::Root;
	float BestContribution = 0.0f;

	for (const FBodyBlob& Blob : BodyBlobs)
	{
		const float Gate = GetSameSideGate(Blob.Bone, UnitPoint);
		if (Gate <= UE_SMALL_NUMBER)
		{
			continue;
		}

		const float Contribution = static_cast<float>(BlobValueAt(Blob, UnitPoint)) * Gate;
		if (Contribution > BestContribution)
		{
			BestContribution = Contribution;
			DominantBone = Blob.Bone;
		}
	}

	return DominantBone;
}

float NormalizeUvRange(double Value, double Min, double Max)
{
	const double Denominator = FMath::Max(Max - Min, 0.001);
	return FMath::Clamp(static_cast<float>((Value - Min) / Denominator), 0.0f, 1.0f);
}

FVector2D PackUvIsland(EChameleonUvIsland Island, const FVector2D& LocalUv)
{
	constexpr int32 Columns = 6;
	constexpr int32 Rows = 4;
	constexpr float MarginU = 0.010f;
	constexpr float MarginV = 0.014f;

	const int32 IslandIndex = FMath::Clamp(static_cast<int32>(Island), 0, static_cast<int32>(EChameleonUvIsland::Count) - 1);
	const int32 Column = IslandIndex % Columns;
	const int32 Row = IslandIndex / Columns;
	const FVector2D CellSize(1.0f / static_cast<float>(Columns), 1.0f / static_cast<float>(Rows));
	const FVector2D CellMin(CellSize.X * static_cast<float>(Column), CellSize.Y * static_cast<float>(Row));
	const FVector2D InnerMin = CellMin + FVector2D(MarginU, MarginV);
	const FVector2D InnerSize = CellSize - FVector2D(MarginU * 2.0f, MarginV * 2.0f);
	const FVector2D ClampedUv(FMath::Clamp(LocalUv.X, 0.0f, 1.0f), FMath::Clamp(LocalUv.Y, 0.0f, 1.0f));

	return InnerMin + FVector2D(InnerSize.X * ClampedUv.X, InnerSize.Y * ClampedUv.Y);
}

EChameleonUvIsland SelectFourWayBodyIsland(
	const FVector& Normal,
	EChameleonUvIsland Front,
	EChameleonUvIsland Back,
	EChameleonUvIsland Left,
	EChameleonUvIsland Right)
{
	if (FMath::Abs(Normal.X) >= FMath::Abs(Normal.Y))
	{
		return Normal.X >= 0.0 ? Front : Back;
	}

	return Normal.Y < 0.0 ? Left : Right;
}

EChameleonUvIsland SelectUvIslandForTriangle(const FVector& UnitPoint, const FVector& Normal)
{
	const EChameleonProceduralBoneSlot DominantBone = GetDominantBoneForFieldPoint(UnitPoint);

	if (DominantBone == EChameleonProceduralBoneSlot::Head
		|| (DominantBone == EChameleonProceduralBoneSlot::Neck && UnitPoint.Z >= 2.16))
	{
		return SelectFourWayBodyIsland(
			Normal,
			EChameleonUvIsland::HeadFront,
			EChameleonUvIsland::HeadBack,
			EChameleonUvIsland::HeadLeft,
			EChameleonUvIsland::HeadRight);
	}

	if (IsArmBone(DominantBone))
	{
		const bool bLeftArm = IsLeftLimbBone(DominantBone);
		if (FMath::Abs(Normal.X) >= FMath::Abs(Normal.Z))
		{
			if (bLeftArm)
			{
				return Normal.X >= 0.0 ? EChameleonUvIsland::LeftArmFront : EChameleonUvIsland::LeftArmBack;
			}

			return Normal.X >= 0.0 ? EChameleonUvIsland::RightArmFront : EChameleonUvIsland::RightArmBack;
		}

		if (bLeftArm)
		{
			return Normal.Z >= 0.0 ? EChameleonUvIsland::LeftArmTop : EChameleonUvIsland::LeftArmBottom;
		}

		return Normal.Z >= 0.0 ? EChameleonUvIsland::RightArmTop : EChameleonUvIsland::RightArmBottom;
	}

	if (IsLegBone(DominantBone))
	{
		const bool bLeftLeg = IsLeftLimbBone(DominantBone);
		if (FMath::Abs(Normal.X) >= FMath::Abs(Normal.Y))
		{
			if (bLeftLeg)
			{
				return Normal.X >= 0.0 ? EChameleonUvIsland::LeftLegFront : EChameleonUvIsland::LeftLegBack;
			}

			return Normal.X >= 0.0 ? EChameleonUvIsland::RightLegFront : EChameleonUvIsland::RightLegBack;
		}

		if (bLeftLeg)
		{
			return Normal.Y < 0.0 ? EChameleonUvIsland::LeftLegOuter : EChameleonUvIsland::LeftLegInner;
		}

		return Normal.Y > 0.0 ? EChameleonUvIsland::RightLegOuter : EChameleonUvIsland::RightLegInner;
	}

	return SelectFourWayBodyIsland(
		Normal,
		EChameleonUvIsland::TorsoFront,
		EChameleonUvIsland::TorsoBack,
		EChameleonUvIsland::TorsoLeft,
		EChameleonUvIsland::TorsoRight);
}

FVector2D ComputeLocalUvForIsland(EChameleonUvIsland Island, const FVector& UnitPoint)
{
	switch (Island)
	{
	case EChameleonUvIsland::HeadFront:
	case EChameleonUvIsland::HeadBack:
		return FVector2D(
			NormalizeUvRange(UnitPoint.Y, -0.48, 0.48),
			NormalizeUvRange(UnitPoint.Z, 2.14, 3.08));

	case EChameleonUvIsland::HeadLeft:
	case EChameleonUvIsland::HeadRight:
		return FVector2D(
			NormalizeUvRange(UnitPoint.X, -0.48, 0.48),
			NormalizeUvRange(UnitPoint.Z, 2.14, 3.08));

	case EChameleonUvIsland::TorsoFront:
	case EChameleonUvIsland::TorsoBack:
		return FVector2D(
			NormalizeUvRange(UnitPoint.Y, -0.58, 0.58),
			NormalizeUvRange(UnitPoint.Z, 0.42, 2.36));

	case EChameleonUvIsland::TorsoLeft:
	case EChameleonUvIsland::TorsoRight:
		return FVector2D(
			NormalizeUvRange(UnitPoint.X, -0.56, 0.58),
			NormalizeUvRange(UnitPoint.Z, 0.42, 2.36));

	case EChameleonUvIsland::LeftArmFront:
	case EChameleonUvIsland::LeftArmBack:
		return FVector2D(
			NormalizeUvRange(UnitPoint.Y, -1.76, -0.40),
			NormalizeUvRange(UnitPoint.Z, 1.54, 2.20));

	case EChameleonUvIsland::RightArmFront:
	case EChameleonUvIsland::RightArmBack:
		return FVector2D(
			NormalizeUvRange(UnitPoint.Y, 0.40, 1.76),
			NormalizeUvRange(UnitPoint.Z, 1.54, 2.20));

	case EChameleonUvIsland::LeftArmTop:
	case EChameleonUvIsland::LeftArmBottom:
		return FVector2D(
			NormalizeUvRange(UnitPoint.Y, -1.76, -0.40),
			NormalizeUvRange(UnitPoint.X, -0.30, 0.30));

	case EChameleonUvIsland::RightArmTop:
	case EChameleonUvIsland::RightArmBottom:
		return FVector2D(
			NormalizeUvRange(UnitPoint.Y, 0.40, 1.76),
			NormalizeUvRange(UnitPoint.X, -0.30, 0.30));

	case EChameleonUvIsland::LeftLegFront:
	case EChameleonUvIsland::LeftLegBack:
		return FVector2D(
			NormalizeUvRange(UnitPoint.Y, -0.54, -0.06),
			NormalizeUvRange(UnitPoint.Z, -0.16, 0.88));

	case EChameleonUvIsland::RightLegFront:
	case EChameleonUvIsland::RightLegBack:
		return FVector2D(
			NormalizeUvRange(UnitPoint.Y, 0.06, 0.54),
			NormalizeUvRange(UnitPoint.Z, -0.16, 0.88));

	case EChameleonUvIsland::LeftLegOuter:
	case EChameleonUvIsland::LeftLegInner:
	case EChameleonUvIsland::RightLegOuter:
	case EChameleonUvIsland::RightLegInner:
		return FVector2D(
			NormalizeUvRange(UnitPoint.X, -0.34, 0.56),
			NormalizeUvRange(UnitPoint.Z, -0.16, 0.88));

	default:
		return FVector2D(0.5f, 0.5f);
	}
}

FVector2D ComputeUnwrappedUv(EChameleonUvIsland Island, const FVector& UnitPoint)
{
	return PackUvIsland(Island, ComputeLocalUvForIsland(Island, UnitPoint));
}

FVector ComputeBarycentricWeights(const FVector& Point, const FVector& A, const FVector& B, const FVector& C)
{
	const FVector V0 = B - A;
	const FVector V1 = C - A;
	const FVector V2 = Point - A;
	const double D00 = FVector::DotProduct(V0, V0);
	const double D01 = FVector::DotProduct(V0, V1);
	const double D11 = FVector::DotProduct(V1, V1);
	const double D20 = FVector::DotProduct(V2, V0);
	const double D21 = FVector::DotProduct(V2, V1);
	const double Denominator = D00 * D11 - D01 * D01;
	if (FMath::IsNearlyZero(Denominator))
	{
		return FVector(1.0, 0.0, 0.0);
	}

	const double V = (D11 * D20 - D01 * D21) / Denominator;
	const double W = (D00 * D21 - D01 * D20) / Denominator;
	const double U = 1.0 - V - W;
	return FVector(U, V, W);
}

FVector ComputeBarycentricWeights2D(const FVector2D& Point, const FVector2D& A, const FVector2D& B, const FVector2D& C)
{
	const FVector2D V0 = B - A;
	const FVector2D V1 = C - A;
	const FVector2D V2 = Point - A;
	const double D00 = FVector2D::DotProduct(V0, V0);
	const double D01 = FVector2D::DotProduct(V0, V1);
	const double D11 = FVector2D::DotProduct(V1, V1);
	const double D20 = FVector2D::DotProduct(V2, V0);
	const double D21 = FVector2D::DotProduct(V2, V1);
	const double Denominator = D00 * D11 - D01 * D01;
	if (FMath::IsNearlyZero(Denominator))
	{
		return FVector(-1.0, -1.0, -1.0);
	}

	const double V = (D11 * D20 - D01 * D21) / Denominator;
	const double W = (D00 * D21 - D01 * D20) / Denominator;
	const double U = 1.0 - V - W;
	return FVector(U, V, W);
}

float SquaredDistanceToBox(const FVector& Point, const FBox& Box)
{
	const float Dx = Point.X < Box.Min.X ? static_cast<float>(Box.Min.X - Point.X) : (Point.X > Box.Max.X ? static_cast<float>(Point.X - Box.Max.X) : 0.0f);
	const float Dy = Point.Y < Box.Min.Y ? static_cast<float>(Box.Min.Y - Point.Y) : (Point.Y > Box.Max.Y ? static_cast<float>(Point.Y - Box.Max.Y) : 0.0f);
	const float Dz = Point.Z < Box.Min.Z ? static_cast<float>(Box.Min.Z - Point.Z) : (Point.Z > Box.Max.Z ? static_cast<float>(Point.Z - Box.Max.Z) : 0.0f);
	return Dx * Dx + Dy * Dy + Dz * Dz;
}

FVector2D ClosestPointOnSegment2D(const FVector2D& Point, const FVector2D& A, const FVector2D& B)
{
	const FVector2D Segment = B - A;
	const double SegmentLengthSquared = Segment.SizeSquared();
	if (SegmentLengthSquared <= UE_SMALL_NUMBER)
	{
		return A;
	}

	const double T = FMath::Clamp(FVector2D::DotProduct(Point - A, Segment) / SegmentLengthSquared, 0.0, 1.0);
	return A + Segment * T;
}

float SquaredDistanceToSegment2D(const FVector2D& Point, const FVector2D& A, const FVector2D& B)
{
	return FVector2D::DistSquared(Point, ClosestPointOnSegment2D(Point, A, B));
}

FVector2D ClosestPointOnTriangle2D(const FVector2D& Point, const FVector2D& A, const FVector2D& B, const FVector2D& C)
{
	const FVector Weights = ComputeBarycentricWeights2D(Point, A, B, C);
	if (Weights.X >= 0.0 && Weights.Y >= 0.0 && Weights.Z >= 0.0)
	{
		return Point;
	}

	FVector2D ClosestPoint = ClosestPointOnSegment2D(Point, A, B);
	float BestDistanceSquared = FVector2D::DistSquared(Point, ClosestPoint);
	const auto TryCandidate = [&Point, &ClosestPoint, &BestDistanceSquared](const FVector2D& Candidate)
	{
		const float CandidateDistanceSquared = FVector2D::DistSquared(Point, Candidate);
		if (CandidateDistanceSquared < BestDistanceSquared)
		{
			BestDistanceSquared = CandidateDistanceSquared;
			ClosestPoint = Candidate;
		}
	};

	TryCandidate(ClosestPointOnSegment2D(Point, B, C));
	TryCandidate(ClosestPointOnSegment2D(Point, C, A));
	return ClosestPoint;
}

float SquaredDistanceToTriangle2D(const FVector2D& Point, const FVector2D& A, const FVector2D& B, const FVector2D& C)
{
	return FVector2D::DistSquared(Point, ClosestPointOnTriangle2D(Point, A, B, C));
}

FColor ScalarToPaintPixel(float Value)
{
	const uint8 QuantizedValue = static_cast<uint8>(FMath::RoundToInt(FMath::Clamp(Value, 0.0f, 1.0f) * 255.0f));
	return FColor(QuantizedValue, QuantizedValue, QuantizedValue, 255);
}

float PaintPixelToScalar(FColor Pixel)
{
	return static_cast<float>(Pixel.R) / 255.0f;
}

UTexture2D* CreateTransientPaintTexture(int32 TextureSize, const FName& TextureName, bool bSRGB)
{
	UTexture2D* Texture = UTexture2D::CreateTransient(TextureSize, TextureSize, PF_B8G8R8A8, TextureName);
	if (Texture)
	{
		Texture->SRGB = bSRGB;
		Texture->NeverStream = true;
		Texture->CompressionSettings = TC_Default;
		Texture->MipGenSettings = TMGS_NoMipmaps;
		Texture->Filter = TF_Bilinear;
		Texture->AddressX = TA_Clamp;
		Texture->AddressY = TA_Clamp;
		Texture->UpdateResource();
	}

	return Texture;
}

void FillPaintPixels(TArray<FColor>& Pixels, FColor FillPixel)
{
	for (FColor& Pixel : Pixels)
	{
		Pixel = FillPixel;
	}
}

void UpdatePaintTexture(UTexture2D* Texture, TArray<FColor>& Pixels, int32 TextureSize)
{
	if (!Texture || Pixels.IsEmpty())
	{
		return;
	}

	FUpdateTextureRegion2D* UpdateRegion = new FUpdateTextureRegion2D(0, 0, 0, 0, TextureSize, TextureSize);
	Texture->UpdateTextureRegions(
		0,
		1,
		UpdateRegion,
		static_cast<uint32>(TextureSize * sizeof(FColor)),
		static_cast<uint32>(sizeof(FColor)),
		reinterpret_cast<uint8*>(Pixels.GetData()),
		[](uint8*, const FUpdateTextureRegion2D* RegionToDelete)
		{
			delete RegionToDelete;
		});
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
	BaseColorTextureParameterNames = {
		TEXT("BaseColorPaintTexture"),
		TEXT("PaintBaseColorTexture"),
		TEXT("BaseColorTexture"),
		TEXT("BaseColorMap")
	};
	RoughnessTextureParameterNames = {
		TEXT("RoughnessPaintTexture"),
		TEXT("PaintRoughnessTexture"),
		TEXT("RoughnessTexture"),
		TEXT("RoughnessMap")
	};
	MetallicTextureParameterNames = {
		TEXT("MetallicPaintTexture"),
		TEXT("PaintMetallicTexture"),
		TEXT("MetallicTexture"),
		TEXT("MetallicMap")
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
	for (const FImplicitTriangle& Triangle : ImplicitTriangles)
	{
		for (const FVector& Point : Triangle.Points)
		{
			MinZ = FMath::Min(MinZ, Point.Z);
		}
	}

	CachedVertices.Reset();
	CachedTriangles.Reset();
	CachedNormals.Reset();
	CachedUV0.Reset();
	CachedPaintTriangles.Reset();
	CachedVertexColors.Reset();
	CachedTangents.Reset();
	AnimatedVertices.Reset();
	AnimatedNormals.Reset();
	AnimatedTangents.Reset();
	CachedSkinWeights.Reset();

	CachedTriangles.Reserve(ImplicitTriangles.Num() * 3);
	TMap<FString, int32> VertexIndices;
	VertexIndices.Reserve(ImplicitTriangles.Num());

	auto AddVertex = [&](const FVector& UnitPoint, EChameleonUvIsland UvIsland) -> int32
	{
		const FVector ShiftedUnit(UnitPoint.X, UnitPoint.Y, UnitPoint.Z - MinZ);
		const FVector PositionCm = ShiftedUnit * MeshScaleToCentimeters;
		const FString Key = FString::Printf(TEXT("%.3f,%.3f,%.3f,%d"), PositionCm.X, PositionCm.Y, PositionCm.Z, static_cast<int32>(UvIsland));

		if (const int32* ExistingIndex = VertexIndices.Find(Key))
		{
			return *ExistingIndex;
		}

		const FVector Normal = FieldGradientAt(UnitPoint);
		const FVector2D UnwrappedUv = ComputeUnwrappedUv(UvIsland, UnitPoint);
		const int32 NewIndex = CachedVertices.Num();

		VertexIndices.Add(Key, NewIndex);
		CachedVertices.Add(PositionCm);
		CachedNormals.Add(Normal);
		CachedUV0.Add(UnwrappedUv);
		CachedVertexColors.Add(CamouflageBaseColor);
		CachedTangents.Add(MakeTangentFromNormal(Normal));
		CachedSkinWeights.Add(MakeSkinWeightsForFieldPoint(UnitPoint));
		return NewIndex;
	};

	for (const FImplicitTriangle& Triangle : ImplicitTriangles)
	{
		const FVector TriangleCenter = (Triangle.Points[0] + Triangle.Points[1] + Triangle.Points[2]) / 3.0;
		const EChameleonUvIsland UvIsland = SelectUvIslandForTriangle(TriangleCenter, FieldGradientAt(TriangleCenter));

		CachedTriangles.Add(AddVertex(Triangle.Points[0], UvIsland));
		CachedTriangles.Add(AddVertex(Triangle.Points[1], UvIsland));
		CachedTriangles.Add(AddVertex(Triangle.Points[2], UvIsland));
	}
	BuildPaintTriangleCache();

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
	EnsureBaseColorPaintTexture();
	EnsureRoughnessPaintTexture();
	EnsureMetallicPaintTexture();
	ResetBaseColorPaintTexture();
	ResetRoughnessPaintTexture();
	ResetMetallicPaintTexture();

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
	ResetBaseColorPaintTexture();
	UpdatePaintedMeshSection();
	ApplyMaterialPaintParameters();
}

void UChameleonMetaballBodyComponent::ClearPaint()
{
	PaintStrokes.Reset();
	RebuildVertexColors();
	ResetBaseColorPaintTexture();
	ResetRoughnessPaintTexture();
	ResetMetallicPaintTexture();
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
	SanitizedStroke.Roughness = FMath::Clamp(SanitizedStroke.Roughness, 0.0f, 1.0f);
	SanitizedStroke.Metallic = FMath::Clamp(SanitizedStroke.Metallic, 0.0f, 1.0f);
	SanitizedStroke.Strength = FMath::Clamp(SanitizedStroke.Strength, 0.0f, 1.0f);
	SanitizedStroke.Falloff = FMath::Max(SanitizedStroke.Falloff, 0.1f);
	PaintStrokes.Add(SanitizedStroke);

	RebuildVertexColors();
	UpdatePaintedMeshSection();
	return true;
}

bool UChameleonMetaballBodyComponent::ApplyPaintStrokeWorld(FVector WorldPosition, FVector WorldNormal, float RadiusCm, FLinearColor Color, float Strength, float Roughness, float Metallic)
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
	Stroke.Roughness = Roughness;
	Stroke.Metallic = Metallic;
	Stroke.Strength = Strength;

	const bool bAppliedLegacyStroke = ApplyPaintStrokeLocal(Stroke);
	const bool bAppliedTextureStroke = ApplyMaterialTextureStrokeLocal(Stroke);
	return bAppliedLegacyStroke || bAppliedTextureStroke;
}

bool UChameleonMetaballBodyComponent::ApplyPaintStrokeFromHit(const FHitResult& Hit, float RadiusCm, FLinearColor Color, float Strength, float Roughness, float Metallic)
{
	if (Hit.Component.Get() != this)
	{
		return false;
	}

	if (Hit.FaceIndex < 0 || CachedTriangles.IsEmpty() || CachedVertices.IsEmpty())
	{
		return ApplyPaintStrokeWorld(Hit.ImpactPoint, Hit.ImpactNormal, RadiusCm, Color, Strength, Roughness, Metallic);
	}

	const int32 TriangleStart = Hit.FaceIndex * 3;
	if (!CachedTriangles.IsValidIndex(TriangleStart + 2))
	{
		return ApplyPaintStrokeWorld(Hit.ImpactPoint, Hit.ImpactNormal, RadiusCm, Color, Strength, Roughness, Metallic);
	}

	const int32 IndexA = CachedTriangles[TriangleStart];
	const int32 IndexB = CachedTriangles[TriangleStart + 1];
	const int32 IndexC = CachedTriangles[TriangleStart + 2];
	const TArray<FVector>& VerticesToUse = AnimatedVertices.Num() == CachedVertices.Num() ? AnimatedVertices : CachedVertices;
	if (!VerticesToUse.IsValidIndex(IndexA) || !VerticesToUse.IsValidIndex(IndexB) || !VerticesToUse.IsValidIndex(IndexC)
		|| !CachedVertices.IsValidIndex(IndexA) || !CachedVertices.IsValidIndex(IndexB) || !CachedVertices.IsValidIndex(IndexC))
	{
		return ApplyPaintStrokeWorld(Hit.ImpactPoint, Hit.ImpactNormal, RadiusCm, Color, Strength, Roughness, Metallic);
	}

	const FVector AnimatedLocalPosition = GetComponentTransform().InverseTransformPosition(Hit.ImpactPoint);
	const FVector Weights = ComputeBarycentricWeights(
		AnimatedLocalPosition,
		VerticesToUse[IndexA],
		VerticesToUse[IndexB],
		VerticesToUse[IndexC]);

	FChameleonPaintStroke Stroke;
	Stroke.LocalPositionCm = CachedVertices[IndexA] * Weights.X
		+ CachedVertices[IndexB] * Weights.Y
		+ CachedVertices[IndexC] * Weights.Z;
	const FVector NormalA = CachedNormals.IsValidIndex(IndexA) ? CachedNormals[IndexA] : FVector::UpVector;
	const FVector NormalB = CachedNormals.IsValidIndex(IndexB) ? CachedNormals[IndexB] : FVector::UpVector;
	const FVector NormalC = CachedNormals.IsValidIndex(IndexC) ? CachedNormals[IndexC] : FVector::UpVector;
	Stroke.LocalNormal = (NormalA * Weights.X + NormalB * Weights.Y + NormalC * Weights.Z).GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
	Stroke.RadiusCm = RadiusCm;
	Stroke.Color = Color;
	Stroke.Roughness = Roughness;
	Stroke.Metallic = Metallic;
	Stroke.Strength = Strength;

	const bool bAppliedLegacyStroke = ApplyPaintStrokeLocal(Stroke);
	const bool bAppliedTextureStroke = ApplyMaterialTextureStrokeLocal(Stroke);
	return bAppliedLegacyStroke || bAppliedTextureStroke;
}

bool UChameleonMetaballBodyComponent::TrySampleBaseColorFromHit(const FHitResult& Hit, FLinearColor& OutColor) const
{
	if (Hit.Component.Get() != this)
	{
		return false;
	}

	if (Hit.FaceIndex < 0 || CachedTriangles.IsEmpty() || CachedUV0.IsEmpty() || PaintTextureSize <= 0)
	{
		OutColor = CamouflageBaseColor;
		OutColor.A = 1.0f;
		return true;
	}

	const int32 TriangleStart = Hit.FaceIndex * 3;
	if (!CachedTriangles.IsValidIndex(TriangleStart + 2))
	{
		return false;
	}

	const int32 IndexA = CachedTriangles[TriangleStart];
	const int32 IndexB = CachedTriangles[TriangleStart + 1];
	const int32 IndexC = CachedTriangles[TriangleStart + 2];
	const TArray<FVector>& VerticesToUse = AnimatedVertices.Num() == CachedVertices.Num() ? AnimatedVertices : CachedVertices;
	if (!VerticesToUse.IsValidIndex(IndexA) || !VerticesToUse.IsValidIndex(IndexB) || !VerticesToUse.IsValidIndex(IndexC)
		|| !CachedUV0.IsValidIndex(IndexA) || !CachedUV0.IsValidIndex(IndexB) || !CachedUV0.IsValidIndex(IndexC))
	{
		return false;
	}

	const FVector AnimatedLocalPosition = GetComponentTransform().InverseTransformPosition(Hit.ImpactPoint);
	const FVector Weights = ComputeBarycentricWeights(
		AnimatedLocalPosition,
		VerticesToUse[IndexA],
		VerticesToUse[IndexB],
		VerticesToUse[IndexC]);
	const FVector2D SampleUv = CachedUV0[IndexA] * Weights.X
		+ CachedUV0[IndexB] * Weights.Y
		+ CachedUV0[IndexC] * Weights.Z;
	const int32 PixelX = FMath::Clamp(FMath::RoundToInt(FMath::Clamp(SampleUv.X, 0.0f, 1.0f) * static_cast<float>(PaintTextureSize - 1)), 0, PaintTextureSize - 1);
	const int32 PixelY = FMath::Clamp(FMath::RoundToInt(FMath::Clamp(SampleUv.Y, 0.0f, 1.0f) * static_cast<float>(PaintTextureSize - 1)), 0, PaintTextureSize - 1);
	const int32 PixelIndex = PixelY * PaintTextureSize + PixelX;
	if (!BaseColorPaintPixels.IsValidIndex(PixelIndex))
	{
		OutColor = CamouflageBaseColor;
		OutColor.A = 1.0f;
		return true;
	}

	OutColor = FLinearColor::FromSRGBColor(BaseColorPaintPixels[PixelIndex]);
	OutColor.A = 1.0f;
	return true;
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

	EnsureBaseColorPaintTexture();
	EnsureRoughnessPaintTexture();
	EnsureMetallicPaintTexture();
	if (BaseColorPaintTexture)
	{
		for (const FName& ParameterName : BaseColorTextureParameterNames)
		{
			if (!ParameterName.IsNone())
			{
				DynamicMaterial->SetTextureParameterValue(ParameterName, BaseColorPaintTexture);
			}
		}
	}
	if (RoughnessPaintTexture)
	{
		for (const FName& ParameterName : RoughnessTextureParameterNames)
		{
			if (!ParameterName.IsNone())
			{
				DynamicMaterial->SetTextureParameterValue(ParameterName, RoughnessPaintTexture);
			}
		}
	}
	if (MetallicPaintTexture)
	{
		for (const FName& ParameterName : MetallicTextureParameterNames)
		{
			if (!ParameterName.IsNone())
			{
				DynamicMaterial->SetTextureParameterValue(ParameterName, MetallicPaintTexture);
			}
		}
	}

	for (const FName& ParameterName : PaintColorParameterNames)
	{
		if (!ParameterName.IsNone())
		{
			DynamicMaterial->SetVectorParameterValue(ParameterName, CamouflageBaseColor);
		}
	}
}

void UChameleonMetaballBodyComponent::EnsureBaseColorPaintTexture()
{
	const int32 ClampedTextureSize = FMath::Clamp(PaintTextureSize, 128, 4096);
	const int32 RequiredPixelCount = ClampedTextureSize * ClampedTextureSize;
	if (BaseColorPaintTexture && BaseColorPaintTexture->GetSizeX() == ClampedTextureSize && BaseColorPaintTexture->GetSizeY() == ClampedTextureSize && BaseColorPaintPixels.Num() == RequiredPixelCount)
	{
		return;
	}

	PaintTextureSize = ClampedTextureSize;
	BaseColorPaintTexture = CreateTransientPaintTexture(PaintTextureSize, FName(TEXT("ChameleonBaseColorPaintTexture")), true);

	BaseColorPaintPixels.SetNumUninitialized(RequiredPixelCount);
	FLinearColor FillColor = CamouflageBaseColor;
	FillColor.A = 1.0f;
	FillPaintPixels(BaseColorPaintPixels, FillColor.ToFColorSRGB());
	UpdateBaseColorPaintTexture();
}

void UChameleonMetaballBodyComponent::EnsureRoughnessPaintTexture()
{
	const int32 ClampedTextureSize = FMath::Clamp(PaintTextureSize, 128, 4096);
	const int32 RequiredPixelCount = ClampedTextureSize * ClampedTextureSize;
	if (RoughnessPaintTexture && RoughnessPaintTexture->GetSizeX() == ClampedTextureSize && RoughnessPaintTexture->GetSizeY() == ClampedTextureSize && RoughnessPaintPixels.Num() == RequiredPixelCount)
	{
		return;
	}

	PaintTextureSize = ClampedTextureSize;
	RoughnessPaintTexture = CreateTransientPaintTexture(PaintTextureSize, FName(TEXT("ChameleonRoughnessPaintTexture")), false);
	RoughnessPaintPixels.SetNumUninitialized(RequiredPixelCount);
	FillPaintPixels(RoughnessPaintPixels, ScalarToPaintPixel(CamouflageBaseRoughness));
	UpdateRoughnessPaintTexture();
}

void UChameleonMetaballBodyComponent::EnsureMetallicPaintTexture()
{
	const int32 ClampedTextureSize = FMath::Clamp(PaintTextureSize, 128, 4096);
	const int32 RequiredPixelCount = ClampedTextureSize * ClampedTextureSize;
	if (MetallicPaintTexture && MetallicPaintTexture->GetSizeX() == ClampedTextureSize && MetallicPaintTexture->GetSizeY() == ClampedTextureSize && MetallicPaintPixels.Num() == RequiredPixelCount)
	{
		return;
	}

	PaintTextureSize = ClampedTextureSize;
	MetallicPaintTexture = CreateTransientPaintTexture(PaintTextureSize, FName(TEXT("ChameleonMetallicPaintTexture")), false);
	MetallicPaintPixels.SetNumUninitialized(RequiredPixelCount);
	FillPaintPixels(MetallicPaintPixels, ScalarToPaintPixel(CamouflageBaseMetallic));
	UpdateMetallicPaintTexture();
}

void UChameleonMetaballBodyComponent::ResetBaseColorPaintTexture()
{
	EnsureBaseColorPaintTexture();
	if (BaseColorPaintPixels.IsEmpty())
	{
		return;
	}

	FLinearColor FillColor = CamouflageBaseColor;
	FillColor.A = 1.0f;
	FillPaintPixels(BaseColorPaintPixels, FillColor.ToFColorSRGB());

	UpdateBaseColorPaintTexture();
}

void UChameleonMetaballBodyComponent::ResetRoughnessPaintTexture()
{
	EnsureRoughnessPaintTexture();
	if (RoughnessPaintPixels.IsEmpty())
	{
		return;
	}

	FillPaintPixels(RoughnessPaintPixels, ScalarToPaintPixel(CamouflageBaseRoughness));
	UpdateRoughnessPaintTexture();
}

void UChameleonMetaballBodyComponent::ResetMetallicPaintTexture()
{
	EnsureMetallicPaintTexture();
	if (MetallicPaintPixels.IsEmpty())
	{
		return;
	}

	FillPaintPixels(MetallicPaintPixels, ScalarToPaintPixel(CamouflageBaseMetallic));
	UpdateMetallicPaintTexture();
}

void UChameleonMetaballBodyComponent::UpdateBaseColorPaintTexture()
{
	UpdatePaintTexture(BaseColorPaintTexture, BaseColorPaintPixels, PaintTextureSize);
}

void UChameleonMetaballBodyComponent::UpdateRoughnessPaintTexture()
{
	UpdatePaintTexture(RoughnessPaintTexture, RoughnessPaintPixels, PaintTextureSize);
}

void UChameleonMetaballBodyComponent::UpdateMetallicPaintTexture()
{
	UpdatePaintTexture(MetallicPaintTexture, MetallicPaintPixels, PaintTextureSize);
}

void UChameleonMetaballBodyComponent::BuildPaintTriangleCache()
{
	CachedPaintTriangles.Reset();
	CachedPaintTriangles.Reserve(CachedTriangles.Num() / 3);

	for (int32 TriangleStart = 0; TriangleStart + 2 < CachedTriangles.Num(); TriangleStart += 3)
	{
		const int32 IndexA = CachedTriangles[TriangleStart];
		const int32 IndexB = CachedTriangles[TriangleStart + 1];
		const int32 IndexC = CachedTriangles[TriangleStart + 2];
		if (!CachedVertices.IsValidIndex(IndexA) || !CachedVertices.IsValidIndex(IndexB) || !CachedVertices.IsValidIndex(IndexC)
			|| !CachedUV0.IsValidIndex(IndexA) || !CachedUV0.IsValidIndex(IndexB) || !CachedUV0.IsValidIndex(IndexC))
		{
			continue;
		}

		FChameleonPaintTriangleCache TriangleCache;
		TriangleCache.Indices[0] = IndexA;
		TriangleCache.Indices[1] = IndexB;
		TriangleCache.Indices[2] = IndexC;
		TriangleCache.RestPositions[0] = CachedVertices[IndexA];
		TriangleCache.RestPositions[1] = CachedVertices[IndexB];
		TriangleCache.RestPositions[2] = CachedVertices[IndexC];
		TriangleCache.UVs[0] = CachedUV0[IndexA];
		TriangleCache.UVs[1] = CachedUV0[IndexB];
		TriangleCache.UVs[2] = CachedUV0[IndexC];
		TriangleCache.RestBounds = FBox(ForceInit);
		TriangleCache.RestBounds += TriangleCache.RestPositions[0];
		TriangleCache.RestBounds += TriangleCache.RestPositions[1];
		TriangleCache.RestBounds += TriangleCache.RestPositions[2];
		TriangleCache.UvBounds = FBox2D(ForceInit);
		TriangleCache.UvBounds += TriangleCache.UVs[0];
		TriangleCache.UvBounds += TriangleCache.UVs[1];
		TriangleCache.UvBounds += TriangleCache.UVs[2];
		TriangleCache.RestNormal = FVector::CrossProduct(
			TriangleCache.RestPositions[1] - TriangleCache.RestPositions[0],
			TriangleCache.RestPositions[2] - TriangleCache.RestPositions[0]).GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
		CachedPaintTriangles.Add(TriangleCache);
	}
}

bool UChameleonMetaballBodyComponent::ApplyMaterialTextureStrokeLocal(const FChameleonPaintStroke& Stroke)
{
	EnsureBaseColorPaintTexture();
	EnsureRoughnessPaintTexture();
	EnsureMetallicPaintTexture();
	if (!BaseColorPaintTexture || !RoughnessPaintTexture || !MetallicPaintTexture
		|| BaseColorPaintPixels.IsEmpty() || RoughnessPaintPixels.IsEmpty() || MetallicPaintPixels.IsEmpty()
		|| Stroke.RadiusCm <= 0.0f)
	{
		return false;
	}

	if (CachedPaintTriangles.IsEmpty() && !CachedTriangles.IsEmpty())
	{
		BuildPaintTriangleCache();
	}

	FLinearColor Color = Stroke.Color;
	Color.A = 1.0f;
	const float ClampedRoughness = FMath::Clamp(Stroke.Roughness, 0.0f, 1.0f);
	const float ClampedMetallic = FMath::Clamp(Stroke.Metallic, 0.0f, 1.0f);
	const float ClampedStrength = FMath::Clamp(Stroke.Strength, 0.0f, 1.0f);
	const float ClampedFalloff = FMath::Max(Stroke.Falloff, 0.1f);
	const float RadiusCm = Stroke.RadiusCm;
	const float RadiusSquared = RadiusCm * RadiusCm;
	const float FeatherRatio = FMath::Clamp(PaintTextureBrushFeatherRatio, 0.0f, 1.0f);
	const float InnerRadiusCm = RadiusCm * (1.0f - FeatherRatio);
	const float FeatherWidthCm = FMath::Max(RadiusCm - InnerRadiusCm, UE_SMALL_NUMBER);
	const float TextureCoordinateScale = static_cast<float>(PaintTextureSize - 1);
	const float TriangleDilationPixels = FMath::Max(PaintTextureTriangleDilationPixels, 0.0f);
	const float TriangleDilationSquared = TriangleDilationPixels * TriangleDilationPixels;

	bool bChangedAnyPixel = false;
	for (const FChameleonPaintTriangleCache& TriangleCache : CachedPaintTriangles)
	{
		if (!TriangleCache.RestBounds.IsValid || !TriangleCache.UvBounds.bIsValid
			|| SquaredDistanceToBox(Stroke.LocalPositionCm, TriangleCache.RestBounds) > RadiusSquared)
		{
			continue;
		}

		const FVector ClosestPoint = FMath::ClosestPointOnTriangleToPoint(
			Stroke.LocalPositionCm,
			TriangleCache.RestPositions[0],
			TriangleCache.RestPositions[1],
			TriangleCache.RestPositions[2]);
		if (FVector::DistSquared(ClosestPoint, Stroke.LocalPositionCm) > RadiusSquared)
		{
			continue;
		}

		const FVector2D UvPixelA = TriangleCache.UVs[0] * TextureCoordinateScale;
		const FVector2D UvPixelB = TriangleCache.UVs[1] * TextureCoordinateScale;
		const FVector2D UvPixelC = TriangleCache.UVs[2] * TextureCoordinateScale;
		const float MinTriangleX = FMath::Min3(UvPixelA.X, UvPixelB.X, UvPixelC.X);
		const float MaxTriangleX = FMath::Max3(UvPixelA.X, UvPixelB.X, UvPixelC.X);
		const float MinTriangleY = FMath::Min3(UvPixelA.Y, UvPixelB.Y, UvPixelC.Y);
		const float MaxTriangleY = FMath::Max3(UvPixelA.Y, UvPixelB.Y, UvPixelC.Y);
		const int32 MinX = FMath::Clamp(FMath::FloorToInt(MinTriangleX - TriangleDilationPixels), 0, PaintTextureSize - 1);
		const int32 MaxX = FMath::Clamp(FMath::CeilToInt(MaxTriangleX + TriangleDilationPixels), 0, PaintTextureSize - 1);
		const int32 MinY = FMath::Clamp(FMath::FloorToInt(MinTriangleY - TriangleDilationPixels), 0, PaintTextureSize - 1);
		const int32 MaxY = FMath::Clamp(FMath::CeilToInt(MaxTriangleY + TriangleDilationPixels), 0, PaintTextureSize - 1);

		for (int32 PixelY = MinY; PixelY <= MaxY; ++PixelY)
		{
			for (int32 PixelX = MinX; PixelX <= MaxX; ++PixelX)
			{
				const FVector2D PixelPoint(static_cast<float>(PixelX) + 0.5f, static_cast<float>(PixelY) + 0.5f);
				if (SquaredDistanceToTriangle2D(PixelPoint, UvPixelA, UvPixelB, UvPixelC) > TriangleDilationSquared)
				{
					continue;
				}

				const FVector2D TrianglePoint = ClosestPointOnTriangle2D(PixelPoint, UvPixelA, UvPixelB, UvPixelC);
				const FVector Weights = ComputeBarycentricWeights2D(TrianglePoint, UvPixelA, UvPixelB, UvPixelC);
				if (Weights.X < -KINDA_SMALL_NUMBER || Weights.Y < -KINDA_SMALL_NUMBER || Weights.Z < -KINDA_SMALL_NUMBER)
				{
					continue;
				}

				const double WeightSum = Weights.X + Weights.Y + Weights.Z;
				if (FMath::IsNearlyZero(WeightSum))
				{
					continue;
				}

				const double WeightA = FMath::Clamp(Weights.X / WeightSum, 0.0, 1.0);
				const double WeightB = FMath::Clamp(Weights.Y / WeightSum, 0.0, 1.0);
				const double WeightC = FMath::Clamp(Weights.Z / WeightSum, 0.0, 1.0);
				const FVector PixelRestPosition = TriangleCache.RestPositions[0] * WeightA
					+ TriangleCache.RestPositions[1] * WeightB
					+ TriangleCache.RestPositions[2] * WeightC;
				const float DistanceSquared = FVector::DistSquared(PixelRestPosition, Stroke.LocalPositionCm);
				if (DistanceSquared > RadiusSquared)
				{
					continue;
				}

				float Alpha = ClampedStrength;
				if (FeatherRatio > 0.0f)
				{
					const float DistanceCm = FMath::Sqrt(DistanceSquared);
					if (DistanceCm > InnerRadiusCm)
					{
						const float EdgeAlpha = 1.0f - ((DistanceCm - InnerRadiusCm) / FeatherWidthCm);
						Alpha *= FMath::Pow(FMath::Clamp(EdgeAlpha, 0.0f, 1.0f), ClampedFalloff);
					}
				}

				const int32 PixelIndex = PixelY * PaintTextureSize + PixelX;
				if (!BaseColorPaintPixels.IsValidIndex(PixelIndex)
					|| !RoughnessPaintPixels.IsValidIndex(PixelIndex)
					|| !MetallicPaintPixels.IsValidIndex(PixelIndex)
					|| Alpha <= 0.0f)
				{
					continue;
				}

				FLinearColor ExistingColor = FLinearColor::FromSRGBColor(BaseColorPaintPixels[PixelIndex]);
				ExistingColor.A = 1.0f;
				FLinearColor PaintedColor = FLinearColor::LerpUsingHSV(ExistingColor, Color, Alpha);
				PaintedColor.A = 1.0f;
				BaseColorPaintPixels[PixelIndex] = PaintedColor.ToFColorSRGB();

				const float ExistingRoughness = PaintPixelToScalar(RoughnessPaintPixels[PixelIndex]);
				RoughnessPaintPixels[PixelIndex] = ScalarToPaintPixel(FMath::Lerp(ExistingRoughness, ClampedRoughness, Alpha));

				const float ExistingMetallic = PaintPixelToScalar(MetallicPaintPixels[PixelIndex]);
				MetallicPaintPixels[PixelIndex] = ScalarToPaintPixel(FMath::Lerp(ExistingMetallic, ClampedMetallic, Alpha));
				bChangedAnyPixel = true;
			}
		}
	}

	if (bChangedAnyPixel)
	{
		UpdateBaseColorPaintTexture();
		UpdateRoughnessPaintTexture();
		UpdateMetallicPaintTexture();
	}

	return bChangedAnyPixel;
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

	FVector BestWeights = FVector(1.0, 0.0, 0.0);
	int32 BestTriangleIndices[3] = { INDEX_NONE, INDEX_NONE, INDEX_NONE };
	double BestDistanceSquared = TNumericLimits<double>::Max();
	for (int32 TriangleStart = 0; TriangleStart + 2 < CachedTriangles.Num(); TriangleStart += 3)
	{
		const int32 IndexA = CachedTriangles[TriangleStart];
		const int32 IndexB = CachedTriangles[TriangleStart + 1];
		const int32 IndexC = CachedTriangles[TriangleStart + 2];
		if (!AnimatedVertices.IsValidIndex(IndexA) || !AnimatedVertices.IsValidIndex(IndexB) || !AnimatedVertices.IsValidIndex(IndexC)
			|| !CachedVertices.IsValidIndex(IndexA) || !CachedVertices.IsValidIndex(IndexB) || !CachedVertices.IsValidIndex(IndexC))
		{
			continue;
		}

		const FVector ClosestPoint = FMath::ClosestPointOnTriangleToPoint(
			AnimatedLocalPosition,
			AnimatedVertices[IndexA],
			AnimatedVertices[IndexB],
			AnimatedVertices[IndexC]);
		const double DistanceSquared = FVector::DistSquared(ClosestPoint, AnimatedLocalPosition);
		if (DistanceSquared < BestDistanceSquared)
		{
			BestDistanceSquared = DistanceSquared;
			BestWeights = ComputeBarycentricWeights(
				ClosestPoint,
				AnimatedVertices[IndexA],
				AnimatedVertices[IndexB],
				AnimatedVertices[IndexC]);
			BestTriangleIndices[0] = IndexA;
			BestTriangleIndices[1] = IndexB;
			BestTriangleIndices[2] = IndexC;
		}
	}

	if (BestTriangleIndices[0] == INDEX_NONE)
	{
		return false;
	}

	OutRestPosition = CachedVertices[BestTriangleIndices[0]] * BestWeights.X
		+ CachedVertices[BestTriangleIndices[1]] * BestWeights.Y
		+ CachedVertices[BestTriangleIndices[2]] * BestWeights.Z;
	const FVector NormalA = CachedNormals.IsValidIndex(BestTriangleIndices[0]) ? CachedNormals[BestTriangleIndices[0]] : FVector::UpVector;
	const FVector NormalB = CachedNormals.IsValidIndex(BestTriangleIndices[1]) ? CachedNormals[BestTriangleIndices[1]] : FVector::UpVector;
	const FVector NormalC = CachedNormals.IsValidIndex(BestTriangleIndices[2]) ? CachedNormals[BestTriangleIndices[2]] : FVector::UpVector;
	OutRestNormal = (NormalA * BestWeights.X + NormalB * BestWeights.Y + NormalC * BestWeights.Z).GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
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

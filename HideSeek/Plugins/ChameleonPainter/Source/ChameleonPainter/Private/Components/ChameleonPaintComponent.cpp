#include "Components/ChameleonPaintComponent.h"

#include "Components/ChameleonMetaballBodyComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

UChameleonPaintComponent::UChameleonPaintComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	PaintColorParameterNames = {
		TEXT("PaintColor"),
		TEXT("BodyColor"),
		TEXT("BaseColor"),
		TEXT("Color")
	};
}

void UChameleonPaintComponent::OnRegister()
{
	Super::OnRegister();

	if (bApplyOnRegister)
	{
		ApplyPaintColor();
	}
}

void UChameleonPaintComponent::BeginPlay()
{
	Super::BeginPlay();
	ApplyPaintColor();
}

void UChameleonPaintComponent::SetTargetComponent(UPrimitiveComponent* NewTargetComponent)
{
	TargetComponent = NewTargetComponent;
	DynamicMaterialInstances.Reset();
	ApplyPaintColor();
}

void UChameleonPaintComponent::SetPaintColor(FLinearColor NewColor)
{
	NewColor.A = 1.0f;
	PaintColor = NewColor;
	ApplyPaintColor();
	OnPaintColorChanged.Broadcast(PaintColor);
}

bool UChameleonPaintComponent::ApplyPaintColor()
{
	UPrimitiveComponent* ResolvedTarget = ResolveTargetComponent();
	if (!ResolvedTarget)
	{
		return false;
	}

	if (UChameleonMetaballBodyComponent* MetaballBody = Cast<UChameleonMetaballBodyComponent>(ResolvedTarget))
	{
		MetaballBody->SetCamouflageBaseColor(PaintColor);
	}

	const int32 NumMaterials = FMath::Max(ResolvedTarget->GetNumMaterials(), 1);
	bool bAppliedAnyMaterial = false;

	for (int32 SlotIndex = 0; SlotIndex < NumMaterials; ++SlotIndex)
	{
		if (!bApplyToAllMaterialSlots && SlotIndex != MaterialSlotIndex)
		{
			continue;
		}

		UMaterialInstanceDynamic* DynamicMaterial = Cast<UMaterialInstanceDynamic>(ResolvedTarget->GetMaterial(SlotIndex));
		if (!DynamicMaterial)
		{
			DynamicMaterial = ResolvedTarget->CreateDynamicMaterialInstance(SlotIndex);
		}
		if (!DynamicMaterial)
		{
			continue;
		}

		ApplyColorToMaterial(DynamicMaterial);
		DynamicMaterialInstances.AddUnique(DynamicMaterial);
		bAppliedAnyMaterial = true;
	}

	return bAppliedAnyMaterial || IsValid(Cast<UChameleonMetaballBodyComponent>(ResolvedTarget));
}

UPrimitiveComponent* UChameleonPaintComponent::ResolveTargetComponent() const
{
	if (TargetComponent)
	{
		return TargetComponent;
	}

	const AActor* Owner = GetOwner();
	if (!Owner)
	{
		return nullptr;
	}

	if (UChameleonMetaballBodyComponent* MetaballBody = Owner->FindComponentByClass<UChameleonMetaballBodyComponent>())
	{
		return MetaballBody;
	}

	return Owner->FindComponentByClass<UPrimitiveComponent>();
}

void UChameleonPaintComponent::ApplyColorToMaterial(UMaterialInstanceDynamic* MaterialInstance) const
{
	if (!MaterialInstance)
	{
		return;
	}

	for (const FName& ParameterName : PaintColorParameterNames)
	{
		if (!ParameterName.IsNone())
		{
			MaterialInstance->SetVectorParameterValue(ParameterName, PaintColor);
		}
	}
}

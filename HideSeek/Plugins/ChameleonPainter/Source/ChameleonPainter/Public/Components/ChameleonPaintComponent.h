#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ChameleonPaintComponent.generated.h"

class UMaterialInstanceDynamic;
class UPrimitiveComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FChameleonPaintColorChanged, FLinearColor, PaintColor);

UCLASS(ClassGroup = (Chameleon), meta = (BlueprintSpawnableComponent))
class CHAMELEONPAINTER_API UChameleonPaintComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UChameleonPaintComponent();

	virtual void OnRegister() override;
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "Chameleon|Paint")
	void SetTargetComponent(UPrimitiveComponent* NewTargetComponent);

	UFUNCTION(BlueprintCallable, Category = "Chameleon|Paint")
	void SetPaintColor(FLinearColor NewColor);

	UFUNCTION(BlueprintCallable, Category = "Chameleon|Paint")
	bool ApplyPaintColor();

	UFUNCTION(BlueprintPure, Category = "Chameleon|Paint")
	FLinearColor GetPaintColor() const { return PaintColor; }

	UFUNCTION(BlueprintPure, Category = "Chameleon|Paint")
	UPrimitiveComponent* GetTargetComponent() const { return TargetComponent; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chameleon|Paint")
	TObjectPtr<UPrimitiveComponent> TargetComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chameleon|Paint")
	FLinearColor PaintColor = FLinearColor(1.0f, 0.02f, 0.02f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chameleon|Paint")
	bool bApplyOnRegister = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chameleon|Paint")
	bool bApplyOnBeginPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chameleon|Paint")
	bool bApplyOnTargetComponentChange = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chameleon|Paint")
	bool bApplyToAllMaterialSlots = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chameleon|Paint", meta = (EditCondition = "!bApplyToAllMaterialSlots", ClampMin = "0"))
	int32 MaterialSlotIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chameleon|Paint")
	TArray<FName> PaintColorParameterNames;

	UPROPERTY(BlueprintAssignable, Category = "Chameleon|Paint")
	FChameleonPaintColorChanged OnPaintColorChanged;

private:
	UPrimitiveComponent* ResolveTargetComponent() const;
	void ApplyColorToMaterial(UMaterialInstanceDynamic* MaterialInstance) const;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> DynamicMaterialInstances;
};

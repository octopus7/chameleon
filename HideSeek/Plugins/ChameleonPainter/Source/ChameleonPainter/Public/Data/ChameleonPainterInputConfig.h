#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ChameleonPainterInputConfig.generated.h"

class UInputAction;
class UInputMappingContext;

UCLASS(BlueprintType)
class CHAMELEONPAINTER_API UChameleonPainterInputConfig : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chameleon|Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chameleon|Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chameleon|Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chameleon|Input")
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chameleon|Input")
	TObjectPtr<UInputAction> PaintAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chameleon|Input")
	TObjectPtr<UInputAction> SampleColorAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chameleon|Input")
	TObjectPtr<UInputAction> ToggleColorPickerAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chameleon|Input")
	TObjectPtr<UInputAction> DecreaseBrushSizeAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chameleon|Input")
	TObjectPtr<UInputAction> IncreaseBrushSizeAction;
};

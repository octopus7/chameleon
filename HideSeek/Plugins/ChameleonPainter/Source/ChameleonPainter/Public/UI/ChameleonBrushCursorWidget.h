#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ChameleonBrushCursorWidget.generated.h"

UCLASS(BlueprintType, Blueprintable)
class CHAMELEONPAINTER_API UChameleonBrushCursorWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UChameleonBrushCursorWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, Category = "Chameleon|Paint")
	void SetPreviewDiameterPixels(float InDiameterPixels);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chameleon|Paint")
	FLinearColor OuterStrokeColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.95f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chameleon|Paint")
	FLinearColor InnerStrokeColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.95f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chameleon|Paint", meta = (ClampMin = "1.0", ClampMax = "12.0"))
	float OuterStrokeThickness = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chameleon|Paint", meta = (ClampMin = "1.0", ClampMax = "8.0"))
	float InnerStrokeThickness = 2.5f;

protected:
	virtual int32 NativePaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override;

private:
	UPROPERTY(Transient)
	float PreviewDiameterPixels = 64.0f;
};

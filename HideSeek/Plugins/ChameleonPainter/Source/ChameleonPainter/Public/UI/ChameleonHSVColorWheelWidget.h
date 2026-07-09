#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "ChameleonHSVColorWheelWidget.generated.h"

class SChameleonHSVColorWheel;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FChameleonHSVColorWheelChanged, FLinearColor, Color);

UCLASS(BlueprintType, Blueprintable)
class CHAMELEONPAINTER_API UChameleonHSVColorWheelWidget : public UWidget
{
	GENERATED_BODY()

public:
	UChameleonHSVColorWheelWidget(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "Chameleon|Color Picker")
	void SetSelectedColor(FLinearColor NewColor);

	UFUNCTION(BlueprintPure, Category = "Chameleon|Color Picker")
	FLinearColor GetSelectedColor() const { return SelectedColor; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chameleon|Color Picker")
	FLinearColor SelectedColor = FLinearColor(1.0f, 0.02f, 0.02f, 1.0f);

	UPROPERTY(BlueprintAssignable, Category = "Chameleon|Color Picker")
	FChameleonHSVColorWheelChanged OnColorChanged;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;
	virtual void SynchronizeProperties() override;

private:
	void HandleSlateColorChanged(FLinearColor NewColor);

	TSharedPtr<SChameleonHSVColorWheel> SlateColorWheel;
};

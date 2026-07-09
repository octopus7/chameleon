#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ChameleonColorPickerWidget.generated.h"

class UBorder;
class UButton;
class UChameleonPaintComponent;
class USlider;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FChameleonColorPickerChanged, FLinearColor, Color);

UCLASS(BlueprintType, Blueprintable)
class CHAMELEONPAINTER_API UChameleonColorPickerWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UChameleonColorPickerWidget(const FObjectInitializer& ObjectInitializer);

	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintCallable, Category = "Chameleon|Color Picker")
	void SetSelectedColor(FLinearColor NewColor, bool bBroadcast = true);

	UFUNCTION(BlueprintPure, Category = "Chameleon|Color Picker")
	FLinearColor GetSelectedColor() const { return SelectedColor; }

	UFUNCTION(BlueprintCallable, Category = "Chameleon|Color Picker")
	void SetTargetPaintComponent(UChameleonPaintComponent* NewTargetPaintComponent);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chameleon|Color Picker")
	FLinearColor SelectedColor = FLinearColor(1.0f, 0.02f, 0.02f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chameleon|Color Picker")
	TArray<FLinearColor> SwatchColors;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chameleon|Color Picker")
	bool bApplyChangesImmediately = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chameleon|Color Picker")
	TObjectPtr<UChameleonPaintComponent> TargetPaintComponent;

	UPROPERTY(BlueprintAssignable, Category = "Chameleon|Color Picker")
	FChameleonColorPickerChanged OnColorChanged;

	UPROPERTY(BlueprintAssignable, Category = "Chameleon|Color Picker")
	FChameleonColorPickerChanged OnColorCommitted;

protected:
	UFUNCTION()
	void HandleRedChanged(float Value);

	UFUNCTION()
	void HandleGreenChanged(float Value);

	UFUNCTION()
	void HandleBlueChanged(float Value);

	UFUNCTION()
	void HandleCommitClicked();

	UFUNCTION()
	void HandleSwatch0Clicked();

	UFUNCTION()
	void HandleSwatch1Clicked();

	UFUNCTION()
	void HandleSwatch2Clicked();

	UFUNCTION()
	void HandleSwatch3Clicked();

	UFUNCTION()
	void HandleSwatch4Clicked();

	UFUNCTION()
	void HandleSwatch5Clicked();

	UFUNCTION()
	void HandleSwatch6Clicked();

	UFUNCTION()
	void HandleSwatch7Clicked();

	UFUNCTION()
	void HandleSwatch8Clicked();

	UFUNCTION()
	void HandleSwatch9Clicked();

	UFUNCTION()
	void HandleSwatch10Clicked();

private:
	void BuildDefaultWidgetTree();
	void BindGeneratedWidgetTree();
	void BindSwatchButton(const FName& WidgetName, int32 SwatchIndex);
	void ApplySelectedColor(bool bBroadcast, bool bCommit);
	void UpdateControlsFromSelectedColor();
	void ChooseSwatch(int32 SwatchIndex);
	UTextBlock* MakeLabel(const FText& Text);
	USlider* MakeSlider(const FName& Name, float Value);
	UButton* MakeSwatchButton(const FName& Name, int32 SwatchIndex);

	UPROPERTY(Transient, meta = (BindWidgetOptional))
	TObjectPtr<UBorder> PreviewBorder;

	UPROPERTY(Transient, meta = (BindWidgetOptional))
	TObjectPtr<USlider> RedSlider;

	UPROPERTY(Transient, meta = (BindWidgetOptional))
	TObjectPtr<USlider> GreenSlider;

	UPROPERTY(Transient, meta = (BindWidgetOptional))
	TObjectPtr<USlider> BlueSlider;

	UPROPERTY(Transient, meta = (BindWidgetOptional))
	TObjectPtr<UButton> CommitButton;

	bool bUpdatingControls = false;
};

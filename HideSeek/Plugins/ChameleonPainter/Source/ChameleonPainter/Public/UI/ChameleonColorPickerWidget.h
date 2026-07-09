#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ChameleonColorPickerWidget.generated.h"

class UBorder;
class UButton;
class UChameleonPaintComponent;
class UChameleonHSVColorWheelWidget;
class USlider;
class USpinBox;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FChameleonColorPickerChanged, FLinearColor, Color);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FChameleonPaintMaterialPropertiesChanged, float, Roughness, float, Metallic);

UCLASS(BlueprintType, Blueprintable)
class CHAMELEONPAINTER_API UChameleonColorPickerWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UChameleonColorPickerWidget(const FObjectInitializer& ObjectInitializer);

	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintCallable, Category = "Chameleon|Color Picker")
	void SetSelectedColor(FLinearColor NewColor, bool bBroadcast = true);

	UFUNCTION(BlueprintCallable, Category = "Chameleon|Color Picker")
	void SetSelectedMaterialProperties(float NewRoughness, float NewMetallic, bool bBroadcast = true);

	UFUNCTION(BlueprintPure, Category = "Chameleon|Color Picker")
	FLinearColor GetSelectedColor() const { return SelectedColor; }

	UFUNCTION(BlueprintCallable, Category = "Chameleon|Color Picker")
	void SetTargetPaintComponent(UChameleonPaintComponent* NewTargetPaintComponent);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chameleon|Color Picker")
	FLinearColor SelectedColor = FLinearColor(1.0f, 0.02f, 0.02f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chameleon|Color Picker", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SelectedRoughness = 0.84f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chameleon|Color Picker", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SelectedMetallic = 0.0f;

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

	UPROPERTY(BlueprintAssignable, Category = "Chameleon|Color Picker")
	FChameleonPaintMaterialPropertiesChanged OnMaterialPropertiesChanged;

	UPROPERTY(BlueprintAssignable, Category = "Chameleon|Color Picker")
	FChameleonPaintMaterialPropertiesChanged OnMaterialPropertiesCommitted;

protected:
	UFUNCTION()
	void HandleRedChanged(float Value);

	UFUNCTION()
	void HandleGreenChanged(float Value);

	UFUNCTION()
	void HandleBlueChanged(float Value);

	UFUNCTION()
	void HandleHueChanged(float Value);

	UFUNCTION()
	void HandleSaturationChanged(float Value);

	UFUNCTION()
	void HandleValueChanged(float Value);

	UFUNCTION()
	void HandleRoughnessChanged(float Value);

	UFUNCTION()
	void HandleMetallicChanged(float Value);

	UFUNCTION()
	void HandleRedValueChanged(float Value);

	UFUNCTION()
	void HandleGreenValueChanged(float Value);

	UFUNCTION()
	void HandleBlueValueChanged(float Value);

	UFUNCTION()
	void HandleHueValueChanged(float Value);

	UFUNCTION()
	void HandleSaturationValueChanged(float Value);

	UFUNCTION()
	void HandleValueValueChanged(float Value);

	UFUNCTION()
	void HandleRoughnessValueChanged(float Value);

	UFUNCTION()
	void HandleMetallicValueChanged(float Value);

	UFUNCTION()
	void HandleHSVWheelColorChanged(FLinearColor NewColor);

	UFUNCTION()
	void HandleCommitClicked();

	UFUNCTION()
	void HandleHistory0Clicked();

	UFUNCTION()
	void HandleHistory1Clicked();

	UFUNCTION()
	void HandleHistory2Clicked();

	UFUNCTION()
	void HandleHistory3Clicked();

	UFUNCTION()
	void HandleHistory4Clicked();

	UFUNCTION()
	void HandleHistory5Clicked();

	UFUNCTION()
	void HandleHistory6Clicked();

	UFUNCTION()
	void HandleHistory7Clicked();

	UFUNCTION()
	void HandleHistory8Clicked();

	UFUNCTION()
	void HandleHistory9Clicked();

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
	void BindHistoryButton(const FName& WidgetName, int32 HistoryIndex);
	void ApplySelectedColor(bool bBroadcast, bool bCommit);
	void ApplySelectedMaterialProperties(bool bBroadcast, bool bCommit);
	void UpdateControlsFromSelectedColor();
	void UpdateHistoryButtons();
	void RecordHistoryColor(FLinearColor NewColor);
	void ChooseSwatch(int32 SwatchIndex);
	void ChooseHistory(int32 HistoryIndex);
	void GetSelectedHSV(float& OutHue, float& OutSaturation, float& OutValue) const;
	void SetSelectedColorFromHSV(float Hue, float Saturation, float Value, bool bBroadcast);
	UTextBlock* MakeLabel(const FText& Text, float FontSize = 18.0f);
	USlider* MakeSlider(const FName& Name, float Value, const FLinearColor& AccentColor);
	USpinBox* MakeValueBox(const FName& Name, float Value);
	UButton* MakeSwatchButton(const FName& Name, int32 SwatchIndex);
	UButton* MakeHistoryButton(const FName& Name, int32 HistoryIndex);

	UPROPERTY(Transient, meta = (BindWidgetOptional))
	TObjectPtr<UBorder> PreviewBorder;

	UPROPERTY(Transient, meta = (BindWidgetOptional))
	TObjectPtr<UChameleonHSVColorWheelWidget> HSVWheel;

	UPROPERTY(Transient, meta = (BindWidgetOptional))
	TObjectPtr<USlider> RedSlider;

	UPROPERTY(Transient, meta = (BindWidgetOptional))
	TObjectPtr<USlider> GreenSlider;

	UPROPERTY(Transient, meta = (BindWidgetOptional))
	TObjectPtr<USlider> BlueSlider;

	UPROPERTY(Transient, meta = (BindWidgetOptional))
	TObjectPtr<USlider> HueSlider;

	UPROPERTY(Transient, meta = (BindWidgetOptional))
	TObjectPtr<USlider> SaturationSlider;

	UPROPERTY(Transient, meta = (BindWidgetOptional))
	TObjectPtr<USlider> ValueSlider;

	UPROPERTY(Transient, meta = (BindWidgetOptional))
	TObjectPtr<USlider> RoughnessSlider;

	UPROPERTY(Transient, meta = (BindWidgetOptional))
	TObjectPtr<USlider> MetallicSlider;

	UPROPERTY(Transient, meta = (BindWidgetOptional))
	TObjectPtr<USpinBox> RedValueBox;

	UPROPERTY(Transient, meta = (BindWidgetOptional))
	TObjectPtr<USpinBox> GreenValueBox;

	UPROPERTY(Transient, meta = (BindWidgetOptional))
	TObjectPtr<USpinBox> BlueValueBox;

	UPROPERTY(Transient, meta = (BindWidgetOptional))
	TObjectPtr<USpinBox> HueValueBox;

	UPROPERTY(Transient, meta = (BindWidgetOptional))
	TObjectPtr<USpinBox> SaturationValueBox;

	UPROPERTY(Transient, meta = (BindWidgetOptional))
	TObjectPtr<USpinBox> ValueValueBox;

	UPROPERTY(Transient, meta = (BindWidgetOptional))
	TObjectPtr<USpinBox> RoughnessValueBox;

	UPROPERTY(Transient, meta = (BindWidgetOptional))
	TObjectPtr<USpinBox> MetallicValueBox;

	UPROPERTY(Transient, meta = (BindWidgetOptional))
	TObjectPtr<UButton> CommitButton;

	UPROPERTY(Transient)
	TArray<FLinearColor> ColorHistory;

	bool bUpdatingControls = false;
};

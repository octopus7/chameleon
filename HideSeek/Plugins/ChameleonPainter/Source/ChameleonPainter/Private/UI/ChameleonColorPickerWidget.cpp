#include "UI/ChameleonColorPickerWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ChameleonPaintComponent.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/Slider.h"
#include "Components/SpinBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "UI/ChameleonHSVColorWheelWidget.h"

namespace
{
constexpr float ColorPickerWidgetWidth = 520.0f;
constexpr float ColorPickerWidgetHeight = 700.0f;
constexpr int32 ColorHistoryCount = 10;

const FLinearColor PanelColor(0.015f, 0.018f, 0.022f, 0.93f);
const FLinearColor GroupColor(0.025f, 0.03f, 0.036f, 0.86f);
const FLinearColor TextColor(0.94f, 0.94f, 0.94f, 1.0f);
const FLinearColor MutedTrackColor(0.34f, 0.34f, 0.34f, 1.0f);
const FLinearColor ValueBoxBackgroundColor(0.055f, 0.06f, 0.068f, 1.0f);
const FLinearColor ValueBoxHoverColor(0.075f, 0.085f, 0.098f, 1.0f);
const FLinearColor ValueBoxActiveColor(0.095f, 0.11f, 0.13f, 1.0f);
const FLinearColor ValueBoxFillColor(0.14f, 0.17f, 0.2f, 1.0f);
const FLinearColor ValueBoxTextColor(0.94f, 0.96f, 1.0f, 1.0f);

float ClampUnit(float Value)
{
	return FMath::Clamp(Value, 0.0f, 1.0f);
}

int32 UnitToByte(float Value)
{
	return FMath::Clamp(FMath::RoundToInt(ClampUnit(Value) * 255.0f), 0, 255);
}

float ByteToUnit(float Value)
{
	return ClampUnit(Value / 255.0f);
}

FLinearColor ClampColor(FLinearColor Color)
{
	Color.R = ClampUnit(Color.R);
	Color.G = ClampUnit(Color.G);
	Color.B = ClampUnit(Color.B);
	Color.A = 1.0f;
	return Color;
}

FLinearColor MakeHSVColor(float Hue, float Saturation, float Value)
{
	FLinearColor HSV(ClampUnit(Hue) * 360.0f, ClampUnit(Saturation), ClampUnit(Value), 1.0f);
	FLinearColor RGB = HSV.HSVToLinearRGB();
	RGB.A = 1.0f;
	return ClampColor(RGB);
}

void ApplySliderAccent(USlider* Slider, const FLinearColor& AccentColor)
{
	if (!Slider)
	{
		return;
	}

	FSliderStyle SliderStyle = Slider->GetWidgetStyle();
	SliderStyle.NormalBarImage.TintColor = FSlateColor(AccentColor);
	SliderStyle.HoveredBarImage.TintColor = FSlateColor(AccentColor);
	SliderStyle.DisabledBarImage.TintColor = FSlateColor(AccentColor.CopyWithNewOpacity(0.35f));
	SliderStyle.NormalThumbImage.TintColor = FSlateColor(FLinearColor::White);
	SliderStyle.HoveredThumbImage.TintColor = FSlateColor(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f));
	SliderStyle.DisabledThumbImage.TintColor = FSlateColor(FLinearColor(0.45f, 0.45f, 0.45f, 1.0f));
	Slider->SetWidgetStyle(SliderStyle);
}

void ApplyValueBoxStyle(USpinBox* ValueBox)
{
	if (!ValueBox)
	{
		return;
	}

	FSpinBoxStyle SpinBoxStyle = ValueBox->GetWidgetStyle();
	SpinBoxStyle.BackgroundBrush.TintColor = FSlateColor(ValueBoxBackgroundColor);
	SpinBoxStyle.HoveredBackgroundBrush.TintColor = FSlateColor(ValueBoxHoverColor);
	SpinBoxStyle.ActiveBackgroundBrush.TintColor = FSlateColor(ValueBoxActiveColor);
	SpinBoxStyle.InactiveFillBrush.TintColor = FSlateColor(ValueBoxFillColor);
	SpinBoxStyle.HoveredFillBrush.TintColor = FSlateColor(ValueBoxFillColor.CopyWithNewOpacity(0.92f));
	SpinBoxStyle.ActiveFillBrush.TintColor = FSlateColor(ValueBoxFillColor.CopyWithNewOpacity(0.95f));
	SpinBoxStyle.ArrowsImage.TintColor = FSlateColor(ValueBoxTextColor.CopyWithNewOpacity(0.78f));
	SpinBoxStyle.SetForegroundColor(FSlateColor(ValueBoxTextColor));
	SpinBoxStyle.SetTextPadding(FMargin(4.0f, 1.0f));
	ValueBox->SetWidgetStyle(SpinBoxStyle);
	ValueBox->SetForegroundColor(FSlateColor(ValueBoxTextColor));
}
}

UChameleonColorPickerWidget::UChameleonColorPickerWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SwatchColors = {
		FLinearColor(1.0f, 0.02f, 0.02f, 1.0f),
		FLinearColor(1.0f, 0.42f, 0.0f, 1.0f),
		FLinearColor(1.0f, 0.92f, 0.0f, 1.0f),
		FLinearColor(0.05f, 1.0f, 0.08f, 1.0f),
		FLinearColor(0.0f, 0.85f, 1.0f, 1.0f),
		FLinearColor(0.02f, 0.18f, 1.0f, 1.0f),
		FLinearColor(0.58f, 0.0f, 1.0f, 1.0f),
		FLinearColor(1.0f, 0.0f, 0.65f, 1.0f),
		FLinearColor::White,
		FLinearColor::Black,
		FLinearColor(0.5f, 0.5f, 0.5f, 1.0f)
	};

	ColorHistory = {
		FLinearColor(1.0f, 0.02f, 0.02f, 1.0f),
		FLinearColor(1.0f, 0.28f, 0.02f, 1.0f),
		FLinearColor(1.0f, 0.62f, 0.02f, 1.0f),
		FLinearColor(1.0f, 0.88f, 0.02f, 1.0f),
		FLinearColor(0.42f, 0.9f, 0.14f, 1.0f),
		FLinearColor(0.12f, 0.72f, 0.56f, 1.0f),
		FLinearColor(0.0f, 0.82f, 0.9f, 1.0f),
		FLinearColor(0.02f, 0.28f, 1.0f, 1.0f),
		FLinearColor(0.7f, 0.04f, 1.0f, 1.0f),
		FLinearColor(1.0f, 0.0f, 0.55f, 1.0f)
	};
}

void UChameleonColorPickerWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildDefaultWidgetTree();
	}

	BindGeneratedWidgetTree();
	UpdateControlsFromSelectedColor();
	UpdateEyedropperButtonState();
	UpdateHistoryButtons();
	ApplySelectedColor(false, false);
	ApplySelectedMaterialProperties(false, false);
}

void UChameleonColorPickerWidget::BindGeneratedWidgetTree()
{
	if (!WidgetTree)
	{
		return;
	}

	if (!PreviewBorder)
	{
		PreviewBorder = Cast<UBorder>(WidgetTree->FindWidget(FName(TEXT("ColorPreview"))));
	}
	if (!HSVWheel)
	{
		HSVWheel = Cast<UChameleonHSVColorWheelWidget>(WidgetTree->FindWidget(FName(TEXT("HSVWheel"))));
	}
	if (!RedSlider)
	{
		RedSlider = Cast<USlider>(WidgetTree->FindWidget(FName(TEXT("RedSlider"))));
	}
	if (!GreenSlider)
	{
		GreenSlider = Cast<USlider>(WidgetTree->FindWidget(FName(TEXT("GreenSlider"))));
	}
	if (!BlueSlider)
	{
		BlueSlider = Cast<USlider>(WidgetTree->FindWidget(FName(TEXT("BlueSlider"))));
	}
	if (!HueSlider)
	{
		HueSlider = Cast<USlider>(WidgetTree->FindWidget(FName(TEXT("HueSlider"))));
	}
	if (!SaturationSlider)
	{
		SaturationSlider = Cast<USlider>(WidgetTree->FindWidget(FName(TEXT("SaturationSlider"))));
	}
	if (!ValueSlider)
	{
		ValueSlider = Cast<USlider>(WidgetTree->FindWidget(FName(TEXT("ValueSlider"))));
	}
	if (!RoughnessSlider)
	{
		RoughnessSlider = Cast<USlider>(WidgetTree->FindWidget(FName(TEXT("RoughnessSlider"))));
	}
	if (!MetallicSlider)
	{
		MetallicSlider = Cast<USlider>(WidgetTree->FindWidget(FName(TEXT("MetallicSlider"))));
	}
	if (!RedValueBox)
	{
		RedValueBox = Cast<USpinBox>(WidgetTree->FindWidget(FName(TEXT("RedValueBox"))));
	}
	if (!GreenValueBox)
	{
		GreenValueBox = Cast<USpinBox>(WidgetTree->FindWidget(FName(TEXT("GreenValueBox"))));
	}
	if (!BlueValueBox)
	{
		BlueValueBox = Cast<USpinBox>(WidgetTree->FindWidget(FName(TEXT("BlueValueBox"))));
	}
	if (!HueValueBox)
	{
		HueValueBox = Cast<USpinBox>(WidgetTree->FindWidget(FName(TEXT("HueValueBox"))));
	}
	if (!SaturationValueBox)
	{
		SaturationValueBox = Cast<USpinBox>(WidgetTree->FindWidget(FName(TEXT("SaturationValueBox"))));
	}
	if (!ValueValueBox)
	{
		ValueValueBox = Cast<USpinBox>(WidgetTree->FindWidget(FName(TEXT("ValueValueBox"))));
	}
	if (!RoughnessValueBox)
	{
		RoughnessValueBox = Cast<USpinBox>(WidgetTree->FindWidget(FName(TEXT("RoughnessValueBox"))));
	}
	if (!MetallicValueBox)
	{
		MetallicValueBox = Cast<USpinBox>(WidgetTree->FindWidget(FName(TEXT("MetallicValueBox"))));
	}
	if (!CommitButton)
	{
		CommitButton = Cast<UButton>(WidgetTree->FindWidget(FName(TEXT("CommitButton"))));
	}
	if (!EyedropperButton)
	{
		EyedropperButton = Cast<UButton>(WidgetTree->FindWidget(FName(TEXT("EyedropperButton"))));
	}
	if (!EyedropperButton)
	{
		if (UHorizontalBox* HeaderRow = Cast<UHorizontalBox>(WidgetTree->FindWidget(FName(TEXT("HeaderRow")))))
		{
			EyedropperButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("EyedropperButton"));
			EyedropperButton->SetContent(MakeLabel(FText::FromString(TEXT("Pick")), 15.0f));
			EyedropperButton->SetToolTipText(FText::FromString(TEXT("Sample color")));
			USizeBox* EyedropperSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("EyedropperButtonSize"));
			EyedropperSize->SetWidthOverride(54.0f);
			EyedropperSize->SetHeightOverride(42.0f);
			EyedropperSize->SetContent(EyedropperButton);
			HeaderRow->AddChildToHorizontalBox(EyedropperSize);
		}
	}

	ApplyValueBoxStyle(RedValueBox);
	ApplyValueBoxStyle(GreenValueBox);
	ApplyValueBoxStyle(BlueValueBox);
	ApplyValueBoxStyle(HueValueBox);
	ApplyValueBoxStyle(SaturationValueBox);
	ApplyValueBoxStyle(ValueValueBox);
	ApplyValueBoxStyle(RoughnessValueBox);
	ApplyValueBoxStyle(MetallicValueBox);

	if (HSVWheel)
	{
		HSVWheel->OnColorChanged.AddUniqueDynamic(this, &UChameleonColorPickerWidget::HandleHSVWheelColorChanged);
	}
	if (RedSlider)
	{
		RedSlider->OnValueChanged.AddUniqueDynamic(this, &UChameleonColorPickerWidget::HandleRedChanged);
	}
	if (GreenSlider)
	{
		GreenSlider->OnValueChanged.AddUniqueDynamic(this, &UChameleonColorPickerWidget::HandleGreenChanged);
	}
	if (BlueSlider)
	{
		BlueSlider->OnValueChanged.AddUniqueDynamic(this, &UChameleonColorPickerWidget::HandleBlueChanged);
	}
	if (HueSlider)
	{
		HueSlider->OnValueChanged.AddUniqueDynamic(this, &UChameleonColorPickerWidget::HandleHueChanged);
	}
	if (SaturationSlider)
	{
		SaturationSlider->OnValueChanged.AddUniqueDynamic(this, &UChameleonColorPickerWidget::HandleSaturationChanged);
	}
	if (ValueSlider)
	{
		ValueSlider->OnValueChanged.AddUniqueDynamic(this, &UChameleonColorPickerWidget::HandleValueChanged);
	}
	if (RoughnessSlider)
	{
		RoughnessSlider->OnValueChanged.AddUniqueDynamic(this, &UChameleonColorPickerWidget::HandleRoughnessChanged);
	}
	if (MetallicSlider)
	{
		MetallicSlider->OnValueChanged.AddUniqueDynamic(this, &UChameleonColorPickerWidget::HandleMetallicChanged);
	}
	if (RedValueBox)
	{
		RedValueBox->OnValueChanged.AddUniqueDynamic(this, &UChameleonColorPickerWidget::HandleRedValueChanged);
	}
	if (GreenValueBox)
	{
		GreenValueBox->OnValueChanged.AddUniqueDynamic(this, &UChameleonColorPickerWidget::HandleGreenValueChanged);
	}
	if (BlueValueBox)
	{
		BlueValueBox->OnValueChanged.AddUniqueDynamic(this, &UChameleonColorPickerWidget::HandleBlueValueChanged);
	}
	if (HueValueBox)
	{
		HueValueBox->OnValueChanged.AddUniqueDynamic(this, &UChameleonColorPickerWidget::HandleHueValueChanged);
	}
	if (SaturationValueBox)
	{
		SaturationValueBox->OnValueChanged.AddUniqueDynamic(this, &UChameleonColorPickerWidget::HandleSaturationValueChanged);
	}
	if (ValueValueBox)
	{
		ValueValueBox->OnValueChanged.AddUniqueDynamic(this, &UChameleonColorPickerWidget::HandleValueValueChanged);
	}
	if (RoughnessValueBox)
	{
		RoughnessValueBox->OnValueChanged.AddUniqueDynamic(this, &UChameleonColorPickerWidget::HandleRoughnessValueChanged);
	}
	if (MetallicValueBox)
	{
		MetallicValueBox->OnValueChanged.AddUniqueDynamic(this, &UChameleonColorPickerWidget::HandleMetallicValueChanged);
	}
	if (CommitButton)
	{
		CommitButton->OnClicked.AddUniqueDynamic(this, &UChameleonColorPickerWidget::HandleCommitClicked);
	}
	if (EyedropperButton)
	{
		EyedropperButton->OnClicked.AddUniqueDynamic(this, &UChameleonColorPickerWidget::HandleEyedropperClicked);
	}

	for (int32 Index = 0; Index < ColorHistoryCount; ++Index)
	{
		BindHistoryButton(FName(*FString::Printf(TEXT("HistorySwatch%d"), Index)), Index);
	}
	for (int32 Index = 0; Index < SwatchColors.Num(); ++Index)
	{
		BindSwatchButton(FName(*FString::Printf(TEXT("Swatch%d"), Index)), Index);
	}
}

void UChameleonColorPickerWidget::BindSwatchButton(const FName& WidgetName, int32 SwatchIndex)
{
	UButton* SwatchButton = WidgetTree ? Cast<UButton>(WidgetTree->FindWidget(WidgetName)) : nullptr;
	if (!SwatchButton)
	{
		return;
	}

	switch (SwatchIndex)
	{
	case 0: SwatchButton->OnClicked.AddUniqueDynamic(this, &UChameleonColorPickerWidget::HandleSwatch0Clicked); break;
	case 1: SwatchButton->OnClicked.AddUniqueDynamic(this, &UChameleonColorPickerWidget::HandleSwatch1Clicked); break;
	case 2: SwatchButton->OnClicked.AddUniqueDynamic(this, &UChameleonColorPickerWidget::HandleSwatch2Clicked); break;
	case 3: SwatchButton->OnClicked.AddUniqueDynamic(this, &UChameleonColorPickerWidget::HandleSwatch3Clicked); break;
	case 4: SwatchButton->OnClicked.AddUniqueDynamic(this, &UChameleonColorPickerWidget::HandleSwatch4Clicked); break;
	case 5: SwatchButton->OnClicked.AddUniqueDynamic(this, &UChameleonColorPickerWidget::HandleSwatch5Clicked); break;
	case 6: SwatchButton->OnClicked.AddUniqueDynamic(this, &UChameleonColorPickerWidget::HandleSwatch6Clicked); break;
	case 7: SwatchButton->OnClicked.AddUniqueDynamic(this, &UChameleonColorPickerWidget::HandleSwatch7Clicked); break;
	case 8: SwatchButton->OnClicked.AddUniqueDynamic(this, &UChameleonColorPickerWidget::HandleSwatch8Clicked); break;
	case 9: SwatchButton->OnClicked.AddUniqueDynamic(this, &UChameleonColorPickerWidget::HandleSwatch9Clicked); break;
	case 10: SwatchButton->OnClicked.AddUniqueDynamic(this, &UChameleonColorPickerWidget::HandleSwatch10Clicked); break;
	default: break;
	}
}

void UChameleonColorPickerWidget::BindHistoryButton(const FName& WidgetName, int32 HistoryIndex)
{
	UButton* HistoryButton = WidgetTree ? Cast<UButton>(WidgetTree->FindWidget(WidgetName)) : nullptr;
	if (!HistoryButton)
	{
		return;
	}

	switch (HistoryIndex)
	{
	case 0: HistoryButton->OnClicked.AddUniqueDynamic(this, &UChameleonColorPickerWidget::HandleHistory0Clicked); break;
	case 1: HistoryButton->OnClicked.AddUniqueDynamic(this, &UChameleonColorPickerWidget::HandleHistory1Clicked); break;
	case 2: HistoryButton->OnClicked.AddUniqueDynamic(this, &UChameleonColorPickerWidget::HandleHistory2Clicked); break;
	case 3: HistoryButton->OnClicked.AddUniqueDynamic(this, &UChameleonColorPickerWidget::HandleHistory3Clicked); break;
	case 4: HistoryButton->OnClicked.AddUniqueDynamic(this, &UChameleonColorPickerWidget::HandleHistory4Clicked); break;
	case 5: HistoryButton->OnClicked.AddUniqueDynamic(this, &UChameleonColorPickerWidget::HandleHistory5Clicked); break;
	case 6: HistoryButton->OnClicked.AddUniqueDynamic(this, &UChameleonColorPickerWidget::HandleHistory6Clicked); break;
	case 7: HistoryButton->OnClicked.AddUniqueDynamic(this, &UChameleonColorPickerWidget::HandleHistory7Clicked); break;
	case 8: HistoryButton->OnClicked.AddUniqueDynamic(this, &UChameleonColorPickerWidget::HandleHistory8Clicked); break;
	case 9: HistoryButton->OnClicked.AddUniqueDynamic(this, &UChameleonColorPickerWidget::HandleHistory9Clicked); break;
	default: break;
	}
}

void UChameleonColorPickerWidget::SetSelectedColor(FLinearColor NewColor, bool bBroadcast)
{
	SelectedColor = ClampColor(NewColor);
	UpdateControlsFromSelectedColor();
	ApplySelectedColor(bBroadcast, false);
}

void UChameleonColorPickerWidget::SetSelectedMaterialProperties(float NewRoughness, float NewMetallic, bool bBroadcast)
{
	SelectedRoughness = ClampUnit(NewRoughness);
	SelectedMetallic = ClampUnit(NewMetallic);

	UpdateControlsFromSelectedColor();
	ApplySelectedMaterialProperties(bBroadcast, false);
}

void UChameleonColorPickerWidget::SetEyedropperModeActive(bool bActive, bool bBroadcast)
{
	if (bEyedropperModeActive == bActive)
	{
		UpdateEyedropperButtonState();
		return;
	}

	bEyedropperModeActive = bActive;
	UpdateEyedropperButtonState();
	if (bBroadcast)
	{
		OnEyedropperModeChanged.Broadcast(bEyedropperModeActive);
	}
}

void UChameleonColorPickerWidget::SetTargetPaintComponent(UChameleonPaintComponent* NewTargetPaintComponent)
{
	TargetPaintComponent = NewTargetPaintComponent;
	ApplySelectedColor(false, false);
}

void UChameleonColorPickerWidget::HandleRedChanged(float Value)
{
	if (bUpdatingControls)
	{
		return;
	}

	SelectedColor.R = ClampUnit(Value);
	ApplySelectedColor(true, false);
	UpdateControlsFromSelectedColor();
}

void UChameleonColorPickerWidget::HandleGreenChanged(float Value)
{
	if (bUpdatingControls)
	{
		return;
	}

	SelectedColor.G = ClampUnit(Value);
	ApplySelectedColor(true, false);
	UpdateControlsFromSelectedColor();
}

void UChameleonColorPickerWidget::HandleBlueChanged(float Value)
{
	if (bUpdatingControls)
	{
		return;
	}

	SelectedColor.B = ClampUnit(Value);
	ApplySelectedColor(true, false);
	UpdateControlsFromSelectedColor();
}

void UChameleonColorPickerWidget::HandleHueChanged(float Value)
{
	if (bUpdatingControls)
	{
		return;
	}

	float Hue = 0.0f;
	float Saturation = 0.0f;
	float ColorValue = 0.0f;
	GetSelectedHSV(Hue, Saturation, ColorValue);
	SetSelectedColorFromHSV(Value, Saturation, ColorValue, true);
}

void UChameleonColorPickerWidget::HandleSaturationChanged(float Value)
{
	if (bUpdatingControls)
	{
		return;
	}

	float Hue = 0.0f;
	float Saturation = 0.0f;
	float ColorValue = 0.0f;
	GetSelectedHSV(Hue, Saturation, ColorValue);
	SetSelectedColorFromHSV(Hue, Value, ColorValue, true);
}

void UChameleonColorPickerWidget::HandleValueChanged(float Value)
{
	if (bUpdatingControls)
	{
		return;
	}

	float Hue = 0.0f;
	float Saturation = 0.0f;
	float ColorValue = 0.0f;
	GetSelectedHSV(Hue, Saturation, ColorValue);
	SetSelectedColorFromHSV(Hue, Saturation, Value, true);
}

void UChameleonColorPickerWidget::HandleRoughnessChanged(float Value)
{
	if (bUpdatingControls)
	{
		return;
	}

	SelectedRoughness = ClampUnit(Value);
	ApplySelectedMaterialProperties(true, false);
	UpdateControlsFromSelectedColor();
}

void UChameleonColorPickerWidget::HandleMetallicChanged(float Value)
{
	if (bUpdatingControls)
	{
		return;
	}

	SelectedMetallic = ClampUnit(Value);
	ApplySelectedMaterialProperties(true, false);
	UpdateControlsFromSelectedColor();
}

void UChameleonColorPickerWidget::HandleRedValueChanged(float Value)
{
	if (bUpdatingControls)
	{
		return;
	}

	SelectedColor.R = ByteToUnit(Value);
	ApplySelectedColor(true, false);
	UpdateControlsFromSelectedColor();
}

void UChameleonColorPickerWidget::HandleGreenValueChanged(float Value)
{
	if (bUpdatingControls)
	{
		return;
	}

	SelectedColor.G = ByteToUnit(Value);
	ApplySelectedColor(true, false);
	UpdateControlsFromSelectedColor();
}

void UChameleonColorPickerWidget::HandleBlueValueChanged(float Value)
{
	if (bUpdatingControls)
	{
		return;
	}

	SelectedColor.B = ByteToUnit(Value);
	ApplySelectedColor(true, false);
	UpdateControlsFromSelectedColor();
}

void UChameleonColorPickerWidget::HandleHueValueChanged(float Value)
{
	if (bUpdatingControls)
	{
		return;
	}

	float Hue = 0.0f;
	float Saturation = 0.0f;
	float ColorValue = 0.0f;
	GetSelectedHSV(Hue, Saturation, ColorValue);
	SetSelectedColorFromHSV(ByteToUnit(Value), Saturation, ColorValue, true);
}

void UChameleonColorPickerWidget::HandleSaturationValueChanged(float Value)
{
	if (bUpdatingControls)
	{
		return;
	}

	float Hue = 0.0f;
	float Saturation = 0.0f;
	float ColorValue = 0.0f;
	GetSelectedHSV(Hue, Saturation, ColorValue);
	SetSelectedColorFromHSV(Hue, ByteToUnit(Value), ColorValue, true);
}

void UChameleonColorPickerWidget::HandleValueValueChanged(float Value)
{
	if (bUpdatingControls)
	{
		return;
	}

	float Hue = 0.0f;
	float Saturation = 0.0f;
	float ColorValue = 0.0f;
	GetSelectedHSV(Hue, Saturation, ColorValue);
	SetSelectedColorFromHSV(Hue, Saturation, ByteToUnit(Value), true);
}

void UChameleonColorPickerWidget::HandleRoughnessValueChanged(float Value)
{
	if (bUpdatingControls)
	{
		return;
	}

	SelectedRoughness = ByteToUnit(Value);
	ApplySelectedMaterialProperties(true, false);
	UpdateControlsFromSelectedColor();
}

void UChameleonColorPickerWidget::HandleMetallicValueChanged(float Value)
{
	if (bUpdatingControls)
	{
		return;
	}

	SelectedMetallic = ByteToUnit(Value);
	ApplySelectedMaterialProperties(true, false);
	UpdateControlsFromSelectedColor();
}

void UChameleonColorPickerWidget::HandleHSVWheelColorChanged(FLinearColor NewColor)
{
	if (bUpdatingControls)
	{
		return;
	}

	SelectedColor = ClampColor(NewColor);
	ApplySelectedColor(true, false);
	UpdateControlsFromSelectedColor();
}

void UChameleonColorPickerWidget::HandleCommitClicked()
{
	RecordHistoryColor(SelectedColor);
	ApplySelectedColor(true, true);
	ApplySelectedMaterialProperties(true, true);
}

void UChameleonColorPickerWidget::HandleEyedropperClicked()
{
	SetEyedropperModeActive(!bEyedropperModeActive, true);
}

void UChameleonColorPickerWidget::HandleHistory0Clicked() { ChooseHistory(0); }
void UChameleonColorPickerWidget::HandleHistory1Clicked() { ChooseHistory(1); }
void UChameleonColorPickerWidget::HandleHistory2Clicked() { ChooseHistory(2); }
void UChameleonColorPickerWidget::HandleHistory3Clicked() { ChooseHistory(3); }
void UChameleonColorPickerWidget::HandleHistory4Clicked() { ChooseHistory(4); }
void UChameleonColorPickerWidget::HandleHistory5Clicked() { ChooseHistory(5); }
void UChameleonColorPickerWidget::HandleHistory6Clicked() { ChooseHistory(6); }
void UChameleonColorPickerWidget::HandleHistory7Clicked() { ChooseHistory(7); }
void UChameleonColorPickerWidget::HandleHistory8Clicked() { ChooseHistory(8); }
void UChameleonColorPickerWidget::HandleHistory9Clicked() { ChooseHistory(9); }

void UChameleonColorPickerWidget::HandleSwatch0Clicked() { ChooseSwatch(0); }
void UChameleonColorPickerWidget::HandleSwatch1Clicked() { ChooseSwatch(1); }
void UChameleonColorPickerWidget::HandleSwatch2Clicked() { ChooseSwatch(2); }
void UChameleonColorPickerWidget::HandleSwatch3Clicked() { ChooseSwatch(3); }
void UChameleonColorPickerWidget::HandleSwatch4Clicked() { ChooseSwatch(4); }
void UChameleonColorPickerWidget::HandleSwatch5Clicked() { ChooseSwatch(5); }
void UChameleonColorPickerWidget::HandleSwatch6Clicked() { ChooseSwatch(6); }
void UChameleonColorPickerWidget::HandleSwatch7Clicked() { ChooseSwatch(7); }
void UChameleonColorPickerWidget::HandleSwatch8Clicked() { ChooseSwatch(8); }
void UChameleonColorPickerWidget::HandleSwatch9Clicked() { ChooseSwatch(9); }
void UChameleonColorPickerWidget::HandleSwatch10Clicked() { ChooseSwatch(10); }

void UChameleonColorPickerWidget::BuildDefaultWidgetTree()
{
	if (!WidgetTree)
	{
		return;
	}

	USizeBox* ColorPickerSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ColorPickerSize"));
	ColorPickerSize->SetWidthOverride(ColorPickerWidgetWidth);
	ColorPickerSize->SetHeightOverride(ColorPickerWidgetHeight);
	WidgetTree->RootWidget = ColorPickerSize;

	UBorder* PanelBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BrushColorPanel"));
	PanelBorder->SetBrushColor(PanelColor);
	PanelBorder->SetPadding(FMargin(14.0f));
	ColorPickerSize->SetContent(PanelBorder);

	UVerticalBox* RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ColorPickerRoot"));
	PanelBorder->SetContent(RootBox);

	UHorizontalBox* HeaderRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HeaderRow"));
	UTextBlock* TitleText = MakeLabel(FText::FromString(TEXT("Paint")), 31.0f);
	if (UHorizontalBoxSlot* TitleSlot = HeaderRow->AddChildToHorizontalBox(TitleText))
	{
		TitleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}
	EyedropperButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("EyedropperButton"));
	EyedropperButton->SetContent(MakeLabel(FText::FromString(TEXT("Pick")), 15.0f));
	EyedropperButton->SetToolTipText(FText::FromString(TEXT("Sample color")));
	USizeBox* EyedropperSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("EyedropperButtonSize"));
	EyedropperSize->SetWidthOverride(54.0f);
	EyedropperSize->SetHeightOverride(42.0f);
	EyedropperSize->SetContent(EyedropperButton);
	HeaderRow->AddChildToHorizontalBox(EyedropperSize);
	if (UVerticalBoxSlot* TitleSlot = RootBox->AddChildToVerticalBox(HeaderRow))
	{
		TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
	}

	USizeBox* PreviewSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PreviewSize"));
	PreviewSizeBox->SetHeightOverride(44.0f);
	PreviewBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ColorPreview"));
	PreviewBorder->SetBrushColor(SelectedColor);
	PreviewSizeBox->SetContent(PreviewBorder);
	if (UVerticalBoxSlot* PreviewSlot = RootBox->AddChildToVerticalBox(PreviewSizeBox))
	{
		PreviewSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 14.0f));
	}

	UHorizontalBox* PickerAndHistoryRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("PickerAndHistoryRow"));
	USizeBox* WheelSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("HSVWheelSize"));
	WheelSize->SetWidthOverride(226.0f);
	WheelSize->SetHeightOverride(226.0f);
	HSVWheel = WidgetTree->ConstructWidget<UChameleonHSVColorWheelWidget>(UChameleonHSVColorWheelWidget::StaticClass(), TEXT("HSVWheel"));
	HSVWheel->SetSelectedColor(SelectedColor);
	WheelSize->SetContent(HSVWheel);
	if (UHorizontalBoxSlot* WheelSlot = PickerAndHistoryRow->AddChildToHorizontalBox(WheelSize))
	{
		WheelSlot->SetPadding(FMargin(0.0f, 0.0f, 14.0f, 0.0f));
	}

	UBorder* HistoryBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("HistoryPanel"));
	HistoryBorder->SetBrushColor(GroupColor);
	HistoryBorder->SetPadding(FMargin(10.0f));
	UVerticalBox* HistoryBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("HistoryBox"));
	HistoryBorder->SetContent(HistoryBox);
	HistoryBox->AddChildToVerticalBox(MakeLabel(FText::FromString(TEXT("History")), 18.0f));
	for (int32 RowIndex = 0; RowIndex < 2; ++RowIndex)
	{
		UHorizontalBox* HistoryRow = WidgetTree->ConstructWidget<UHorizontalBox>(
			UHorizontalBox::StaticClass(),
			FName(*FString::Printf(TEXT("HistoryRow%d"), RowIndex)));
		for (int32 ColumnIndex = 0; ColumnIndex < 5; ++ColumnIndex)
		{
			const int32 HistoryIndex = RowIndex * 5 + ColumnIndex;
			USizeBox* SwatchSize = WidgetTree->ConstructWidget<USizeBox>(
				USizeBox::StaticClass(),
				FName(*FString::Printf(TEXT("HistorySwatchSize%d"), HistoryIndex)));
			SwatchSize->SetWidthOverride(38.0f);
			SwatchSize->SetHeightOverride(38.0f);
			SwatchSize->SetContent(MakeHistoryButton(FName(*FString::Printf(TEXT("HistorySwatch%d"), HistoryIndex)), HistoryIndex));
			if (UHorizontalBoxSlot* SwatchSlot = HistoryRow->AddChildToHorizontalBox(SwatchSize))
			{
				SwatchSlot->SetPadding(FMargin(0.0f, 7.0f, 7.0f, 0.0f));
			}
		}
		HistoryBox->AddChildToVerticalBox(HistoryRow);
	}
	if (UHorizontalBoxSlot* HistorySlot = PickerAndHistoryRow->AddChildToHorizontalBox(HistoryBorder))
	{
		HistorySlot->SetPadding(FMargin(0.0f));
	}
	if (UVerticalBoxSlot* PickerRowSlot = RootBox->AddChildToVerticalBox(PickerAndHistoryRow))
	{
		PickerRowSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
	}

	UBorder* SwatchesBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SwatchesPanel"));
	SwatchesBorder->SetBrushColor(GroupColor);
	SwatchesBorder->SetPadding(FMargin(10.0f, 8.0f));
	UVerticalBox* SwatchesBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SwatchesBox"));
	SwatchesBorder->SetContent(SwatchesBox);
	SwatchesBox->AddChildToVerticalBox(MakeLabel(FText::FromString(TEXT("Swatches")), 17.0f));
	UHorizontalBox* SwatchRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Swatches"));
	for (int32 Index = 0; Index < SwatchColors.Num(); ++Index)
	{
		const bool bLargeSwatch = Index == 8 || Index == 9;
		USizeBox* SwatchSizeBox = WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(),
			FName(*FString::Printf(TEXT("SwatchSize%d"), Index)));
		SwatchSizeBox->SetWidthOverride(bLargeSwatch ? 54.0f : 30.0f);
		SwatchSizeBox->SetHeightOverride(bLargeSwatch ? 54.0f : 30.0f);
		SwatchSizeBox->SetContent(MakeSwatchButton(FName(*FString::Printf(TEXT("Swatch%d"), Index)), Index));
		if (UHorizontalBoxSlot* SwatchSlot = SwatchRow->AddChildToHorizontalBox(SwatchSizeBox))
		{
			SwatchSlot->SetPadding(FMargin(0.0f, 7.0f, 6.0f, 0.0f));
		}
	}
	SwatchesBox->AddChildToVerticalBox(SwatchRow);
	if (UVerticalBoxSlot* SwatchesSlot = RootBox->AddChildToVerticalBox(SwatchesBorder))
	{
		SwatchesSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
	}

	UBorder* ColorSlidersBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ColorSlidersPanel"));
	ColorSlidersBorder->SetBrushColor(GroupColor);
	ColorSlidersBorder->SetPadding(FMargin(10.0f));
	UHorizontalBox* SliderColumns = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("SliderColumns"));
	ColorSlidersBorder->SetContent(SliderColumns);
	UVerticalBox* RGBColumn = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RGBColumn"));
	UVerticalBox* HSVColumn = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("HSVColumn"));

	auto AddSliderRow = [this](UVerticalBox* Parent, const FText& Label, const FName& SliderName, const FName& ValueBoxName, float SliderValue, const FLinearColor& Accent) -> TPair<USlider*, USpinBox*>
	{
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(
			UHorizontalBox::StaticClass(),
			FName(*FString::Printf(TEXT("%sRow"), *SliderName.ToString())));
		USizeBox* LabelSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		LabelSizeBox->SetWidthOverride(28.0f);
		LabelSizeBox->SetContent(MakeLabel(Label, 18.0f));
		Row->AddChildToHorizontalBox(LabelSizeBox);

		USizeBox* SliderSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		SliderSizeBox->SetWidthOverride(136.0f);
		USlider* Slider = MakeSlider(SliderName, SliderValue, Accent);
		SliderSizeBox->SetContent(Slider);
		if (UHorizontalBoxSlot* SliderSlot = Row->AddChildToHorizontalBox(SliderSizeBox))
		{
			SliderSlot->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));
		}

		USizeBox* ValueSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		ValueSizeBox->SetWidthOverride(56.0f);
		USpinBox* ValueBox = MakeValueBox(ValueBoxName, UnitToByte(SliderValue));
		ValueSizeBox->SetContent(ValueBox);
		Row->AddChildToHorizontalBox(ValueSizeBox);

		if (UVerticalBoxSlot* RowSlot = Parent->AddChildToVerticalBox(Row))
		{
			RowSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 7.0f));
		}

		return TPair<USlider*, USpinBox*>(Slider, ValueBox);
	};

	const TPair<USlider*, USpinBox*> RedControls = AddSliderRow(RGBColumn, FText::FromString(TEXT("R")), TEXT("RedSlider"), TEXT("RedValueBox"), SelectedColor.R, FLinearColor(1.0f, 0.06f, 0.04f, 1.0f));
	RedSlider = RedControls.Key;
	RedValueBox = RedControls.Value;
	const TPair<USlider*, USpinBox*> GreenControls = AddSliderRow(RGBColumn, FText::FromString(TEXT("G")), TEXT("GreenSlider"), TEXT("GreenValueBox"), SelectedColor.G, FLinearColor(0.0f, 0.9f, 0.05f, 1.0f));
	GreenSlider = GreenControls.Key;
	GreenValueBox = GreenControls.Value;
	const TPair<USlider*, USpinBox*> BlueControls = AddSliderRow(RGBColumn, FText::FromString(TEXT("B")), TEXT("BlueSlider"), TEXT("BlueValueBox"), SelectedColor.B, FLinearColor(0.02f, 0.12f, 1.0f, 1.0f));
	BlueSlider = BlueControls.Key;
	BlueValueBox = BlueControls.Value;

	float Hue = 0.0f;
	float Saturation = 0.0f;
	float ColorValue = 0.0f;
	GetSelectedHSV(Hue, Saturation, ColorValue);
	const TPair<USlider*, USpinBox*> HueControls = AddSliderRow(HSVColumn, FText::FromString(TEXT("H")), TEXT("HueSlider"), TEXT("HueValueBox"), Hue, FLinearColor(1.0f, 0.25f, 0.0f, 1.0f));
	HueSlider = HueControls.Key;
	HueValueBox = HueControls.Value;
	const TPair<USlider*, USpinBox*> SaturationControls = AddSliderRow(HSVColumn, FText::FromString(TEXT("S")), TEXT("SaturationSlider"), TEXT("SaturationValueBox"), Saturation, FLinearColor(1.0f, 0.36f, 0.18f, 1.0f));
	SaturationSlider = SaturationControls.Key;
	SaturationValueBox = SaturationControls.Value;
	const TPair<USlider*, USpinBox*> ValueControls = AddSliderRow(HSVColumn, FText::FromString(TEXT("V")), TEXT("ValueSlider"), TEXT("ValueValueBox"), ColorValue, FLinearColor(0.9f, 0.34f, 0.0f, 1.0f));
	ValueSlider = ValueControls.Key;
	ValueValueBox = ValueControls.Value;

	if (UHorizontalBoxSlot* RGBSlot = SliderColumns->AddChildToHorizontalBox(RGBColumn))
	{
		RGBSlot->SetPadding(FMargin(0.0f, 0.0f, 12.0f, 0.0f));
	}
	SliderColumns->AddChildToHorizontalBox(HSVColumn);
	if (UVerticalBoxSlot* ColorSlidersSlot = RootBox->AddChildToVerticalBox(ColorSlidersBorder))
	{
		ColorSlidersSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
	}

	UBorder* MaterialBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MaterialPanel"));
	MaterialBorder->SetBrushColor(GroupColor);
	MaterialBorder->SetPadding(FMargin(10.0f));
	UVerticalBox* MaterialBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MaterialBox"));
	MaterialBorder->SetContent(MaterialBox);
	auto AddMaterialRow = [this, MaterialBox](const FText& Label, const FName& SliderName, const FName& ValueBoxName, float SliderValue) -> TPair<USlider*, USpinBox*>
	{
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(
			UHorizontalBox::StaticClass(),
			FName(*FString::Printf(TEXT("%sRow"), *SliderName.ToString())));
		USizeBox* LabelSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		LabelSizeBox->SetWidthOverride(52.0f);
		LabelSizeBox->SetContent(MakeLabel(Label, 18.0f));
		Row->AddChildToHorizontalBox(LabelSizeBox);

		USizeBox* SliderSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		SliderSizeBox->SetWidthOverride(342.0f);
		USlider* Slider = MakeSlider(SliderName, SliderValue, MutedTrackColor);
		SliderSizeBox->SetContent(Slider);
		if (UHorizontalBoxSlot* SliderSlot = Row->AddChildToHorizontalBox(SliderSizeBox))
		{
			SliderSlot->SetPadding(FMargin(0.0f, 0.0f, 10.0f, 0.0f));
		}

		USizeBox* ValueSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		ValueSizeBox->SetWidthOverride(60.0f);
		USpinBox* ValueBox = MakeValueBox(ValueBoxName, UnitToByte(SliderValue));
		ValueSizeBox->SetContent(ValueBox);
		Row->AddChildToHorizontalBox(ValueSizeBox);
		if (UVerticalBoxSlot* RowSlot = MaterialBox->AddChildToVerticalBox(Row))
		{
			RowSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 7.0f));
		}

		return TPair<USlider*, USpinBox*>(Slider, ValueBox);
	};
	const TPair<USlider*, USpinBox*> RoughnessControls = AddMaterialRow(FText::FromString(TEXT("Rgh")), TEXT("RoughnessSlider"), TEXT("RoughnessValueBox"), SelectedRoughness);
	RoughnessSlider = RoughnessControls.Key;
	RoughnessValueBox = RoughnessControls.Value;
	const TPair<USlider*, USpinBox*> MetallicControls = AddMaterialRow(FText::FromString(TEXT("Met")), TEXT("MetallicSlider"), TEXT("MetallicValueBox"), SelectedMetallic);
	MetallicSlider = MetallicControls.Key;
	MetallicValueBox = MetallicControls.Value;
	if (UVerticalBoxSlot* MaterialSlot = RootBox->AddChildToVerticalBox(MaterialBorder))
	{
		MaterialSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 14.0f));
	}

	CommitButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CommitButton"));
	CommitButton->SetBackgroundColor(FLinearColor(0.02f, 0.22f, 0.52f, 1.0f));
	CommitButton->SetContent(MakeLabel(FText::FromString(TEXT("Apply")), 28.0f));
	USizeBox* CommitSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("CommitSize"));
	CommitSize->SetHeightOverride(54.0f);
	CommitSize->SetContent(CommitButton);
	RootBox->AddChildToVerticalBox(CommitSize);
}

void UChameleonColorPickerWidget::ApplySelectedColor(bool bBroadcast, bool bCommit)
{
	if (TargetPaintComponent && (bApplyChangesImmediately || bCommit))
	{
		TargetPaintComponent->SetPaintColor(SelectedColor);
	}

	if (bBroadcast)
	{
		OnColorChanged.Broadcast(SelectedColor);
	}

	if (bCommit)
	{
		OnColorCommitted.Broadcast(SelectedColor);
	}
}

void UChameleonColorPickerWidget::ApplySelectedMaterialProperties(bool bBroadcast, bool bCommit)
{
	if (bBroadcast)
	{
		OnMaterialPropertiesChanged.Broadcast(SelectedRoughness, SelectedMetallic);
	}

	if (bCommit)
	{
		OnMaterialPropertiesCommitted.Broadcast(SelectedRoughness, SelectedMetallic);
	}
}

void UChameleonColorPickerWidget::UpdateControlsFromSelectedColor()
{
	TGuardValue<bool> UpdatingGuard(bUpdatingControls, true);

	float Hue = 0.0f;
	float Saturation = 0.0f;
	float ColorValue = 0.0f;
	GetSelectedHSV(Hue, Saturation, ColorValue);

	if (PreviewBorder)
	{
		PreviewBorder->SetBrushColor(SelectedColor);
	}
	if (HSVWheel)
	{
		HSVWheel->SetSelectedColor(SelectedColor);
	}
	if (RedSlider)
	{
		RedSlider->SetValue(SelectedColor.R);
	}
	if (GreenSlider)
	{
		GreenSlider->SetValue(SelectedColor.G);
	}
	if (BlueSlider)
	{
		BlueSlider->SetValue(SelectedColor.B);
	}
	if (HueSlider)
	{
		HueSlider->SetValue(Hue);
	}
	if (SaturationSlider)
	{
		SaturationSlider->SetValue(Saturation);
	}
	if (ValueSlider)
	{
		ValueSlider->SetValue(ColorValue);
	}
	if (RoughnessSlider)
	{
		RoughnessSlider->SetValue(SelectedRoughness);
	}
	if (MetallicSlider)
	{
		MetallicSlider->SetValue(SelectedMetallic);
	}

	if (RedValueBox)
	{
		RedValueBox->SetValue(UnitToByte(SelectedColor.R));
	}
	if (GreenValueBox)
	{
		GreenValueBox->SetValue(UnitToByte(SelectedColor.G));
	}
	if (BlueValueBox)
	{
		BlueValueBox->SetValue(UnitToByte(SelectedColor.B));
	}
	if (HueValueBox)
	{
		HueValueBox->SetValue(UnitToByte(Hue));
	}
	if (SaturationValueBox)
	{
		SaturationValueBox->SetValue(UnitToByte(Saturation));
	}
	if (ValueValueBox)
	{
		ValueValueBox->SetValue(UnitToByte(ColorValue));
	}
	if (RoughnessValueBox)
	{
		RoughnessValueBox->SetValue(UnitToByte(SelectedRoughness));
	}
	if (MetallicValueBox)
	{
		MetallicValueBox->SetValue(UnitToByte(SelectedMetallic));
	}
}

void UChameleonColorPickerWidget::UpdateEyedropperButtonState()
{
	if (!EyedropperButton)
	{
		return;
	}

	EyedropperButton->SetBackgroundColor(bEyedropperModeActive
		? FLinearColor(0.0f, 0.54f, 0.95f, 1.0f)
		: FLinearColor(0.075f, 0.09f, 0.11f, 1.0f));
}

void UChameleonColorPickerWidget::UpdateHistoryButtons()
{
	if (!WidgetTree)
	{
		return;
	}

	for (int32 Index = 0; Index < ColorHistoryCount; ++Index)
	{
		if (UButton* HistoryButton = Cast<UButton>(WidgetTree->FindWidget(FName(*FString::Printf(TEXT("HistorySwatch%d"), Index)))))
		{
			const FLinearColor HistoryColor = ColorHistory.IsValidIndex(Index) ? ColorHistory[Index] : FLinearColor(0.08f, 0.08f, 0.08f, 1.0f);
			HistoryButton->SetBackgroundColor(HistoryColor);
		}
	}
}

void UChameleonColorPickerWidget::RecordHistoryColor(FLinearColor NewColor)
{
	NewColor = ClampColor(NewColor);
	for (int32 Index = ColorHistory.Num() - 1; Index >= 0; --Index)
	{
		if (ColorHistory[Index].Equals(NewColor, 0.002f))
		{
			ColorHistory.RemoveAt(Index);
		}
	}

	ColorHistory.Insert(NewColor, 0);
	while (ColorHistory.Num() > ColorHistoryCount)
	{
		ColorHistory.RemoveAt(ColorHistory.Num() - 1);
	}

	UpdateHistoryButtons();
}

void UChameleonColorPickerWidget::ChooseSwatch(int32 SwatchIndex)
{
	if (SwatchColors.IsValidIndex(SwatchIndex))
	{
		SetSelectedColor(SwatchColors[SwatchIndex], true);
	}
}

void UChameleonColorPickerWidget::ChooseHistory(int32 HistoryIndex)
{
	if (ColorHistory.IsValidIndex(HistoryIndex))
	{
		SetSelectedColor(ColorHistory[HistoryIndex], true);
	}
}

void UChameleonColorPickerWidget::GetSelectedHSV(float& OutHue, float& OutSaturation, float& OutValue) const
{
	const FLinearColor HSV = ClampColor(SelectedColor).LinearRGBToHSV();
	OutHue = ClampUnit(HSV.R / 360.0f);
	OutSaturation = ClampUnit(HSV.G);
	OutValue = ClampUnit(HSV.B);
}

void UChameleonColorPickerWidget::SetSelectedColorFromHSV(float Hue, float Saturation, float Value, bool bBroadcast)
{
	SelectedColor = MakeHSVColor(Hue, Saturation, Value);
	ApplySelectedColor(bBroadcast, false);
	UpdateControlsFromSelectedColor();
}

UTextBlock* UChameleonColorPickerWidget::MakeLabel(const FText& Text, float FontSize)
{
	UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Label->SetText(Text);
	Label->SetColorAndOpacity(FSlateColor(TextColor));
	FSlateFontInfo Font = Label->GetFont();
	Font.Size = FMath::RoundToInt(FontSize);
	Label->SetFont(Font);
	Label->SetShadowOffset(FVector2D(1.0f, 1.0f));
	Label->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.55f));
	return Label;
}

USlider* UChameleonColorPickerWidget::MakeSlider(const FName& Name, float Value, const FLinearColor& AccentColor)
{
	USlider* Slider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass(), Name);
	Slider->SetValue(ClampUnit(Value));
	ApplySliderAccent(Slider, AccentColor);
	return Slider;
}

USpinBox* UChameleonColorPickerWidget::MakeValueBox(const FName& Name, float Value)
{
	USpinBox* ValueBox = WidgetTree->ConstructWidget<USpinBox>(USpinBox::StaticClass(), Name);
	ValueBox->SetMinValue(0.0f);
	ValueBox->SetMaxValue(255.0f);
	ValueBox->SetMinSliderValue(0.0f);
	ValueBox->SetMaxSliderValue(255.0f);
	ValueBox->SetDelta(1.0f);
	ValueBox->SetMinFractionalDigits(0);
	ValueBox->SetMaxFractionalDigits(0);
	ValueBox->SetValue(FMath::Clamp(Value, 0.0f, 255.0f));
	ApplyValueBoxStyle(ValueBox);
	return ValueBox;
}

UButton* UChameleonColorPickerWidget::MakeSwatchButton(const FName& Name, int32 SwatchIndex)
{
	UButton* SwatchButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
	const FLinearColor SwatchColor = SwatchColors.IsValidIndex(SwatchIndex) ? SwatchColors[SwatchIndex] : FLinearColor::Black;
	SwatchButton->SetBackgroundColor(SwatchColor);
	BindSwatchButton(Name, SwatchIndex);
	return SwatchButton;
}

UButton* UChameleonColorPickerWidget::MakeHistoryButton(const FName& Name, int32 HistoryIndex)
{
	UButton* HistoryButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
	const FLinearColor HistoryColor = ColorHistory.IsValidIndex(HistoryIndex) ? ColorHistory[HistoryIndex] : FLinearColor(0.08f, 0.08f, 0.08f, 1.0f);
	HistoryButton->SetBackgroundColor(HistoryColor);
	BindHistoryButton(Name, HistoryIndex);
	return HistoryButton;
}

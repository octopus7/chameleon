#include "UI/ChameleonColorPickerWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ChameleonPaintComponent.h"
#include "Components/HorizontalBox.h"
#include "Components/SizeBox.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"

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
	if (!RoughnessSlider)
	{
		RoughnessSlider = Cast<USlider>(WidgetTree->FindWidget(FName(TEXT("RoughnessSlider"))));
	}
	if (!MetallicSlider)
	{
		MetallicSlider = Cast<USlider>(WidgetTree->FindWidget(FName(TEXT("MetallicSlider"))));
	}
	if (!CommitButton)
	{
		CommitButton = Cast<UButton>(WidgetTree->FindWidget(FName(TEXT("CommitButton"))));
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
	if (RoughnessSlider)
	{
		RoughnessSlider->OnValueChanged.AddUniqueDynamic(this, &UChameleonColorPickerWidget::HandleRoughnessChanged);
	}
	if (MetallicSlider)
	{
		MetallicSlider->OnValueChanged.AddUniqueDynamic(this, &UChameleonColorPickerWidget::HandleMetallicChanged);
	}
	if (CommitButton)
	{
		CommitButton->OnClicked.AddUniqueDynamic(this, &UChameleonColorPickerWidget::HandleCommitClicked);
	}

	BindSwatchButton(FName(TEXT("Swatch0")), 0);
	BindSwatchButton(FName(TEXT("Swatch1")), 1);
	BindSwatchButton(FName(TEXT("Swatch2")), 2);
	BindSwatchButton(FName(TEXT("Swatch3")), 3);
	BindSwatchButton(FName(TEXT("Swatch4")), 4);
	BindSwatchButton(FName(TEXT("Swatch5")), 5);
	BindSwatchButton(FName(TEXT("Swatch6")), 6);
	BindSwatchButton(FName(TEXT("Swatch7")), 7);
	BindSwatchButton(FName(TEXT("Swatch8")), 8);
	BindSwatchButton(FName(TEXT("Swatch9")), 9);
	BindSwatchButton(FName(TEXT("Swatch10")), 10);
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
	case 0:
		SwatchButton->OnClicked.AddUniqueDynamic(this, &UChameleonColorPickerWidget::HandleSwatch0Clicked);
		break;
	case 1:
		SwatchButton->OnClicked.AddUniqueDynamic(this, &UChameleonColorPickerWidget::HandleSwatch1Clicked);
		break;
	case 2:
		SwatchButton->OnClicked.AddUniqueDynamic(this, &UChameleonColorPickerWidget::HandleSwatch2Clicked);
		break;
	case 3:
		SwatchButton->OnClicked.AddUniqueDynamic(this, &UChameleonColorPickerWidget::HandleSwatch3Clicked);
		break;
	case 4:
		SwatchButton->OnClicked.AddUniqueDynamic(this, &UChameleonColorPickerWidget::HandleSwatch4Clicked);
		break;
	case 5:
		SwatchButton->OnClicked.AddUniqueDynamic(this, &UChameleonColorPickerWidget::HandleSwatch5Clicked);
		break;
	case 6:
		SwatchButton->OnClicked.AddUniqueDynamic(this, &UChameleonColorPickerWidget::HandleSwatch6Clicked);
		break;
	case 7:
		SwatchButton->OnClicked.AddUniqueDynamic(this, &UChameleonColorPickerWidget::HandleSwatch7Clicked);
		break;
	case 8:
		SwatchButton->OnClicked.AddUniqueDynamic(this, &UChameleonColorPickerWidget::HandleSwatch8Clicked);
		break;
	case 9:
		SwatchButton->OnClicked.AddUniqueDynamic(this, &UChameleonColorPickerWidget::HandleSwatch9Clicked);
		break;
	case 10:
		SwatchButton->OnClicked.AddUniqueDynamic(this, &UChameleonColorPickerWidget::HandleSwatch10Clicked);
		break;
	default:
		break;
	}
}

void UChameleonColorPickerWidget::SetSelectedColor(FLinearColor NewColor, bool bBroadcast)
{
	NewColor.R = FMath::Clamp(NewColor.R, 0.0f, 1.0f);
	NewColor.G = FMath::Clamp(NewColor.G, 0.0f, 1.0f);
	NewColor.B = FMath::Clamp(NewColor.B, 0.0f, 1.0f);
	NewColor.A = 1.0f;
	SelectedColor = NewColor;

	UpdateControlsFromSelectedColor();
	ApplySelectedColor(bBroadcast, false);
}

void UChameleonColorPickerWidget::SetSelectedMaterialProperties(float NewRoughness, float NewMetallic, bool bBroadcast)
{
	SelectedRoughness = FMath::Clamp(NewRoughness, 0.0f, 1.0f);
	SelectedMetallic = FMath::Clamp(NewMetallic, 0.0f, 1.0f);

	UpdateControlsFromSelectedColor();
	ApplySelectedMaterialProperties(bBroadcast, false);
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

	SelectedColor.R = FMath::Clamp(Value, 0.0f, 1.0f);
	ApplySelectedColor(true, false);
	UpdateControlsFromSelectedColor();
}

void UChameleonColorPickerWidget::HandleGreenChanged(float Value)
{
	if (bUpdatingControls)
	{
		return;
	}

	SelectedColor.G = FMath::Clamp(Value, 0.0f, 1.0f);
	ApplySelectedColor(true, false);
	UpdateControlsFromSelectedColor();
}

void UChameleonColorPickerWidget::HandleBlueChanged(float Value)
{
	if (bUpdatingControls)
	{
		return;
	}

	SelectedColor.B = FMath::Clamp(Value, 0.0f, 1.0f);
	ApplySelectedColor(true, false);
	UpdateControlsFromSelectedColor();
}

void UChameleonColorPickerWidget::HandleRoughnessChanged(float Value)
{
	if (bUpdatingControls)
	{
		return;
	}

	SelectedRoughness = FMath::Clamp(Value, 0.0f, 1.0f);
	ApplySelectedMaterialProperties(true, false);
	UpdateControlsFromSelectedColor();
}

void UChameleonColorPickerWidget::HandleMetallicChanged(float Value)
{
	if (bUpdatingControls)
	{
		return;
	}

	SelectedMetallic = FMath::Clamp(Value, 0.0f, 1.0f);
	ApplySelectedMaterialProperties(true, false);
	UpdateControlsFromSelectedColor();
}

void UChameleonColorPickerWidget::HandleCommitClicked()
{
	ApplySelectedColor(true, true);
	ApplySelectedMaterialProperties(true, true);
}

void UChameleonColorPickerWidget::HandleSwatch0Clicked()
{
	ChooseSwatch(0);
}

void UChameleonColorPickerWidget::HandleSwatch1Clicked()
{
	ChooseSwatch(1);
}

void UChameleonColorPickerWidget::HandleSwatch2Clicked()
{
	ChooseSwatch(2);
}

void UChameleonColorPickerWidget::HandleSwatch3Clicked()
{
	ChooseSwatch(3);
}

void UChameleonColorPickerWidget::HandleSwatch4Clicked()
{
	ChooseSwatch(4);
}

void UChameleonColorPickerWidget::HandleSwatch5Clicked()
{
	ChooseSwatch(5);
}

void UChameleonColorPickerWidget::HandleSwatch6Clicked()
{
	ChooseSwatch(6);
}

void UChameleonColorPickerWidget::HandleSwatch7Clicked()
{
	ChooseSwatch(7);
}

void UChameleonColorPickerWidget::HandleSwatch8Clicked()
{
	ChooseSwatch(8);
}

void UChameleonColorPickerWidget::HandleSwatch9Clicked()
{
	ChooseSwatch(9);
}

void UChameleonColorPickerWidget::HandleSwatch10Clicked()
{
	ChooseSwatch(10);
}

void UChameleonColorPickerWidget::BuildDefaultWidgetTree()
{
	if (!WidgetTree)
	{
		return;
	}

	UBorder* PanelBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BrushColorPanel"));
	PanelBorder->SetBrushColor(FLinearColor(0.02f, 0.025f, 0.028f, 0.92f));
	PanelBorder->SetPadding(FMargin(12.0f));
	WidgetTree->RootWidget = PanelBorder;

	UVerticalBox* RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ColorPickerRoot"));
	PanelBorder->SetContent(RootBox);

	USizeBox* PreviewSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PreviewSize"));
	PreviewSizeBox->SetHeightOverride(30.0f);
	PreviewBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ColorPreview"));
	PreviewSizeBox->SetContent(PreviewBorder);
	RootBox->AddChildToVerticalBox(PreviewSizeBox);

	UHorizontalBox* SwatchRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Swatches"));
	for (int32 Index = 0; Index < SwatchColors.Num(); ++Index)
	{
		USizeBox* SwatchSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		SwatchSizeBox->SetWidthOverride(26.0f);
		SwatchSizeBox->SetHeightOverride(26.0f);
		SwatchSizeBox->SetContent(MakeSwatchButton(FName(*FString::Printf(TEXT("Swatch%d"), Index)), Index));
		SwatchRow->AddChildToHorizontalBox(SwatchSizeBox);
	}
	RootBox->AddChildToVerticalBox(SwatchRow);

	auto AddSliderRow = [this, RootBox](const FText& Label, const FName& SliderName) -> USlider*
	{
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		USizeBox* LabelSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		LabelSizeBox->SetWidthOverride(34.0f);
		LabelSizeBox->SetContent(MakeLabel(Label));
		Row->AddChildToHorizontalBox(LabelSizeBox);

		USlider* Slider = MakeSlider(SliderName, 0.0f);
		Row->AddChildToHorizontalBox(Slider);
		RootBox->AddChildToVerticalBox(Row);
		return Slider;
	};

	RedSlider = AddSliderRow(FText::FromString(TEXT("R")), TEXT("RedSlider"));
	GreenSlider = AddSliderRow(FText::FromString(TEXT("G")), TEXT("GreenSlider"));
	BlueSlider = AddSliderRow(FText::FromString(TEXT("B")), TEXT("BlueSlider"));
	RoughnessSlider = AddSliderRow(FText::FromString(TEXT("Rgh")), TEXT("RoughnessSlider"));
	MetallicSlider = AddSliderRow(FText::FromString(TEXT("Met")), TEXT("MetallicSlider"));

	if (RedSlider)
	{
		RedSlider->OnValueChanged.AddDynamic(this, &UChameleonColorPickerWidget::HandleRedChanged);
	}
	if (GreenSlider)
	{
		GreenSlider->OnValueChanged.AddDynamic(this, &UChameleonColorPickerWidget::HandleGreenChanged);
	}
	if (BlueSlider)
	{
		BlueSlider->OnValueChanged.AddDynamic(this, &UChameleonColorPickerWidget::HandleBlueChanged);
	}
	if (RoughnessSlider)
	{
		RoughnessSlider->OnValueChanged.AddDynamic(this, &UChameleonColorPickerWidget::HandleRoughnessChanged);
	}
	if (MetallicSlider)
	{
		MetallicSlider->OnValueChanged.AddDynamic(this, &UChameleonColorPickerWidget::HandleMetallicChanged);
	}

	CommitButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CommitButton"));
	CommitButton->SetContent(MakeLabel(FText::FromString(TEXT("Apply"))));
	CommitButton->OnClicked.AddDynamic(this, &UChameleonColorPickerWidget::HandleCommitClicked);
	RootBox->AddChildToVerticalBox(CommitButton);
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

	if (PreviewBorder)
	{
		PreviewBorder->SetBrushColor(SelectedColor);
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
	if (RoughnessSlider)
	{
		RoughnessSlider->SetValue(SelectedRoughness);
	}
	if (MetallicSlider)
	{
		MetallicSlider->SetValue(SelectedMetallic);
	}
}

void UChameleonColorPickerWidget::ChooseSwatch(int32 SwatchIndex)
{
	if (SwatchColors.IsValidIndex(SwatchIndex))
	{
		SetSelectedColor(SwatchColors[SwatchIndex], true);
	}
}

UTextBlock* UChameleonColorPickerWidget::MakeLabel(const FText& Text)
{
	UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Label->SetText(Text);
	return Label;
}

USlider* UChameleonColorPickerWidget::MakeSlider(const FName& Name, float Value)
{
	USlider* Slider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass(), Name);
	Slider->SetValue(Value);
	return Slider;
}

UButton* UChameleonColorPickerWidget::MakeSwatchButton(const FName& Name, int32 SwatchIndex)
{
	UButton* SwatchButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
	const FLinearColor SwatchColor = SwatchColors.IsValidIndex(SwatchIndex) ? SwatchColors[SwatchIndex] : FLinearColor::Black;
	SwatchButton->SetBackgroundColor(SwatchColor);

	switch (SwatchIndex)
	{
	case 0:
		SwatchButton->OnClicked.AddDynamic(this, &UChameleonColorPickerWidget::HandleSwatch0Clicked);
		break;
	case 1:
		SwatchButton->OnClicked.AddDynamic(this, &UChameleonColorPickerWidget::HandleSwatch1Clicked);
		break;
	case 2:
		SwatchButton->OnClicked.AddDynamic(this, &UChameleonColorPickerWidget::HandleSwatch2Clicked);
		break;
	case 3:
		SwatchButton->OnClicked.AddDynamic(this, &UChameleonColorPickerWidget::HandleSwatch3Clicked);
		break;
	case 4:
		SwatchButton->OnClicked.AddDynamic(this, &UChameleonColorPickerWidget::HandleSwatch4Clicked);
		break;
	case 5:
		SwatchButton->OnClicked.AddDynamic(this, &UChameleonColorPickerWidget::HandleSwatch5Clicked);
		break;
	case 6:
		SwatchButton->OnClicked.AddDynamic(this, &UChameleonColorPickerWidget::HandleSwatch6Clicked);
		break;
	case 7:
		SwatchButton->OnClicked.AddDynamic(this, &UChameleonColorPickerWidget::HandleSwatch7Clicked);
		break;
	case 8:
		SwatchButton->OnClicked.AddDynamic(this, &UChameleonColorPickerWidget::HandleSwatch8Clicked);
		break;
	case 9:
		SwatchButton->OnClicked.AddDynamic(this, &UChameleonColorPickerWidget::HandleSwatch9Clicked);
		break;
	case 10:
		SwatchButton->OnClicked.AddDynamic(this, &UChameleonColorPickerWidget::HandleSwatch10Clicked);
		break;
	default:
		break;
	}

	return SwatchButton;
}

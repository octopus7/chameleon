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
		FLinearColor(0.84f, 0.82f, 0.76f, 1.0f),
		FLinearColor(0.12f, 0.12f, 0.10f, 1.0f),
		FLinearColor(0.64f, 0.58f, 0.46f, 1.0f),
		FLinearColor(0.38f, 0.48f, 0.34f, 1.0f),
		FLinearColor(0.52f, 0.60f, 0.62f, 1.0f),
		FLinearColor(0.72f, 0.36f, 0.27f, 1.0f),
		FLinearColor(0.28f, 0.31f, 0.43f, 1.0f),
		FLinearColor(0.92f, 0.90f, 0.84f, 1.0f)
	};
}

void UChameleonColorPickerWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildDefaultWidgetTree();
	}

	UpdateControlsFromSelectedColor();
	ApplySelectedColor(false, false);
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

void UChameleonColorPickerWidget::HandleCommitClicked()
{
	ApplySelectedColor(true, true);
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

void UChameleonColorPickerWidget::BuildDefaultWidgetTree()
{
	if (!WidgetTree)
	{
		return;
	}

	UVerticalBox* RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ColorPickerRoot"));
	WidgetTree->RootWidget = RootBox;

	USizeBox* PreviewSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PreviewSize"));
	PreviewSizeBox->SetHeightOverride(30.0f);
	PreviewBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ColorPreview"));
	PreviewSizeBox->SetContent(PreviewBorder);
	RootBox->AddChildToVerticalBox(PreviewSizeBox);

	UHorizontalBox* SwatchRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Swatches"));
	for (int32 Index = 0; Index < 8; ++Index)
	{
		USizeBox* SwatchSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		SwatchSizeBox->SetWidthOverride(34.0f);
		SwatchSizeBox->SetHeightOverride(34.0f);
		SwatchSizeBox->SetContent(MakeSwatchButton(FName(*FString::Printf(TEXT("Swatch%d"), Index)), Index));
		SwatchRow->AddChildToHorizontalBox(SwatchSizeBox);
	}
	RootBox->AddChildToVerticalBox(SwatchRow);

	auto AddSliderRow = [this, RootBox](const FText& Label, const FName& SliderName) -> USlider*
	{
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		USizeBox* LabelSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		LabelSizeBox->SetWidthOverride(24.0f);
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
	default:
		break;
	}

	return SwatchButton;
}

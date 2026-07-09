#include "UI/HideSeekTransitionWidget.h"

#include "Styling/CoreStyle.h"
#include "Widgets/Images/SThrobber.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SVerticalBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

void UHideSeekTransitionWidget::StartTransition(FText InMessage, float InFadeInSeconds, float InHoldSeconds, float InFadeOutSeconds)
{
	Message = MoveTemp(InMessage);
	FadeInSeconds = FMath::Max(0.0f, InFadeInSeconds);
	HoldSeconds = FMath::Max(0.0f, InHoldSeconds);
	FadeOutSeconds = FMath::Max(0.0f, InFadeOutSeconds);
	ElapsedSeconds = 0.0f;
	bPlaying = true;

	SetVisibility(ESlateVisibility::HitTestInvisible);
	if (MessageTextBlock.IsValid())
	{
		MessageTextBlock->SetText(Message);
	}
	ApplyOpacity(FadeInSeconds <= UE_SMALL_NUMBER ? 1.0f : 0.0f);
}

TSharedRef<SWidget> UHideSeekTransitionWidget::RebuildWidget()
{
	const FText InitialMessage = Message.IsEmpty() ? FText::FromString(TEXT("Loading")) : Message;

	return SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SAssignNew(FadeBorder, SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
			.BorderBackgroundColor(FLinearColor::Transparent)
		]
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			.Padding(0.0f, 0.0f, 0.0f, 18.0f)
			[
				SAssignNew(MessageTextBlock, STextBlock)
				.Text(InitialMessage)
				.Font(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 34))
				.ColorAndOpacity(FLinearColor::Transparent)
				.Justification(ETextJustify::Center)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			[
				SAssignNew(LoadingTextBlock, STextBlock)
				.Text(FText::FromString(TEXT("Loading...")))
				.Font(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 18))
				.ColorAndOpacity(FLinearColor::Transparent)
				.Justification(ETextJustify::Center)
			]
		];
}

void UHideSeekTransitionWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);

	FadeBorder.Reset();
	MessageTextBlock.Reset();
	LoadingTextBlock.Reset();
}

void UHideSeekTransitionWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bPlaying)
	{
		return;
	}

	ElapsedSeconds += InDeltaTime;
	const float TotalSeconds = FadeInSeconds + HoldSeconds + FadeOutSeconds;
	float Opacity = 1.0f;
	if (FadeInSeconds > UE_SMALL_NUMBER && ElapsedSeconds < FadeInSeconds)
	{
		Opacity = ElapsedSeconds / FadeInSeconds;
	}
	else if (FadeOutSeconds > UE_SMALL_NUMBER && ElapsedSeconds > FadeInSeconds + HoldSeconds)
	{
		const float FadeOutElapsed = ElapsedSeconds - FadeInSeconds - HoldSeconds;
		Opacity = 1.0f - FMath::Clamp(FadeOutElapsed / FadeOutSeconds, 0.0f, 1.0f);
	}

	ApplyOpacity(FMath::Clamp(Opacity, 0.0f, 1.0f));

	if (TotalSeconds <= UE_SMALL_NUMBER || ElapsedSeconds >= TotalSeconds)
	{
		bPlaying = false;
		RemoveFromParent();
	}
}

void UHideSeekTransitionWidget::ApplyOpacity(float Opacity)
{
	const float ClampedOpacity = FMath::Clamp(Opacity, 0.0f, 1.0f);
	if (FadeBorder.IsValid())
	{
		FadeBorder->SetBorderBackgroundColor(FLinearColor(0.0f, 0.0f, 0.0f, ClampedOpacity * 0.86f));
	}
	if (MessageTextBlock.IsValid())
	{
		MessageTextBlock->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 1.0f, 1.0f, ClampedOpacity)));
		MessageTextBlock->SetText(Message);
	}
	if (LoadingTextBlock.IsValid())
	{
		LoadingTextBlock->SetColorAndOpacity(FSlateColor(FLinearColor(0.72f, 0.88f, 1.0f, ClampedOpacity)));
	}
}

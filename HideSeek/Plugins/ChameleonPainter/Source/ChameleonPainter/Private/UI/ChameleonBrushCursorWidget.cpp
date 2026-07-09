#include "UI/ChameleonBrushCursorWidget.h"

#include "Brushes/SlateRoundedBoxBrush.h"
#include "Rendering/DrawElements.h"

namespace
{
void AppendBrushCursorCirclePoints(TArray<FVector2D>& OutPoints, const FVector2D& Center, float Radius, int32 SegmentCount)
{
	OutPoints.Reset();
	for (int32 Index = 0; Index <= SegmentCount; ++Index)
	{
		const float Angle = (static_cast<float>(Index) / static_cast<float>(SegmentCount)) * UE_TWO_PI;
		OutPoints.Add(Center + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius);
	}
}

}

UChameleonBrushCursorWidget::UChameleonBrushCursorWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsFocusable(false);
}

void UChameleonBrushCursorWidget::SetPreviewDiameterPixels(float InDiameterPixels)
{
	PreviewDiameterPixels = FMath::Max(InDiameterPixels, 1.0f);
	InvalidateLayoutAndVolatility();
}

void UChameleonBrushCursorWidget::SetCursorMode(EChameleonBrushCursorMode InCursorMode)
{
	if (CursorMode == InCursorMode)
	{
		return;
	}

	CursorMode = InCursorMode;
	InvalidateLayoutAndVolatility();
}

void UChameleonBrushCursorWidget::SetSamplePreviewColor(FLinearColor InPreviewColor, bool bInHasPreviewColor)
{
	InPreviewColor.A = 1.0f;
	SamplePreviewColor = InPreviewColor;
	bHasSamplePreviewColor = bInHasPreviewColor;
	Invalidate(EInvalidateWidgetReason::Paint);
}

int32 UChameleonBrushCursorWidget::NativePaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled) const
{
	const int32 PaintedLayer = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
	const FVector2D LocalSize = AllottedGeometry.GetLocalSize();
	const float Diameter = FMath::Min(FMath::Min(LocalSize.X, LocalSize.Y), PreviewDiameterPixels);
	if (Diameter <= 2.0f)
	{
		return PaintedLayer;
	}

	if (CursorMode == EChameleonBrushCursorMode::Eyedropper)
	{
		const float Scale = FMath::Max(Diameter / 64.0f, 0.25f);
		const FVector2D Tip(12.0f * Scale, 50.0f * Scale);
		const FVector2D End(48.0f * Scale, 14.0f * Scale);
		const FVector2D Neck(36.0f * Scale, 26.0f * Scale);
		TArray<FVector2D> OuterPoints;
		OuterPoints.Add(Tip);
		OuterPoints.Add(Neck);
		OuterPoints.Add(End);
		FSlateDrawElement::MakeLines(
			OutDrawElements,
			PaintedLayer + 1,
			AllottedGeometry.ToPaintGeometry(),
			OuterPoints,
			ESlateDrawEffect::None,
			FLinearColor(0.0f, 0.0f, 0.0f, 0.95f),
			true,
			7.0f * Scale);

		TArray<FVector2D> InnerPoints;
		InnerPoints.Add(Tip);
		InnerPoints.Add(Neck);
		InnerPoints.Add(End);
		FSlateDrawElement::MakeLines(
			OutDrawElements,
			PaintedLayer + 2,
			AllottedGeometry.ToPaintGeometry(),
			InnerPoints,
			ESlateDrawEffect::None,
			FLinearColor(0.94f, 0.96f, 1.0f, 0.98f),
			true,
			3.0f * Scale);

		const FVector2D SampleCircleCenter(46.0f * Scale, 46.0f * Scale);
		const float SampleCircleRadius = 18.0f * Scale;
		const FLinearColor SampleColor = bHasSamplePreviewColor ? SamplePreviewColor : FLinearColor(0.12f, 0.12f, 0.12f, 0.92f);
		const FVector2D ShadowPosition = SampleCircleCenter - FVector2D(SampleCircleRadius + 4.0f * Scale, SampleCircleRadius + 4.0f * Scale);
		const FVector2D ShadowSize = FVector2D((SampleCircleRadius + 4.0f * Scale) * 2.0f);
		const FVector2D SamplePosition = SampleCircleCenter - FVector2D(SampleCircleRadius, SampleCircleRadius);
		const FVector2D SampleSize = FVector2D(SampleCircleRadius * 2.0f);
		const FSlateRoundedBoxBrush ShadowBrush(FLinearColor::White, SampleCircleRadius + 4.0f * Scale, ShadowSize);
		const FSlateRoundedBoxBrush SampleBrush(FLinearColor::White, SampleCircleRadius, SampleSize);
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			PaintedLayer + 3,
			AllottedGeometry.ToPaintGeometry(ShadowSize, FSlateLayoutTransform(ShadowPosition)),
			&ShadowBrush,
			ESlateDrawEffect::None,
			FLinearColor(0.0f, 0.0f, 0.0f, 0.64f));
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			PaintedLayer + 4,
			AllottedGeometry.ToPaintGeometry(SampleSize, FSlateLayoutTransform(SamplePosition)),
			&SampleBrush,
			ESlateDrawEffect::None,
			SampleColor);

		TArray<FVector2D> SampleCirclePoints;
		AppendBrushCursorCirclePoints(SampleCirclePoints, SampleCircleCenter, SampleCircleRadius, 72);
		FSlateDrawElement::MakeLines(
			OutDrawElements,
			PaintedLayer + 5,
			AllottedGeometry.ToPaintGeometry(),
			SampleCirclePoints,
			ESlateDrawEffect::None,
			FLinearColor(0.02f, 0.02f, 0.02f, 0.95f),
			true,
			2.0f * Scale);

		return PaintedLayer + 5;
	}

	const float MaxStrokeThickness = FMath::Max(OuterStrokeThickness, InnerStrokeThickness);
	const float Radius = FMath::Max((Diameter * 0.5f) - MaxStrokeThickness, 1.0f);
	const FVector2D Center = LocalSize * 0.5f;

	TArray<FVector2D> CirclePoints;
	AppendBrushCursorCirclePoints(CirclePoints, Center, Radius, 96);
	FSlateDrawElement::MakeLines(
		OutDrawElements,
		PaintedLayer + 1,
		AllottedGeometry.ToPaintGeometry(),
		CirclePoints,
		ESlateDrawEffect::None,
		OuterStrokeColor,
		true,
		OuterStrokeThickness);

	FSlateDrawElement::MakeLines(
		OutDrawElements,
		PaintedLayer + 2,
		AllottedGeometry.ToPaintGeometry(),
		CirclePoints,
		ESlateDrawEffect::None,
		InnerStrokeColor,
		true,
		InnerStrokeThickness);

	return PaintedLayer + 2;
}

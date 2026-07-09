#include "UI/ChameleonBrushCursorWidget.h"

#include "Rendering/DrawElements.h"

namespace
{
void AppendCirclePoints(TArray<FVector2D>& OutPoints, const FVector2D& Center, float Radius, int32 SegmentCount)
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

	const float MaxStrokeThickness = FMath::Max(OuterStrokeThickness, InnerStrokeThickness);
	const float Radius = FMath::Max((Diameter * 0.5f) - MaxStrokeThickness, 1.0f);
	const FVector2D Center = LocalSize * 0.5f;

	TArray<FVector2D> CirclePoints;
	AppendCirclePoints(CirclePoints, Center, Radius, 96);
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

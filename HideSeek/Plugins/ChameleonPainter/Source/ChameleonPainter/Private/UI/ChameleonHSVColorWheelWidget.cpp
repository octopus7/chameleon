#include "UI/ChameleonHSVColorWheelWidget.h"

#include "Input/Reply.h"
#include "InputCoreTypes.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SLeafWidget.h"

DECLARE_DELEGATE_OneParam(FOnChameleonHSVSlateColorChanged, FLinearColor);

namespace
{
constexpr float TwoPi = UE_TWO_PI;

FLinearColor ClampHSVWidgetColor(FLinearColor Color)
{
	Color.R = FMath::Clamp(Color.R, 0.0f, 1.0f);
	Color.G = FMath::Clamp(Color.G, 0.0f, 1.0f);
	Color.B = FMath::Clamp(Color.B, 0.0f, 1.0f);
	Color.A = 1.0f;
	return Color;
}

void ColorToHSV01(const FLinearColor& Color, float& OutHue, float& OutSaturation, float& OutValue)
{
	const FLinearColor HSV = ClampHSVWidgetColor(Color).LinearRGBToHSV();
	OutHue = FMath::Clamp(HSV.R / 360.0f, 0.0f, 1.0f);
	OutSaturation = FMath::Clamp(HSV.G, 0.0f, 1.0f);
	OutValue = FMath::Clamp(HSV.B, 0.0f, 1.0f);
}

FLinearColor ColorFromHSV01(float Hue, float Saturation, float Value)
{
	const FLinearColor HSV(FMath::Clamp(Hue, 0.0f, 1.0f) * 360.0f, FMath::Clamp(Saturation, 0.0f, 1.0f), FMath::Clamp(Value, 0.0f, 1.0f), 1.0f);
	FLinearColor RGB = HSV.HSVToLinearRGB();
	RGB.A = 1.0f;
	return ClampHSVWidgetColor(RGB);
}

void AppendCirclePoints(TArray<FVector2D>& OutPoints, const FVector2D& Center, float Radius, int32 SegmentCount)
{
	OutPoints.Reset();
	for (int32 Index = 0; Index <= SegmentCount; ++Index)
	{
		const float Angle = (static_cast<float>(Index) / static_cast<float>(SegmentCount)) * TwoPi;
		OutPoints.Add(Center + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius);
	}
}
}

class SChameleonHSVColorWheel final : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SChameleonHSVColorWheel)
		: _Color(FLinearColor::Red)
	{
	}
		SLATE_ARGUMENT(FLinearColor, Color)
		SLATE_EVENT(FOnChameleonHSVSlateColorChanged, OnColorChanged)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		CurrentColor = ClampHSVWidgetColor(InArgs._Color);
		OnColorChanged = InArgs._OnColorChanged;
	}

	void SetColor(FLinearColor NewColor)
	{
		CurrentColor = ClampHSVWidgetColor(NewColor);
	}

	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override
	{
		return FVector2D(226.0f, 226.0f);
	}

	virtual int32 OnPaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override
	{
		const FVector2D LocalSize = AllottedGeometry.GetLocalSize();
		const float Diameter = FMath::Min(LocalSize.X, LocalSize.Y);
		const FVector2D Center = LocalSize * 0.5f;
		const float OuterRadius = Diameter * 0.5f - 5.0f;
		const float RingThickness = FMath::Max(20.0f, Diameter * 0.105f);
		const float RingMidRadius = OuterRadius - RingThickness * 0.5f;
		const float DiskRadius = OuterRadius - RingThickness - 9.0f;
		const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));

		float Hue = 0.0f;
		float Saturation = 0.0f;
		float Value = 0.0f;
		ColorToHSV01(CurrentColor, Hue, Saturation, Value);

		constexpr int32 RingSegments = 96;
		for (int32 Index = 0; Index < RingSegments; ++Index)
		{
			const float StartHue = static_cast<float>(Index) / static_cast<float>(RingSegments);
			const float EndHue = static_cast<float>(Index + 1) / static_cast<float>(RingSegments);
			const float StartAngle = -StartHue * TwoPi;
			const float EndAngle = -EndHue * TwoPi;
			TArray<FVector2D> SegmentPoints;
			SegmentPoints.Add(Center + FVector2D(FMath::Cos(StartAngle), FMath::Sin(StartAngle)) * RingMidRadius);
			SegmentPoints.Add(Center + FVector2D(FMath::Cos(EndAngle), FMath::Sin(EndAngle)) * RingMidRadius);

			FSlateDrawElement::MakeLines(
				OutDrawElements,
				LayerId,
				AllottedGeometry.ToPaintGeometry(),
				SegmentPoints,
				ESlateDrawEffect::None,
				ColorFromHSV01(StartHue, 1.0f, 1.0f),
				true,
				RingThickness);
		}

		constexpr int32 DiskCells = 34;
		const float CellSize = (DiskRadius * 2.0f) / static_cast<float>(DiskCells);
		const FVector2D DiskTopLeft = Center - FVector2D(DiskRadius, DiskRadius);
		for (int32 Y = 0; Y < DiskCells; ++Y)
		{
			for (int32 X = 0; X < DiskCells; ++X)
			{
				const FVector2D CellCenter = DiskTopLeft + FVector2D((static_cast<float>(X) + 0.5f) * CellSize, (static_cast<float>(Y) + 0.5f) * CellSize);
				if (FVector2D::Distance(CellCenter, Center) > DiskRadius)
				{
					continue;
				}

				const float CellSaturation = FMath::Clamp((CellCenter.X - (Center.X - DiskRadius)) / (DiskRadius * 2.0f), 0.0f, 1.0f);
				const float CellValue = FMath::Clamp(1.0f - ((CellCenter.Y - (Center.Y - DiskRadius)) / (DiskRadius * 2.0f)), 0.0f, 1.0f);
				FSlateDrawElement::MakeBox(
					OutDrawElements,
					LayerId + 1,
					AllottedGeometry.ToPaintGeometry(
						FVector2D(CellSize + 0.5f, CellSize + 0.5f),
						FSlateLayoutTransform(DiskTopLeft + FVector2D(static_cast<float>(X) * CellSize, static_cast<float>(Y) * CellSize))),
					WhiteBrush,
					ESlateDrawEffect::None,
					ColorFromHSV01(Hue, CellSaturation, CellValue));
			}
		}

		TArray<FVector2D> CirclePoints;
		AppendCirclePoints(CirclePoints, Center, OuterRadius, 96);
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 2, AllottedGeometry.ToPaintGeometry(), CirclePoints, ESlateDrawEffect::None, FLinearColor(0.02f, 0.02f, 0.02f, 0.9f), true, 2.0f);
		AppendCirclePoints(CirclePoints, Center, DiskRadius, 72);
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 2, AllottedGeometry.ToPaintGeometry(), CirclePoints, ESlateDrawEffect::None, FLinearColor(0.02f, 0.02f, 0.02f, 1.0f), true, 3.0f);

		const float HueAngle = -Hue * TwoPi;
		const FVector2D HueHandle = Center + FVector2D(FMath::Cos(HueAngle), FMath::Sin(HueAngle)) * RingMidRadius;
		DrawHandle(OutDrawElements, AllottedGeometry, LayerId + 3, HueHandle, 11.0f);

		const FVector2D SVHandle(
			Center.X - DiskRadius + Saturation * DiskRadius * 2.0f,
			Center.Y - DiskRadius + (1.0f - Value) * DiskRadius * 2.0f);
		DrawHandle(OutDrawElements, AllottedGeometry, LayerId + 3, SVHandle, 9.0f);

		return LayerId + 4;
	}

	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
		{
			return FReply::Unhandled();
		}

		if (ApplyPointer(MyGeometry, MouseEvent.GetScreenSpacePosition()))
		{
			return FReply::Handled().CaptureMouse(SharedThis(this));
		}

		return FReply::Unhandled();
	}

	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		if (HasMouseCapture())
		{
			ApplyPointer(MyGeometry, MouseEvent.GetScreenSpacePosition());
			return FReply::Handled();
		}

		return FReply::Unhandled();
	}

	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && HasMouseCapture())
		{
			ActiveDragMode = EDragMode::None;
			return FReply::Handled().ReleaseMouseCapture();
		}

		return FReply::Unhandled();
	}

private:
	enum class EDragMode : uint8
	{
		None,
		Hue,
		SaturationValue
	};

	static void DrawHandle(FSlateWindowElementList& OutDrawElements, const FGeometry& Geometry, int32 LayerId, const FVector2D& Center, float Radius)
	{
		TArray<FVector2D> Points;
		AppendCirclePoints(Points, Center, Radius + 2.0f, 32);
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId, Geometry.ToPaintGeometry(), Points, ESlateDrawEffect::None, FLinearColor::Black, true, 3.0f);
		AppendCirclePoints(Points, Center, Radius, 32);
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, Geometry.ToPaintGeometry(), Points, ESlateDrawEffect::None, FLinearColor::White, true, 3.0f);
	}

	bool ApplyPointer(const FGeometry& Geometry, const FVector2D& ScreenPosition)
	{
		const FVector2D LocalSize = Geometry.GetLocalSize();
		const float Diameter = FMath::Min(LocalSize.X, LocalSize.Y);
		const FVector2D Center = LocalSize * 0.5f;
		const float OuterRadius = Diameter * 0.5f - 5.0f;
		const float RingThickness = FMath::Max(20.0f, Diameter * 0.105f);
		const float InnerRingRadius = OuterRadius - RingThickness;
		const float DiskRadius = InnerRingRadius - 9.0f;
		const FVector2D LocalPosition = Geometry.AbsoluteToLocal(ScreenPosition);
		const FVector2D Delta = LocalPosition - Center;
		const float Distance = Delta.Size();

		if (ActiveDragMode == EDragMode::None)
		{
			if (Distance <= OuterRadius && Distance >= InnerRingRadius)
			{
				ActiveDragMode = EDragMode::Hue;
			}
			else if (Distance <= DiskRadius)
			{
				ActiveDragMode = EDragMode::SaturationValue;
			}
			else
			{
				return false;
			}
		}

		float Hue = 0.0f;
		float Saturation = 0.0f;
		float Value = 0.0f;
		ColorToHSV01(CurrentColor, Hue, Saturation, Value);

		if (ActiveDragMode == EDragMode::Hue)
		{
			float Angle = FMath::Atan2(-Delta.Y, Delta.X);
			if (Angle < 0.0f)
			{
				Angle += TwoPi;
			}
			Hue = FMath::Clamp(Angle / TwoPi, 0.0f, 1.0f);
		}
		else if (ActiveDragMode == EDragMode::SaturationValue)
		{
			Saturation = FMath::Clamp((Delta.X + DiskRadius) / (DiskRadius * 2.0f), 0.0f, 1.0f);
			Value = FMath::Clamp(1.0f - ((Delta.Y + DiskRadius) / (DiskRadius * 2.0f)), 0.0f, 1.0f);
		}

		CurrentColor = ColorFromHSV01(Hue, Saturation, Value);
		OnColorChanged.ExecuteIfBound(CurrentColor);
		return true;
	}

	FLinearColor CurrentColor = FLinearColor::Red;
	FOnChameleonHSVSlateColorChanged OnColorChanged;
	EDragMode ActiveDragMode = EDragMode::None;
};

UChameleonHSVColorWheelWidget::UChameleonHSVColorWheelWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UChameleonHSVColorWheelWidget::SetSelectedColor(FLinearColor NewColor)
{
	SelectedColor = ClampHSVWidgetColor(NewColor);
	if (SlateColorWheel)
	{
		SlateColorWheel->SetColor(SelectedColor);
	}
}

TSharedRef<SWidget> UChameleonHSVColorWheelWidget::RebuildWidget()
{
	SAssignNew(SlateColorWheel, SChameleonHSVColorWheel)
		.Color(SelectedColor)
		.OnColorChanged(FOnChameleonHSVSlateColorChanged::CreateUObject(this, &UChameleonHSVColorWheelWidget::HandleSlateColorChanged));

	return SlateColorWheel.ToSharedRef();
}

void UChameleonHSVColorWheelWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	SlateColorWheel.Reset();
}

void UChameleonHSVColorWheelWidget::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	if (SlateColorWheel)
	{
		SlateColorWheel->SetColor(SelectedColor);
	}
}

void UChameleonHSVColorWheelWidget::HandleSlateColorChanged(FLinearColor NewColor)
{
	SelectedColor = ClampHSVWidgetColor(NewColor);
	OnColorChanged.Broadcast(SelectedColor);
}

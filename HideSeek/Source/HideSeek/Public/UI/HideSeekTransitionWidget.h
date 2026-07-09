#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HideSeekTransitionWidget.generated.h"

class SBorder;
class STextBlock;
class SWidget;

UCLASS()
class HIDESEEK_API UHideSeekTransitionWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "HideSeek|Transition")
	void StartTransition(FText InMessage, float InFadeInSeconds, float InHoldSeconds, float InFadeOutSeconds);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void ApplyOpacity(float Opacity);

	TSharedPtr<SBorder> FadeBorder;
	TSharedPtr<STextBlock> MessageTextBlock;
	TSharedPtr<STextBlock> LoadingTextBlock;

	FText Message;
	float FadeInSeconds = 0.0f;
	float HoldSeconds = 0.0f;
	float FadeOutSeconds = 0.0f;
	float ElapsedSeconds = 0.0f;
	bool bPlaying = false;
};

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "ChameleonHiderCharacter.generated.h"

class UCameraComponent;
class UChameleonColorPickerWidget;
class UChameleonMetaballBodyComponent;
class UChameleonPainterInputConfig;
class UChameleonPaintComponent;
class USpringArmComponent;
class UUserWidget;

UCLASS(BlueprintType, Blueprintable)
class CHAMELEONPAINTER_API AChameleonHiderCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AChameleonHiderCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_Controller() override;

	UFUNCTION(BlueprintCallable, Category = "Chameleon|Input")
	void RefreshInputConfigFromGameInstance();

	UFUNCTION(BlueprintPure, Category = "Chameleon|Input")
	UChameleonPainterInputConfig* GetInputConfig() const { return InputConfig; }

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Chameleon")
	TObjectPtr<UChameleonMetaballBodyComponent> BodyComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Chameleon")
	TObjectPtr<UChameleonPaintComponent> PaintComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chameleon|UI")
	TSubclassOf<UChameleonColorPickerWidget> ColorPickerWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chameleon|UI")
	TSubclassOf<UUserWidget> BrushCursorWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chameleon|Paint", meta = (ClampMin = "1.0"))
	float BrushRadiusCm = 18.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chameleon|Paint", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float BrushStrength = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chameleon|Paint", meta = (ClampMin = "100.0"))
	float BrushTraceDistance = 1200.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chameleon|Paint", meta = (ClampMin = "100.0"))
	float SampleTraceDistance = 2000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chameleon|Paint")
	FLinearColor CurrentBrushColor = FLinearColor(1.0f, 0.02f, 0.02f, 1.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chameleon|Paint")
	TArray<FName> SampleColorParameterNames;

protected:
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void StartPaint();
	void StopPaint();
	void PaintTriggered();
	void SampleColor();
	void ToggleColorPicker();

	UFUNCTION()
	void HandlePickerColorChanged(FLinearColor Color);

private:
	void AddMappingContext();
	void BindEnhancedInput(UInputComponent* PlayerInputComponent);
	bool TraceFromView(float Distance, FHitResult& OutHit, bool bTraceSelfBody) const;
	bool TrySampleColorFromHit(const FHitResult& Hit, FLinearColor& OutColor) const;
	void EnsureColorPicker();
	void EnsureBrushCursor();
	void UpdateBrushCursorPosition();
	void SetColorPickerVisible(bool bVisible);

	UPROPERTY(Transient)
	TObjectPtr<UChameleonPainterInputConfig> InputConfig;

	UPROPERTY(Transient)
	TObjectPtr<UChameleonColorPickerWidget> ColorPickerWidget;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> BrushCursorWidget;

	bool bPaintHeld = false;
	bool bColorPickerVisible = false;
};

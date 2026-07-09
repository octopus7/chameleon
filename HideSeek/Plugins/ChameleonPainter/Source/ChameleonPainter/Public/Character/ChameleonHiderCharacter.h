#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "ChameleonHiderCharacter.generated.h"

class AChameleonPaintSprayActor;
class UCameraComponent;
class UChameleonBrushCursorWidget;
class UChameleonColorPickerWidget;
class UChameleonMetaballBodyComponent;
class UChameleonPainterInputConfig;
class UChameleonPaintComponent;
class UMaterialInterface;
class USpringArmComponent;

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
	TSubclassOf<UChameleonBrushCursorWidget> BrushCursorWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chameleon|Paint", meta = (ClampMin = "1.0"))
	float BrushRadiusCm = 18.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chameleon|Paint", meta = (ClampMin = "1.0"))
	float MinBrushRadiusCm = 4.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chameleon|Paint", meta = (ClampMin = "1.0"))
	float MaxBrushRadiusCm = 72.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chameleon|Paint", meta = (ClampMin = "0.1"))
	float BrushRadiusStepCm = 3.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chameleon|UI", meta = (ClampMin = "0.1"))
	float BrushCursorFallbackPixelsPerCm = 2.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chameleon|UI", meta = (ClampMin = "4.0"))
	float MinBrushCursorDiameterPixels = 24.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chameleon|UI", meta = (ClampMin = "4.0"))
	float MaxBrushCursorDiameterPixels = 260.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chameleon|Paint", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float BrushStrength = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chameleon|Paint", meta = (ClampMin = "100.0"))
	float BrushTraceDistance = 1200.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chameleon|Paint", meta = (ClampMin = "100.0"))
	float SampleTraceDistance = 2000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chameleon|Paint")
	FLinearColor CurrentBrushColor = FLinearColor(1.0f, 0.02f, 0.02f, 1.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chameleon|Paint", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float CurrentBrushRoughness = 0.84f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chameleon|Paint", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float CurrentBrushMetallic = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chameleon|Paint")
	TArray<FName> SampleColorParameterNames;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chameleon|Paint|Effects")
	TSubclassOf<AChameleonPaintSprayActor> PaintSprayEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chameleon|Paint|Effects")
	TObjectPtr<UMaterialInterface> PaintSprayMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chameleon|Paint|Effects", meta = (ClampMin = "0.05", ClampMax = "2.0"))
	float PaintSprayLifetimeSeconds = 0.35f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chameleon|Paint|Effects", meta = (ClampMin = "1", ClampMax = "128"))
	int32 PaintSprayParticleCount = 42;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chameleon|Paint|Effects", meta = (ClampMin = "0.1", ClampMax = "2.0"))
	float PaintSprayRadiusScale = 0.72f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chameleon|Paint|Effects", meta = (ClampMin = "0.0", ClampMax = "0.25"))
	float PaintSpraySpawnIntervalSeconds = 0.035f;

protected:
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void StartPaint();
	void StopPaint();
	void PaintTriggered();
	void SampleColor();
	void ToggleColorPicker();
	void DecreaseBrushSize();
	void IncreaseBrushSize();

	UFUNCTION()
	void HandlePickerColorChanged(FLinearColor Color);

	UFUNCTION()
	void HandlePickerMaterialPropertiesChanged(float Roughness, float Metallic);

private:
	void AddMappingContext();
	void BindEnhancedInput(UInputComponent* PlayerInputComponent);
	bool TraceFromView(float Distance, FHitResult& OutHit, bool bTraceSelfBody) const;
	bool TrySampleColorFromHit(const FHitResult& Hit, FLinearColor& OutColor) const;
	void SpawnPaintSprayEffect(const FHitResult& Hit);
	void EnsureColorPicker();
	void EnsureBrushCursor();
	void UpdateBrushCursorPosition();
	void AdjustBrushSize(float DeltaRadiusCm);
	float CalculateBrushCursorDiameterPixels(const APlayerController& PlayerController) const;
	void SetColorPickerVisible(bool bVisible);

	UPROPERTY(Transient)
	TObjectPtr<UChameleonPainterInputConfig> InputConfig;

	UPROPERTY(Transient)
	TObjectPtr<UChameleonColorPickerWidget> ColorPickerWidget;

	UPROPERTY(Transient)
	TObjectPtr<UChameleonBrushCursorWidget> BrushCursorWidget;

	bool bPaintHeld = false;
	bool bColorPickerVisible = false;
	double LastPaintSpraySpawnTimeSeconds = -1000.0;
};

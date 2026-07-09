#include "Character/ChameleonHiderCharacter.h"

#include "Actors/ChameleonPaintSprayActor.h"
#include "Blueprint/UserWidget.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/ChameleonMetaballBodyComponent.h"
#include "Components/ChameleonPaintComponent.h"
#include "Data/ChameleonPainterInputConfig.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Game/ChameleonPainterGameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputCoreTypes.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialParameters.h"
#include "UI/ChameleonBrushCursorWidget.h"
#include "UI/ChameleonColorPickerWidget.h"
#include "UnrealClient.h"

AChameleonHiderCharacter::AChameleonHiderCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
	GetCharacterMovement()->JumpZVelocity = 520.0f;
	GetCharacterMovement()->AirControl = 0.35f;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetRootComponent());
	CameraBoom->TargetArmLength = 420.0f;
	CameraBoom->SocketOffset = FVector(0.0f, 70.0f, 70.0f);
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom);
	FollowCamera->bUsePawnControlRotation = false;

	BodyComponent = CreateDefaultSubobject<UChameleonMetaballBodyComponent>(TEXT("ChameleonBody"));
	BodyComponent->SetupAttachment(GetRootComponent());
	BodyComponent->SetRelativeLocation(FVector(-8.0f, 0.0f, -88.0f));
	BodyComponent->bBuildQueryCollision = true;

	PaintComponent = CreateDefaultSubobject<UChameleonPaintComponent>(TEXT("ChameleonPaint"));
	PaintComponent->TargetComponent = BodyComponent;
	PaintComponent->bApplyOnRegister = false;
	PaintComponent->bApplyOnBeginPlay = false;
	PaintComponent->bApplyOnTargetComponentChange = false;

	SampleColorParameterNames = {
		TEXT("ChameleonSampleColor"),
		TEXT("PaintColor"),
		TEXT("BodyColor"),
		TEXT("BaseColor"),
		TEXT("Color")
	};
}

void AChameleonHiderCharacter::BeginPlay()
{
	Super::BeginPlay();
	RefreshInputConfigFromGameInstance();
	AddMappingContext();
	EnsureColorPicker();
	SetColorPickerVisible(false);
}

void AChameleonHiderCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bColorPickerVisible)
	{
		if (bEyedropperModeActive)
		{
			UpdateEyedropperPreviewColor();
		}
		UpdateBrushCursorPosition();
	}
}

void AChameleonHiderCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	RefreshInputConfigFromGameInstance();
	BindEnhancedInput(PlayerInputComponent);
}

void AChameleonHiderCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	RefreshInputConfigFromGameInstance();
	AddMappingContext();
}

void AChameleonHiderCharacter::OnRep_Controller()
{
	Super::OnRep_Controller();
	RefreshInputConfigFromGameInstance();
	AddMappingContext();
}

void AChameleonHiderCharacter::RefreshInputConfigFromGameInstance()
{
	const UChameleonPainterGameInstance* ChameleonGameInstance = GetGameInstance<UChameleonPainterGameInstance>();
	InputConfig = ChameleonGameInstance ? ChameleonGameInstance->ChameleonInputConfig : nullptr;
}

void AChameleonHiderCharacter::Move(const FInputActionValue& Value)
{
	if (!Controller)
	{
		return;
	}

	const FVector2D Axis = Value.Get<FVector2D>();
	const FRotator YawRotation(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
	AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X), Axis.Y);
	AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y), Axis.X);
}

void AChameleonHiderCharacter::Look(const FInputActionValue& Value)
{
	if (bColorPickerVisible)
	{
		return;
	}

	const FVector2D Axis = Value.Get<FVector2D>();
	AddControllerYawInput(Axis.X);
	AddControllerPitchInput(-Axis.Y);
}

void AChameleonHiderCharacter::StartPaint()
{
	if (bColorPickerVisible && bEyedropperModeActive)
	{
		PickEyedropperColor();
		return;
	}

	bPaintHeld = true;
	PaintTriggered();
}

void AChameleonHiderCharacter::StopPaint()
{
	bPaintHeld = false;
}

void AChameleonHiderCharacter::PaintTriggered()
{
	if ((bColorPickerVisible && bEyedropperModeActive) || !bPaintHeld || !BodyComponent)
	{
		return;
	}

	FHitResult Hit;
	if (TraceFromView(BrushTraceDistance, Hit, true))
	{
		const bool bAppliedPaint = BodyComponent->ApplyPaintStrokeFromHit(Hit, BrushRadiusCm, CurrentBrushColor, BrushStrength, CurrentBrushRoughness, CurrentBrushMetallic);
		if (bAppliedPaint)
		{
			SpawnPaintSprayEffect(Hit);
		}
	}
}

void AChameleonHiderCharacter::SampleColor()
{
	FLinearColor SampledColor;
	if (TrySampleEyedropperColor(SampledColor))
	{
		CurrentBrushColor = SampledColor;
		if (ColorPickerWidget)
		{
			ColorPickerWidget->SetSelectedColor(CurrentBrushColor, true);
		}
	}
}

void AChameleonHiderCharacter::ToggleColorPicker()
{
	EnsureColorPicker();
	SetColorPickerVisible(!bColorPickerVisible);
}

void AChameleonHiderCharacter::DecreaseBrushSize()
{
	AdjustBrushSize(-BrushRadiusStepCm);
}

void AChameleonHiderCharacter::IncreaseBrushSize()
{
	AdjustBrushSize(BrushRadiusStepCm);
}

void AChameleonHiderCharacter::HandlePickerColorChanged(FLinearColor Color)
{
	Color.A = 1.0f;
	CurrentBrushColor = Color;
}

void AChameleonHiderCharacter::HandlePickerMaterialPropertiesChanged(float Roughness, float Metallic)
{
	CurrentBrushRoughness = FMath::Clamp(Roughness, 0.0f, 1.0f);
	CurrentBrushMetallic = FMath::Clamp(Metallic, 0.0f, 1.0f);
}

void AChameleonHiderCharacter::HandlePickerEyedropperModeChanged(bool bActive)
{
	SetEyedropperModeActive(bActive);
}

void AChameleonHiderCharacter::AddMappingContext()
{
	if (!InputConfig || !InputConfig->DefaultMappingContext)
	{
		return;
	}

	const APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController)
	{
		return;
	}

	ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer();
	if (!LocalPlayer)
	{
		return;
	}

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
	{
		Subsystem->AddMappingContext(InputConfig->DefaultMappingContext, 0);
	}
}

void AChameleonHiderCharacter::BindEnhancedInput(UInputComponent* PlayerInputComponent)
{
	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EnhancedInputComponent || !InputConfig)
	{
		return;
	}

	if (InputConfig->MoveAction)
	{
		EnhancedInputComponent->BindAction(InputConfig->MoveAction, ETriggerEvent::Triggered, this, &AChameleonHiderCharacter::Move);
	}
	if (InputConfig->LookAction)
	{
		EnhancedInputComponent->BindAction(InputConfig->LookAction, ETriggerEvent::Triggered, this, &AChameleonHiderCharacter::Look);
	}
	if (InputConfig->JumpAction)
	{
		EnhancedInputComponent->BindAction(InputConfig->JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(InputConfig->JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
	}
	if (InputConfig->PaintAction)
	{
		EnhancedInputComponent->BindAction(InputConfig->PaintAction, ETriggerEvent::Started, this, &AChameleonHiderCharacter::StartPaint);
		EnhancedInputComponent->BindAction(InputConfig->PaintAction, ETriggerEvent::Triggered, this, &AChameleonHiderCharacter::PaintTriggered);
		EnhancedInputComponent->BindAction(InputConfig->PaintAction, ETriggerEvent::Completed, this, &AChameleonHiderCharacter::StopPaint);
	}
	if (InputConfig->SampleColorAction)
	{
		EnhancedInputComponent->BindAction(InputConfig->SampleColorAction, ETriggerEvent::Started, this, &AChameleonHiderCharacter::SampleColor);
	}
	if (InputConfig->ToggleColorPickerAction)
	{
		EnhancedInputComponent->BindAction(InputConfig->ToggleColorPickerAction, ETriggerEvent::Started, this, &AChameleonHiderCharacter::ToggleColorPicker);
	}
	if (InputConfig->DecreaseBrushSizeAction)
	{
		EnhancedInputComponent->BindAction(InputConfig->DecreaseBrushSizeAction, ETriggerEvent::Started, this, &AChameleonHiderCharacter::DecreaseBrushSize);
	}
	if (InputConfig->IncreaseBrushSizeAction)
	{
		EnhancedInputComponent->BindAction(InputConfig->IncreaseBrushSizeAction, ETriggerEvent::Started, this, &AChameleonHiderCharacter::IncreaseBrushSize);
	}
}

bool AChameleonHiderCharacter::TraceFromView(float Distance, FHitResult& OutHit, bool bTraceSelfBody) const
{
	const APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController)
	{
		return false;
	}

	FVector ViewLocation;
	FVector ViewDirection;
	if (bColorPickerVisible)
	{
		if (!PlayerController->DeprojectMousePositionToWorld(ViewLocation, ViewDirection))
		{
			return false;
		}
	}
	else
	{
		FRotator ViewRotation;
		PlayerController->GetPlayerViewPoint(ViewLocation, ViewRotation);
		ViewDirection = ViewRotation.Vector();
	}

	const FVector End = ViewLocation + ViewDirection * Distance;

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ChameleonPainterTrace), true);
	QueryParams.bReturnFaceIndex = true;

	if (bTraceSelfBody && BodyComponent)
	{
		return BodyComponent->LineTraceComponent(OutHit, ViewLocation, End, QueryParams);
	}

	QueryParams.AddIgnoredActor(this);
	return GetWorld() && GetWorld()->LineTraceSingleByChannel(OutHit, ViewLocation, End, ECC_Visibility, QueryParams);
}

bool AChameleonHiderCharacter::TrySampleColorFromHit(const FHitResult& Hit, FLinearColor& OutColor) const
{
	const UPrimitiveComponent* HitComponent = Hit.GetComponent();
	if (!HitComponent)
	{
		return false;
	}

	if (const UChameleonMetaballBodyComponent* MetaballBody = Cast<UChameleonMetaballBodyComponent>(HitComponent))
	{
		if (MetaballBody->TrySampleBaseColorFromHit(Hit, OutColor))
		{
			OutColor.A = 1.0f;
			return true;
		}
	}

	int32 MaterialIndex = 0;
	UMaterialInterface* Material = HitComponent->GetMaterialFromCollisionFaceIndex(Hit.FaceIndex, MaterialIndex);
	if (!Material)
	{
		Material = HitComponent->GetMaterial(MaterialIndex);
	}

	if (!Material)
	{
		return false;
	}

	for (const FName& ParameterName : SampleColorParameterNames)
	{
		FLinearColor ParameterColor;
		if (!ParameterName.IsNone() && Material->GetVectorParameterValue(FHashedMaterialParameterInfo(ParameterName), ParameterColor))
		{
			ParameterColor.A = 1.0f;
			OutColor = ParameterColor;
			return true;
		}
	}

	return false;
}

bool AChameleonHiderCharacter::TrySampleSurfaceDataColor(FLinearColor& OutColor) const
{
	FHitResult Hit;
	return TraceFromView(SampleTraceDistance, Hit, false) && TrySampleColorFromHit(Hit, OutColor);
}

bool AChameleonHiderCharacter::TrySampleFinalScreenColor(FLinearColor& OutColor) const
{
	const APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController)
	{
		return false;
	}

	float MouseX = 0.0f;
	float MouseY = 0.0f;
	if (!PlayerController->GetMousePosition(MouseX, MouseY))
	{
		return false;
	}

	ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer();
	UGameViewportClient* ViewportClient = LocalPlayer ? LocalPlayer->ViewportClient : nullptr;
	FViewport* Viewport = ViewportClient ? ViewportClient->Viewport : nullptr;
	if (!Viewport)
	{
		return false;
	}

	const FIntPoint ViewportSize = Viewport->GetSizeXY();
	if (ViewportSize.X <= 0 || ViewportSize.Y <= 0)
	{
		return false;
	}

	const int32 PixelX = FMath::Clamp(FMath::RoundToInt(MouseX), 0, ViewportSize.X - 1);
	const int32 PixelY = FMath::Clamp(FMath::RoundToInt(MouseY), 0, ViewportSize.Y - 1);
	TArray<FColor> SampledPixels;
	FReadSurfaceDataFlags ReadFlags(RCM_UNorm);
	ReadFlags.SetLinearToGamma(false);
	if (!Viewport->ReadPixels(SampledPixels, ReadFlags, FIntRect(PixelX, PixelY, PixelX + 1, PixelY + 1)) || SampledPixels.IsEmpty())
	{
		return false;
	}

	OutColor = FLinearColor::FromSRGBColor(SampledPixels[0]);
	OutColor.A = 1.0f;
	return true;
}

bool AChameleonHiderCharacter::TrySampleEyedropperColor(FLinearColor& OutColor) const
{
	switch (ColorPickSource)
	{
	case EChameleonColorPickSource::FinalScreen:
		if (TrySampleFinalScreenColor(OutColor))
		{
			return true;
		}
		return TrySampleSurfaceDataColor(OutColor);
	case EChameleonColorPickSource::SurfaceData:
	default:
		return TrySampleSurfaceDataColor(OutColor);
	}
}

void AChameleonHiderCharacter::UpdateEyedropperPreviewColor()
{
	FLinearColor SampledColor;
	bHasEyedropperPreviewColor = TrySampleEyedropperColor(SampledColor);
	if (bHasEyedropperPreviewColor)
	{
		EyedropperPreviewColor = SampledColor;
	}

	if (BrushCursorWidget)
	{
		BrushCursorWidget->SetSamplePreviewColor(EyedropperPreviewColor, bHasEyedropperPreviewColor);
	}
}

void AChameleonHiderCharacter::PickEyedropperColor()
{
	FLinearColor SampledColor;
	if (!TrySampleEyedropperColor(SampledColor))
	{
		return;
	}

	SampledColor.A = 1.0f;
	CurrentBrushColor = SampledColor;
	EyedropperPreviewColor = SampledColor;
	bHasEyedropperPreviewColor = true;
	if (ColorPickerWidget)
	{
		ColorPickerWidget->SetSelectedColor(CurrentBrushColor, true);
	}
	if (BrushCursorWidget)
	{
		BrushCursorWidget->SetSamplePreviewColor(CurrentBrushColor, true);
	}
}

void AChameleonHiderCharacter::SpawnPaintSprayEffect(const FHitResult& Hit)
{
	UWorld* World = GetWorld();
	if (!World || PaintSprayParticleCount <= 0 || PaintSprayLifetimeSeconds <= 0.0f)
	{
		return;
	}

	const double CurrentTimeSeconds = World->GetTimeSeconds();
	if (PaintSpraySpawnIntervalSeconds > 0.0f && CurrentTimeSeconds - LastPaintSpraySpawnTimeSeconds < static_cast<double>(PaintSpraySpawnIntervalSeconds))
	{
		return;
	}
	LastPaintSpraySpawnTimeSeconds = CurrentTimeSeconds;

	const FVector SurfaceNormal = Hit.ImpactNormal.GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
	const FVector SpawnLocation = Hit.ImpactPoint + SurfaceNormal * 1.5f;
	TSubclassOf<AChameleonPaintSprayActor> SprayClass = PaintSprayEffectClass;
	if (!SprayClass)
	{
		SprayClass = AChameleonPaintSprayActor::StaticClass();
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.Instigator = this;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AChameleonPaintSprayActor* SprayActor = World->SpawnActor<AChameleonPaintSprayActor>(SprayClass, SpawnLocation, FRotator::ZeroRotator, SpawnParameters);
	if (SprayActor)
	{
		SprayActor->InitializeSpray(
			SurfaceNormal,
			CurrentBrushColor,
			BrushRadiusCm * PaintSprayRadiusScale,
			PaintSprayMaterial,
			PaintSprayLifetimeSeconds,
			PaintSprayParticleCount);
	}
}

void AChameleonHiderCharacter::EnsureColorPicker()
{
	if (ColorPickerWidget || !IsLocallyControlled())
	{
		return;
	}

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController)
	{
		return;
	}

	TSubclassOf<UChameleonColorPickerWidget> WidgetClass = ColorPickerWidgetClass;
	if (!WidgetClass)
	{
		WidgetClass = UChameleonColorPickerWidget::StaticClass();
	}
	ColorPickerWidget = CreateWidget<UChameleonColorPickerWidget>(PlayerController, WidgetClass);
	if (ColorPickerWidget)
	{
		ColorPickerWidget->SetTargetPaintComponent(nullptr);
		ColorPickerWidget->bApplyChangesImmediately = false;
		ColorPickerWidget->SetSelectedColor(CurrentBrushColor, false);
		ColorPickerWidget->SetSelectedMaterialProperties(CurrentBrushRoughness, CurrentBrushMetallic, false);
		ColorPickerWidget->OnColorChanged.AddDynamic(this, &AChameleonHiderCharacter::HandlePickerColorChanged);
		ColorPickerWidget->OnMaterialPropertiesChanged.AddDynamic(this, &AChameleonHiderCharacter::HandlePickerMaterialPropertiesChanged);
		ColorPickerWidget->OnEyedropperModeChanged.AddDynamic(this, &AChameleonHiderCharacter::HandlePickerEyedropperModeChanged);
		ColorPickerWidget->AddToViewport(100);
		ColorPickerWidget->SetAlignmentInViewport(FVector2D::ZeroVector);
		ColorPickerWidget->SetPositionInViewport(FVector2D(24.0f, 24.0f), false);
		ColorPickerWidget->SetDesiredSizeInViewport(FVector2D(520.0f, 700.0f));
	}
}

void AChameleonHiderCharacter::EnsureBrushCursor()
{
	if (BrushCursorWidget || !IsLocallyControlled())
	{
		return;
	}

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController)
	{
		return;
	}

	TSubclassOf<UChameleonBrushCursorWidget> WidgetClass = BrushCursorWidgetClass;
	if (!WidgetClass)
	{
		WidgetClass = UChameleonBrushCursorWidget::StaticClass();
	}

	BrushCursorWidget = CreateWidget<UChameleonBrushCursorWidget>(PlayerController, WidgetClass);
	if (BrushCursorWidget)
	{
		BrushCursorWidget->AddToViewport(1000);
		BrushCursorWidget->SetAlignmentInViewport(FVector2D::ZeroVector);
		const float CursorDiameter = FMath::Clamp(
			BrushRadiusCm * BrushCursorFallbackPixelsPerCm * 2.0f,
			MinBrushCursorDiameterPixels,
			MaxBrushCursorDiameterPixels);
		BrushCursorWidget->SetPreviewDiameterPixels(CursorDiameter);
		BrushCursorWidget->SetCursorMode(EChameleonBrushCursorMode::Brush);
		BrushCursorWidget->SetSamplePreviewColor(CurrentBrushColor, false);
		BrushCursorWidget->SetDesiredSizeInViewport(FVector2D(CursorDiameter, CursorDiameter));
		BrushCursorWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void AChameleonHiderCharacter::UpdateBrushCursorPosition()
{
	if (!BrushCursorWidget)
	{
		return;
	}

	const APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController)
	{
		return;
	}

	float MouseX = 0.0f;
	float MouseY = 0.0f;
	if (PlayerController->GetMousePosition(MouseX, MouseY))
	{
		const float CursorDiameter = bEyedropperModeActive
			? FMath::Max(EyedropperCursorSizePixels, 24.0f)
			: CalculateBrushCursorDiameterPixels(*PlayerController);
		BrushCursorWidget->SetCursorMode(bEyedropperModeActive ? EChameleonBrushCursorMode::Eyedropper : EChameleonBrushCursorMode::Brush);
		BrushCursorWidget->SetPreviewDiameterPixels(CursorDiameter);
		BrushCursorWidget->SetDesiredSizeInViewport(FVector2D(CursorDiameter, CursorDiameter));
		const FVector2D CursorPosition = bEyedropperModeActive
			? FVector2D(MouseX - CursorDiameter * 0.18f, MouseY - CursorDiameter * 0.78f)
			: FVector2D(MouseX - CursorDiameter * 0.5f, MouseY - CursorDiameter * 0.5f);
		BrushCursorWidget->SetPositionInViewport(CursorPosition, true);
	}
}

void AChameleonHiderCharacter::AdjustBrushSize(float DeltaRadiusCm)
{
	const float MinRadius = FMath::Max(MinBrushRadiusCm, 0.1f);
	const float MaxRadius = FMath::Max(MaxBrushRadiusCm, MinRadius);
	BrushRadiusCm = FMath::Clamp(BrushRadiusCm + DeltaRadiusCm, MinRadius, MaxRadius);
	if (bColorPickerVisible)
	{
		UpdateBrushCursorPosition();
	}
}

float AChameleonHiderCharacter::CalculateBrushCursorDiameterPixels(const APlayerController& PlayerController) const
{
	const float MinDiameter = FMath::Max(MinBrushCursorDiameterPixels, 4.0f);
	const float MaxDiameter = FMath::Max(MaxBrushCursorDiameterPixels, MinDiameter);
	const float FallbackDiameter = FMath::Clamp(
		BrushRadiusCm * FMath::Max(BrushCursorFallbackPixelsPerCm, 0.1f) * 2.0f,
		MinDiameter,
		MaxDiameter);

	FHitResult Hit;
	if (!TraceFromView(BrushTraceDistance, Hit, true))
	{
		return FallbackDiameter;
	}

	FVector ViewLocation;
	FVector ViewDirection;
	if (!PlayerController.DeprojectMousePositionToWorld(ViewLocation, ViewDirection))
	{
		return FallbackDiameter;
	}

	const FVector SurfaceNormal = Hit.ImpactNormal.GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
	FVector TangentA = FVector::CrossProduct(SurfaceNormal, ViewDirection).GetSafeNormal();
	if (TangentA.IsNearlyZero())
	{
		TangentA = FVector::CrossProduct(SurfaceNormal, FVector::UpVector).GetSafeNormal();
	}
	if (TangentA.IsNearlyZero())
	{
		TangentA = FVector::CrossProduct(SurfaceNormal, FVector::RightVector).GetSafeNormal();
	}

	const FVector TangentB = FVector::CrossProduct(TangentA, SurfaceNormal).GetSafeNormal();
	if (TangentA.IsNearlyZero() || TangentB.IsNearlyZero())
	{
		return FallbackDiameter;
	}

	FVector2D CenterScreen;
	FVector2D TangentAScreen;
	FVector2D TangentBScreen;
	if (!PlayerController.ProjectWorldLocationToScreen(Hit.ImpactPoint, CenterScreen, true)
		|| !PlayerController.ProjectWorldLocationToScreen(Hit.ImpactPoint + TangentA * BrushRadiusCm, TangentAScreen, true)
		|| !PlayerController.ProjectWorldLocationToScreen(Hit.ImpactPoint + TangentB * BrushRadiusCm, TangentBScreen, true))
	{
		return FallbackDiameter;
	}

	const float RadiusA = FVector2D::Distance(CenterScreen, TangentAScreen);
	const float RadiusB = FVector2D::Distance(CenterScreen, TangentBScreen);
	const float ScreenRadius = FMath::Max(RadiusA, RadiusB);
	if (!FMath::IsFinite(ScreenRadius) || ScreenRadius <= 0.0f)
	{
		return FallbackDiameter;
	}

	return FMath::Clamp(ScreenRadius * 2.0f, MinDiameter, MaxDiameter);
}

void AChameleonHiderCharacter::SetColorPickerVisible(bool bVisible)
{
	bColorPickerVisible = bVisible;
	if (!bVisible)
	{
		SetEyedropperModeActive(false);
	}

	if (ColorPickerWidget)
	{
		ColorPickerWidget->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (PlayerController)
	{
		PlayerController->bShowMouseCursor = bVisible;
		if (bVisible)
		{
			EnsureBrushCursor();
			if (BrushCursorWidget)
			{
				BrushCursorWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
				BrushCursorWidget->SetCursorMode(bEyedropperModeActive ? EChameleonBrushCursorMode::Eyedropper : EChameleonBrushCursorMode::Brush);
				UpdateBrushCursorPosition();
			}
			SetActorTickEnabled(true);

			FInputModeGameAndUI InputMode;
			InputMode.SetHideCursorDuringCapture(false);
			InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			PlayerController->SetInputMode(InputMode);
		}
		else
		{
			if (BrushCursorWidget)
			{
				BrushCursorWidget->SetVisibility(ESlateVisibility::Collapsed);
			}
			SetActorTickEnabled(false);
			FInputModeGameOnly InputMode;
			PlayerController->SetInputMode(InputMode);
		}
	}
}

void AChameleonHiderCharacter::SetEyedropperModeActive(bool bActive)
{
	if (bEyedropperModeActive == bActive)
	{
		if (ColorPickerWidget)
		{
			ColorPickerWidget->SetEyedropperModeActive(bEyedropperModeActive, false);
		}
		return;
	}

	bEyedropperModeActive = bActive;
	bPaintHeld = false;
	if (!bEyedropperModeActive)
	{
		bHasEyedropperPreviewColor = false;
	}

	if (ColorPickerWidget)
	{
		ColorPickerWidget->SetEyedropperModeActive(bEyedropperModeActive, false);
	}
	if (BrushCursorWidget)
	{
		BrushCursorWidget->SetCursorMode(bEyedropperModeActive ? EChameleonBrushCursorMode::Eyedropper : EChameleonBrushCursorMode::Brush);
		BrushCursorWidget->SetSamplePreviewColor(EyedropperPreviewColor, bEyedropperModeActive && bHasEyedropperPreviewColor);
	}
	UpdateBrushCursorPosition();
}

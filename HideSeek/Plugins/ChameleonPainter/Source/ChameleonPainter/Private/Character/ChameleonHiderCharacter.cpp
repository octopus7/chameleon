#include "Character/ChameleonHiderCharacter.h"

#include "Blueprint/UserWidget.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/ChameleonMetaballBodyComponent.h"
#include "Components/ChameleonPaintComponent.h"
#include "Data/ChameleonPainterInputConfig.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Game/ChameleonPainterGameInstance.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputCoreTypes.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialParameters.h"
#include "UI/ChameleonColorPickerWidget.h"

AChameleonHiderCharacter::AChameleonHiderCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;
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
	const FVector2D Axis = Value.Get<FVector2D>();
	AddControllerYawInput(Axis.X);
	AddControllerPitchInput(-Axis.Y);
}

void AChameleonHiderCharacter::StartPaint()
{
	bPaintHeld = true;
	PaintTriggered();
}

void AChameleonHiderCharacter::StopPaint()
{
	bPaintHeld = false;
}

void AChameleonHiderCharacter::PaintTriggered()
{
	if (!bPaintHeld || !BodyComponent)
	{
		return;
	}

	FHitResult Hit;
	if (TraceFromView(BrushTraceDistance, Hit, true))
	{
		BodyComponent->ApplyPaintStrokeFromHit(Hit, BrushRadiusCm, CurrentBrushColor, BrushStrength);
	}
}

void AChameleonHiderCharacter::SampleColor()
{
	FHitResult Hit;
	FLinearColor SampledColor;
	if (TraceFromView(SampleTraceDistance, Hit, false) && TrySampleColorFromHit(Hit, SampledColor))
	{
		CurrentBrushColor = SampledColor;
		if (PaintComponent)
		{
			PaintComponent->SetPaintColor(CurrentBrushColor);
		}
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

void AChameleonHiderCharacter::HandlePickerColorChanged(FLinearColor Color)
{
	Color.A = 1.0f;
	CurrentBrushColor = Color;
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
}

bool AChameleonHiderCharacter::TraceFromView(float Distance, FHitResult& OutHit, bool bTraceSelfBody) const
{
	const APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController)
	{
		return false;
	}

	FVector ViewLocation;
	FRotator ViewRotation;
	PlayerController->GetPlayerViewPoint(ViewLocation, ViewRotation);
	const FVector End = ViewLocation + ViewRotation.Vector() * Distance;

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
		ColorPickerWidget->OnColorChanged.AddDynamic(this, &AChameleonHiderCharacter::HandlePickerColorChanged);
		ColorPickerWidget->AddToViewport(100);
		ColorPickerWidget->SetAlignmentInViewport(FVector2D::ZeroVector);
		ColorPickerWidget->SetPositionInViewport(FVector2D(24.0f, 24.0f), false);
		ColorPickerWidget->SetDesiredSizeInViewport(FVector2D(320.0f, 220.0f));
	}
}

void AChameleonHiderCharacter::EnsureBrushCursor()
{
	if (BrushCursorWidget || !IsLocallyControlled() || !BrushCursorWidgetClass)
	{
		return;
	}

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController)
	{
		return;
	}

	BrushCursorWidget = CreateWidget<UUserWidget>(PlayerController, BrushCursorWidgetClass);
}

void AChameleonHiderCharacter::SetColorPickerVisible(bool bVisible)
{
	bColorPickerVisible = bVisible;

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
				PlayerController->CurrentMouseCursor = EMouseCursor::Default;
				PlayerController->SetMouseCursorWidget(EMouseCursor::Default, BrushCursorWidget);
			}

			FInputModeGameAndUI InputMode;
			InputMode.SetHideCursorDuringCapture(false);
			InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			PlayerController->SetInputMode(InputMode);
		}
		else
		{
			PlayerController->SetMouseCursorWidget(EMouseCursor::Default, nullptr);
			FInputModeGameOnly InputMode;
			PlayerController->SetInputMode(InputMode);
		}
	}
}

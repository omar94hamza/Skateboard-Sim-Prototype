// Copyright Epic Games, Inc. All Rights Reserved.

#include "SkateboardSimCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

//////////////////////////////////////////////////////////////////////////
// ASkateboardSimCharacter

ASkateboardSimCharacter::ASkateboardSimCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)


	/** Initialize Speed Values */
	BaseSpeed = GetCharacterMovement()->MaxWalkSpeed;	//Default / Minimum Skating Speed = 500
	MaxSpeed = BaseSpeed * 2.1;							//Maximum Speed after push
	PushSpeed = BaseSpeed * 0.25;						//Speed increment during a push
	CurrentSpeed = BaseSpeed;							//Current Speed of the character
	BrakeRate = BaseSpeed * 0.125;						//Brake Speed Decrementing
	bIsPushing = false;									// Initialize pushing state
	bIsBraking = false;									// Initialize braking state
}

void ASkateboardSimCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	//Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}

	OmarLog("Begin Play");
	OmarLog("Current Speed = " + FString::SanitizeFloat(CurrentSpeed));
}

//////////////////////////////////////////////////////////////////////////
// Input

void ASkateboardSimCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ASkateboardSimCharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ASkateboardSimCharacter::Look);

		//Speeding Up
		EnhancedInputComponent->BindAction(SpeedUpAction, ETriggerEvent::Triggered, this, &ASkateboardSimCharacter::StartSpeedingUp);

		//Braking
		EnhancedInputComponent->BindAction(BrakeAction, ETriggerEvent::Triggered, this, &ASkateboardSimCharacter::StartBraking);
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void ASkateboardSimCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	
		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);

		// Ensure proper speed update after stopping braking
		if (CurrentSpeed == 0.0f)
		{
			CurrentSpeed = BaseSpeed;  // Reset to BaseSpeed after braking stops
		}

		//Adjust Speed
		if (!bIsPushing && !bIsBraking)
		{
			UpdateSpeed(0.0f, BaseSpeed);
		}
	}
}

void ASkateboardSimCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void ASkateboardSimCharacter::StartSpeedingUp()
{

	if (CurrentSpeed < MaxSpeed)
	{
		float TargetSpeed = FMath::Min(CurrentSpeed + PushSpeed, MaxSpeed);

		// Smoothly interpolate CurrentSpeed towards TargetSpeed
		float SpeedLerpAlpha = 0.25f;
		CurrentSpeed = FMath::Lerp(CurrentSpeed, TargetSpeed, SpeedLerpAlpha);
		UpdateSpeed(PushSpeed, CurrentSpeed + PushSpeed);
		bIsPushing = true;
		OmarLog(FString::SanitizeFloat(CurrentSpeed));
	}

	GetWorldTimerManager().SetTimer(SpeedResetTimerHandle, this, &ASkateboardSimCharacter::ResetSpeedAfterPush, 1.5f, false);
}

void ASkateboardSimCharacter::ResetSpeedAfterPush()
{
	UpdateSpeed(0, BaseSpeed);
	bIsPushing = false;
}

void ASkateboardSimCharacter::SetPushingState()
{
	bIsPushing = true;
}

void ASkateboardSimCharacter::StartBraking()
{
	if (CurrentSpeed > 0)
	{
		bIsBraking = true;
		GetWorldTimerManager().SetTimer(BrakeResetTimerHandle, this, &ASkateboardSimCharacter::ApplyBraking, 0.1f, true);
	}
}

void ASkateboardSimCharacter::ApplyBraking()
{
	if (bIsBraking)
	{
		UpdateSpeed(-BrakeRate, CurrentSpeed-BrakeRate);
	}

	// Clamp speed to zero to ensure we don't go below 0
	if (CurrentSpeed <= 0.0f)
	{
		CurrentSpeed = 0.0f;
		bIsBraking = false;

		// Clean up the timer once we stop braking
		GetWorldTimerManager().ClearTimer(BrakeResetTimerHandle);

		OmarLog("Speed reached zero after braking.");
	}
}

void ASkateboardSimCharacter::UpdateSpeed(float DeltaSpeed, float ourMaxSpeed)
{
	CurrentSpeed += DeltaSpeed;
	CurrentSpeed = FMath::Clamp(CurrentSpeed, 0.0f, ourMaxSpeed);
	GetCharacterMovement()->MaxWalkSpeed = CurrentSpeed;
}

void ASkateboardSimCharacter::OmarLog(FString Message)
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 20.0f, FColor::Red, FString::Printf(TEXT("%s"), *Message));
	}
}
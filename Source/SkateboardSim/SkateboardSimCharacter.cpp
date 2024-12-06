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
#include <Kismet/GameplayStatics.h>
#include "ObstacleActor.h"
#include "ObstacleCollisionManager.h"

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
	SpeedRecoveryRate = BaseSpeed * 0.5;				//Speed Recovery after Brake

	bIsPushing = false;									// Initialize pushing state
	bIsBraking = false;									// Initialize braking state

	/** Scoring */
	TotalScore = 0;										// Total score of the player
	ObstacleHitPenalty = 5.0f;							// Penalty for hitting obstacles
	ObstacleJumpReward = 10.0f;							// Reward for successfully jumping over obstacles
}

void ASkateboardSimCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	// Set the "Player" tag for identification
	Tags.Add(FName("Player"));

	//Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}

	TArray<AActor*> FoundManagers;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AObstacleCollisionManager::StaticClass(), FoundManagers);

	if (FoundManagers.Num() > 0)
	{
		ObstacleCollisionManager = Cast<AObstacleCollisionManager>(FoundManagers[0]);
		
		// Find all ObstacleActor instances and assign the collision manager
		TArray<AActor*> FoundObstacles;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), AObstacleActor::StaticClass(), FoundObstacles);

		for (AActor* ObstacleActor : FoundObstacles)
		{
			AObstacleActor* Obstacle = Cast<AObstacleActor>(ObstacleActor);
			if (Obstacle)
			{
				Obstacle->SetCollisionManager(ObstacleCollisionManager);
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("No ObstacleCollisionManager found in the level."));
	}
}

void ASkateboardSimCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsBraking && CurrentSpeed > 0)
	{
		// While braking, reduce speed smoothly
		UpdateSpeed(-BrakeRate * DeltaTime, BaseSpeed);
	}
	else
	{
		// Smoothly recover speed back to BaseSpeed when button is released
		if (CurrentSpeed <= BaseSpeed)
		{
			float RecoveryAmount = SpeedRecoveryRate * DeltaTime;
			UpdateSpeed(RecoveryAmount, BaseSpeed);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// Input

void ASkateboardSimCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ASkateboardSimCharacter::StartJumping);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ASkateboardSimCharacter::EndJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ASkateboardSimCharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ASkateboardSimCharacter::Look);

		//Speeding Up
		EnhancedInputComponent->BindAction(SpeedUpAction, ETriggerEvent::Triggered, this, &ASkateboardSimCharacter::StartSpeedingUp);

		//Braking
		EnhancedInputComponent->BindAction(BrakeAction, ETriggerEvent::Started, this, &ASkateboardSimCharacter::StartBraking);
		EnhancedInputComponent->BindAction(BrakeAction, ETriggerEvent::Completed, this, &ASkateboardSimCharacter::StopBraking);
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
		UpdateSpeed(PushSpeed);
		bIsPushing = true;
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
		bIsBraking = true;
}

void ASkateboardSimCharacter::StopBraking()
{
	bIsBraking = false;
}


void ASkateboardSimCharacter::UpdateSpeed(float DeltaSpeed)
{
	UpdateSpeed(DeltaSpeed, CurrentSpeed + DeltaSpeed);
}

void ASkateboardSimCharacter::UpdateSpeed(float DeltaSpeed, float ourMaxSpeed)
{
	CurrentSpeed += DeltaSpeed;
	CurrentSpeed = FMath::Clamp(CurrentSpeed, 0.0f, ourMaxSpeed);
	GetCharacterMovement()->MaxWalkSpeed = CurrentSpeed;
}

void ASkateboardSimCharacter::StartJumping()
{
	// Call base jump method
	Super::Jump();

	// Check if we successfully jump over an obstacle
	CheckForObstaclesOnJump();
}

void ASkateboardSimCharacter::EndJumping()
{
	//Call base stop jump method
	Super::StopJumping();
}

void ASkateboardSimCharacter::CheckForObstaclesOnJump()
{
	TArray<AActor*> OverlappingActors;
	GetCapsuleComponent()->GetOverlappingActors(OverlappingActors);

	for (AActor* Actor : OverlappingActors)
	{
		if (Actor->ActorHasTag("Obstacle"))
		{
			TotalScore += ObstacleJumpReward;
			UpdateHUDScore();
			return;
		}
	}
}

// Subtract points for failing obstacles
void ASkateboardSimCharacter::HandleObstacleCollision(AActor* Obstacle)
{
	TotalScore = FMath::Clamp(TotalScore - ObstacleHitPenalty, 0, TotalScore);
	UpdateHUDScore();
}

// Update HUD logic
void ASkateboardSimCharacter::UpdateHUDScore()
{
	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		if (PC->IsLocalController())
		{
			PC->ClientMessage(FString::Printf(TEXT("Score: %d"), TotalScore));
		}
	}
}
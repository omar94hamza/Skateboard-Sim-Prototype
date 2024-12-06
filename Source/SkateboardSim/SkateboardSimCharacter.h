// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "SkateboardSimCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;
class AObstacleCollisionManager;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

UCLASS(config=Game)
class ASkateboardSimCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;
	
	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LookAction;

	/** Speeding Up Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* SpeedUpAction;

	/** Brake Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* BrakeAction;

public:
	ASkateboardSimCharacter();
	

private:
	/* Speed Relative Variables */
	float BaseSpeed;						//Default Skating Speed (Max Speed with no Push)
	float MaxSpeed;							//Maximum Speed after push
	float PushSpeed;						//Speed increment during a push
	float CurrentSpeed;						//Current Speed of the character
	float BrakeRate;						//Rate of Speed Decrement per brake;
	float SpeedRecoveryRate;				//Rate of Speed Recovery after brake

	/** Timer Handles */
	FTimerHandle SpeedResetTimerHandle;		//Resetting Speed
	FTimerHandle BrakeResetTimerHandle;		// Resetting Brakes

	/** Scoring */
	int32 TotalScore;						// Total score of the player
	float ObstacleHitPenalty;				// Penalty for hitting obstacles
	float ObstacleJumpReward;				// Reward for successfully jumping over obstacles


	/** Push animation state for AnimBP */
	UPROPERTY(BlueprintReadWrite, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	bool bIsPushing;

	/** Brake animation state for AnimBP */
	UPROPERTY(BlueprintReadWrite, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	bool bIsBraking;

protected:
	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

	/** Speeding Up */
	void StartSpeedingUp();
	void ResetSpeedAfterPush();

	/** Set pushing state for AnimBP */
	void SetPushingState();


	/** Braking */
	void StartBraking();					//Call to Start Braking
	void StopBraking();						//Call to Stop Braking


	void UpdateSpeed(float DelatSpeed);

	/** Helper Functions */
	void UpdateSpeed(float DeltaSpeed, float ourMaxSpeed);

	void HandleObstacleCollision(AActor* Obstacle);

	void StartJumping();

	void EndJumping();

	void CheckForObstaclesOnJump();

	void UpdateHUDScore();


	// Reference to the obstacle collision manager
	AObstacleCollisionManager* ObstacleCollisionManager;

	//Logging
	void OmarLog(FString Message);

protected:
	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	// To add mapping context
	virtual void BeginPlay();

	void Tick(float DeltaTime);

public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
};


// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ObstacleActor.generated.h"

class UBoxComponent;

UCLASS()
class SKATEBOARDSIM_API AObstacleActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AObstacleActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Collision component for detecting overlaps
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UBoxComponent* MainCollision;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UBoxComponent* FailCollision;

	// Methods to bind overlap events
	UFUNCTION()
	void OnMainCollisionOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	void OnMainCollisionEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	UFUNCTION()
	void OnFailCollisionOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	// Tracks if fail collision was triggered
	bool bFailZoneTriggered;
	bool bHasCollided;


	UPROPERTY(EditAnywhere, Category = "Gameplay")
	int32 PositiveObstaclePointValue = 10;
	int32 NegativeObstaclePointValue = 5;

	FTimerHandle ResetOverlapFlagsTimerHandle;

	class AObstacleCollisionManager* CollisionManager;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void OnSuccessfulJump();
	void OnFailedJump();

	void SetCollisionManager(class AObstacleCollisionManager* Manager);

	void ResetOverlapFlags();







	//Logging
	void OmarLog(FString Message);
};

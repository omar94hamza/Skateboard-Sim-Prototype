// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ObstacleCollisionManager.generated.h"

UCLASS()
class SKATEBOARDSIM_API AObstacleCollisionManager : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AObstacleCollisionManager();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void AddScore(int32 Points);
	void SubtractScore(int32 Points);





	//Logging
	void OmarLog(FString Message);

private:
	int32 TotalScore;

};

// Fill out your copyright notice in the Description page of Project Settings.


#include "ObstacleCollisionManager.h"
#include "ObstacleActor.h"

// Sets default values
AObstacleCollisionManager::AObstacleCollisionManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	TotalScore = 0;

}

// Called when the game starts or when spawned
void AObstacleCollisionManager::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AObstacleCollisionManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AObstacleCollisionManager::AddScore(int32 Points)
{
	TotalScore += Points;
	OmarLog(FString::Printf(TEXT("Score: %d"), TotalScore));
}

void AObstacleCollisionManager::SubtractScore(int32 Points)
{
	TotalScore -= Points;
	OmarLog(FString::Printf(TEXT("Score: %d"), TotalScore));
}








void AObstacleCollisionManager::OmarLog(FString Message)
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 20.0f, FColor::Red, FString::Printf(TEXT("%s"), *Message));
	}
}


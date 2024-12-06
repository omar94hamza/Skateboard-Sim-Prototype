// Fill out your copyright notice in the Description page of Project Settings.


#include "ObstacleCollisionManager.h"
#include "ObstacleActor.h"

// Sets default values
AObstacleCollisionManager::AObstacleCollisionManager()
{
	PrimaryActorTick.bCanEverTick = true;
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
	OnScoreUpdated.Broadcast(TotalScore);
}

void AObstacleCollisionManager::SubtractScore(int32 Points)
{
	if (TotalScore != 0)
	{
		TotalScore -= Points;
		OnScoreUpdated.Broadcast(TotalScore);
	}
}


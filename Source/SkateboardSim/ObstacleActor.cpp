// Fill out your copyright notice in the Description page of Project Settings.


#include "ObstacleActor.h"
#include "ObstacleCollisionManager.h"
#include "Components/BoxComponent.h"

// Sets default values
AObstacleActor::AObstacleActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	// Create an empty root component to organize everything
	USceneComponent* ObstacleRoot = CreateDefaultSubobject<USceneComponent>(TEXT("ObstacleRoot"));
	RootComponent = ObstacleRoot;

	// Create and configure the main collision component
	MainCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("MainCollision"));
	MainCollision->SetupAttachment(RootComponent);
	MainCollision->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	MainCollision->SetGenerateOverlapEvents(true);
	MainCollision->SetBoxExtent(FVector(50.0f, 50.0f, 50.0f));

	// Create and configure the fail collision component
	FailCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("FailCollision"));
	FailCollision->SetupAttachment(RootComponent);
	FailCollision->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	FailCollision->SetGenerateOverlapEvents(true);

	// Position fail zone slightly above or in front of the main collision
	FailCollision->SetBoxExtent(FVector(25.0f, 25.0f, 25.0f));      // Adjust size as needed

	FailCollision->SetHiddenInGame(false);
	MainCollision->SetHiddenInGame(false);
}

// Called when the game starts or when spawned
void AObstacleActor::BeginPlay()
{
	Super::BeginPlay();

	if (MainCollision)
	{
		MainCollision->OnComponentBeginOverlap.AddDynamic(this, &AObstacleActor::OnMainCollisionOverlap);
	}

	if (FailCollision)
	{
		FailCollision->OnComponentBeginOverlap.AddDynamic(this, &AObstacleActor::OnFailCollisionOverlap);
	}

	bHasCollided = false;
	bFailZoneTriggered = false;
}

void AObstacleActor::OnMainCollisionOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor && OtherActor->ActorHasTag(TEXT("Player")))
	{
		if (!bFailZoneTriggered && !bHasCollided)
		{
			bHasCollided = true;
			// Player cleared the obstacle successfully
			OnSuccessfulJump();
		}

		bHasCollided = false;
	}

	// Start a timer to reset flags after a short delay
	GetWorld()->GetTimerManager().SetTimer(ResetOverlapFlagsTimerHandle, this, &AObstacleActor::ResetOverlapFlags, 0.1f, false);
}

void AObstacleActor::OnFailCollisionOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor && OtherActor->ActorHasTag(TEXT("Player")))
	{
		bFailZoneTriggered = true;  // Mark the fail zone as triggered
		OnFailedJump();
	}
}

// Called every frame
void AObstacleActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AObstacleActor::OnSuccessfulJump()
{
	if (CollisionManager)
	{
		CollisionManager->AddScore(PositiveObstaclePointValue);
	}
}

void AObstacleActor::OnFailedJump()
{
	if (CollisionManager)
	{
		CollisionManager->SubtractScore(NegativeObstaclePointValue);
	}
}

void AObstacleActor::SetCollisionManager(AObstacleCollisionManager* Manager)
{
	CollisionManager = Manager;
}

void AObstacleActor::ResetOverlapFlags()
{
	bHasCollided = false;
	bFailZoneTriggered = false;

	UE_LOG(LogTemp, Warning, TEXT("Flags reset after overlap."));
}


// Fill out your copyright notice in the Description page of Project Settings.


#include "ObstacleActor.h"
#include "ObstacleCollisionManager.h"
#include "Components/BoxComponent.h"

// Sets default values
AObstacleActor::AObstacleActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	ActorHasTag(TEXT("Obstacle"));

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
}

void AObstacleActor::OnMainCollisionOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	OmarLog("OnMainCollisionOverlap");
	if (OtherActor && OtherActor->ActorHasTag(TEXT("Player")))
	{
		OmarLog("Player Found");
		if (CollisionManager)
		{
			OmarLog("Collision Manager Found");
			if (bFailZoneTriggered)
			{
				OmarLog("Failed Trigger True");
				// Player hit both Main and Fail zones (failure)
				CollisionManager->SubtractScore(NegativeObstaclePointValue);
				UE_LOG(LogTemp, Warning, TEXT("Player failed! Penalty applied."));
			}
			else
			{
				OmarLog("FailedTrigger False");
				// Player cleared the obstacle successfully
				CollisionManager->AddScore(PositiveObstaclePointValue);
				UE_LOG(LogTemp, Warning, TEXT("Player succeeded! Points awarded."));
			}
		}

		// Reset the fail zone trigger for future collisions
		bFailZoneTriggered = false;
	}
}

void AObstacleActor::OnFailCollisionOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	OmarLog("OnFailCollisionOverlap");
	if (OtherActor && OtherActor->ActorHasTag(TEXT("Player")))
	{
		OmarLog("Player Found at Fail Zone and Trigger True");
		// Player touched the fail zone
		bFailZoneTriggered = true;
		UE_LOG(LogTemp, Warning, TEXT("Fail zone triggered!"));
	}
}

// Called every frame
void AObstacleActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AObstacleActor::NotifyActorBeginOverlap(AActor* OtherActor)
{
	if (OtherActor && OtherActor->ActorHasTag(TEXT("Player")))
	{
		if (CollisionManager)
		{
			FVector PlayerLocation = OtherActor->GetActorLocation();
			FVector ObstacleLocation = GetActorLocation();

			float DistanceAboveObstacle = PlayerLocation.Z - ObstacleLocation.Z;

			if (DistanceAboveObstacle > 100.0f)
			{
				OnSuccessfulJump();
			}
			else
			{
				OnFailedJump();
			}
		}
	}
}

void AObstacleActor::OnSuccessfulJump()
{
	if (CollisionManager)
	{
		CollisionManager->AddScore(PositiveObstaclePointValue);
		OmarLog("Successful Jump");
	}
}

void AObstacleActor::OnFailedJump()
{
	if (CollisionManager)
	{
		CollisionManager->SubtractScore(NegativeObstaclePointValue);
		OmarLog("Failed Jump");
	}
}

void AObstacleActor::SetCollisionManager(AObstacleCollisionManager* Manager)
{
	CollisionManager = Manager;
}








void AObstacleActor::OmarLog(FString Message)
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 20.0f, FColor::Red, FString::Printf(TEXT("%s"), *Message));
	}
}


// Fill out your copyright notice in the Description page of Project Settings.


#include "MyCPPGameModeBase.h"

#include "EngineUtils.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/PlayerStart.h"
#include "../GameComponents/MyPlayerState.h"
#include "GameFramework/GameModeBase.h"
#include "../GameComponents/MyPlayerStart.h"
#include "testinginBP/Character/CPPTestCharacter.h"
#include "testinginBP/GameComponents/SpawnPoint.h"
#include "TimerManager.h"

AMyCPPGameModeBase::AMyCPPGameModeBase() 
{
	PlayerStateClass = AMyPlayerState::StaticClass();

	DefaultSpawnLocation = FVector(500.002479, -830.000000, 232.001526);
}

void AMyCPPGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	UClass* SpawnPointClass = ASpawnPoint::StaticClass();
	for (TActorIterator<AActor> Point (GetWorld(),SpawnPointClass); Point; ++Point)
	{
		SpawnPoints.Add(Cast<ASpawnPoint>(*Point));
	}

}

void AMyCPPGameModeBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AMyCPPGameModeBase, CurrentPawnToAssign);

}



UClass* AMyCPPGameModeBase::GetDefaultPawnClassForController_Implementation(AController* InController)
{
	if (CurrentPawnToAssign)
	{
		if (FirstPawn != nullptr && SecondPawn != nullptr )
		{
			if (CurrentPawnToAssign == FirstPawn)
			{
				CurrentPawnToAssign = SecondPawn;
			}
			
			else
			{
				CurrentPawnToAssign = FirstPawn;

			}

		}
	}
	else
	{
		if (FirstPawn != nullptr && SecondPawn != nullptr )
		{
			CurrentPawnToAssign = (true) ? FirstPawn : SecondPawn;
		}
	

	
	}
	return CurrentPawnToAssign;
}

ASpawnPoint* AMyCPPGameModeBase::GetSpawnPoint()
{
	for (int32 i = 0; i < SpawnPoints.Num(); i++)
	{
		int32 Point = FMath::RandRange(0, SpawnPoints.Num() - 1);

		if(SpawnPoints[Point])
		{
			return SpawnPoints[Point];
		}
	}
	return nullptr;
}

void AMyCPPGameModeBase::Spawn(AController* Controller)
{
	
	if (ASpawnPoint* SpawnPoint = GetSpawnPoint()) {

		FVector Location = SpawnPoint->GetActorLocation();
		FRotator Rotation = SpawnPoint->GetActorRotation();
		if (APawn* Pawn = GetWorld()->SpawnActor<APawn>(Controller->GetPawn()->GetClass(), Location, Rotation))
		{
			Controller->Possess(Pawn);
		}
	}
	else
	{
		FVector Location = DefaultSpawnLocation;
		FRotator Rotation = FRotator::ZeroRotator;
		if (APawn* Pawn = GetWorld()->SpawnActor<APawn>(Controller->GetPawn()->GetClass(), Location, Rotation))
		{
			Controller->Possess(Pawn);
		}
	}

	
}



void AMyCPPGameModeBase::Respawn(AController* Controller)
{
	if (Controller) {

		if (GetLocalRole() == ROLE_Authority)
		{

			FTimerDelegate RespawnDelegate;
			RespawnDelegate.BindUFunction(this, FName("Spawn"), Controller);

			GetWorld()->GetTimerManager().SetTimer(RespawnHandle, RespawnDelegate, RespawnTime, false);

			if(ACPPTestCharacter* MyChar = Cast<ACPPTestCharacter>(Controller->GetCharacter()))
			{
				MyChar->ClientRespawnCountDown(5);
			}
			
		} 
	}
}

// Fill out your copyright notice in the Description page of Project Settings.

#include "Chase.h"
#include "ChaseGameState.h"
#include "UnrealNetwork.h"
#include "ChaseCharacter.h"

AChaseGameState::AChaseGameState(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bReplicates = true;
	CurrentGameState = EChaseGameState::Waiting;

	PrimaryActorTick.bCanEverTick = true;
}

void AChaseGameState::GetLifetimeReplicatedProps(TArray< class FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AChaseGameState, CurrentGameState);
	DOREPLIFETIME(AChaseGameState, TimeUntilNextState);
	DOREPLIFETIME(AChaseGameState, Winner);
}

void AChaseGameState::StartGame()
{
	if (Role < ROLE_Authority || CurrentGameState != EChaseGameState::Waiting)
		return;

	CurrentGameState = EChaseGameState::Setup;

	TArray<AChaseCharacter*> Characters;
	for (TActorIterator<AChaseCharacter> ActorItr(GetWorld()); ActorItr; ++ActorItr)
	{
		Characters.Add(*ActorItr);
	}

	int32 VictimIndex = FMath::RandHelper(Characters.Num());
	AChaseCharacter* Victim = Characters[VictimIndex];
	Characters.RemoveAt(VictimIndex);
	Victim->Team = EChaseTeam::Victim;

	for (auto Character : Characters)
	{
		Character->Team = EChaseTeam::Chaser;
	}

	TimeUntilNextState = 10.f;

	GameStateChanged();
}

void AChaseGameState::FinishGame(AChaseCharacter* Winner)
{
	if (Role < ROLE_Authority || CurrentGameState != EChaseGameState::Playing)
		return;


	this->Winner = EChaseTeam::Chaser;
	CurrentGameState = EChaseGameState::Finished;
	TimeUntilNextState = 0.f;
	GameStateChanged();
}

void AChaseGameState::GameStateChanged()
{
	AChaseCharacter* Character = Cast<AChaseCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	if (!Character || !Character->IsValidLowLevel())
		return;

	Character->GameStateChanged();

	for (TActorIterator<AChaseCharacter> ActorItr(GetWorld()); ActorItr; ++ActorItr)
	{
		ActorItr->GetMesh()->SetMaterial(0, ActorItr->Team == EChaseTeam::Chaser ? ActorItr->ChaserMaterial : ActorItr->VictimMaterial);
	}
}

void AChaseGameState::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (Role < ROLE_Authority)
		return;

	if (TimeUntilNextState >= 0)
	{
		TimeUntilNextState -= DeltaSeconds;
	}
	else
	{
		switch (CurrentGameState)
		{
		case EChaseGameState::Setup:
			CurrentGameState = EChaseGameState::Playing;
			TimeUntilNextState = 60.f * 3.f;
			GameStateChanged();
			break;

		case EChaseGameState::Playing:
			CurrentGameState = EChaseGameState::Finished;
			Winner = EChaseTeam::Victim;
			GameStateChanged();
			break;
		}
	}
}
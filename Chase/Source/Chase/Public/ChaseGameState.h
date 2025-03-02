// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/GameState.h"
#include "ChaseCharacter.h"
#include "ChaseGameState.generated.h"

UENUM(BlueprintType)
enum class EChaseGameState : uint8
{
	Waiting,
	Setup,
	Playing,
	Finished
};

class AChaseCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGameStateChangedSignature);

/**
 * 
 */
UCLASS()
class CHASE_API AChaseGameState : public AGameState
{
	GENERATED_BODY()
	
public:
	AChaseGameState(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Game, Replicated, ReplicatedUsing = GameStateChanged)
	EChaseGameState CurrentGameState;

	UFUNCTION(BlueprintCallable, Category=Game)
	void StartGame();

	void FinishGame(AChaseCharacter* Winner);

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category=Game, Replicated)
	float TimeUntilNextState;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category=Game, Replicated)
	EChaseTeam Winner;

	UPROPERTY(BlueprintAssignable, Category=Game)
	FGameStateChangedSignature OnGameStateChanged;

protected:
	UFUNCTION()
	void GameStateChanged();

	virtual void Tick( float DeltaSeconds ) override;
};

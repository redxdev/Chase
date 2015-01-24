// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/PlayerState.h"
#include "ChasePlayerState.generated.h"

UENUM()
enum class EChaseTeam
{
	Chaser,
	BeingChased
};

UCLASS()
class CHASE_API AChasePlayerState : public APlayerState
{
	GENERATED_BODY()
	
public:
	AChasePlayerState(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Game)
	TEnumAsByte<EChaseTeam> Team;
};

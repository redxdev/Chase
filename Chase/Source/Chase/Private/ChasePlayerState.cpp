// Fill out your copyright notice in the Description page of Project Settings.

#include "Chase.h"
#include "ChasePlayerState.h"

AChasePlayerState::AChasePlayerState(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Team = EChaseTeam::Chaser;
}
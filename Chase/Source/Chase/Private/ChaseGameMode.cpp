// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Chase.h"
#include "ChaseGameMode.h"
#include "ChaseCharacter.h"

AChaseGameMode::AChaseGameMode(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/Blueprints/PlayerCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "MovementComponentGameMode.h"
#include "MovementComponentHUD.h"
#include "MovementComponentCharacter.h"
#include "UObject/ConstructorHelpers.h"

AMovementComponentGameMode::AMovementComponentGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPersonCPP/Blueprints/FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

	// use our custom HUD class
	HUDClass = AMovementComponentHUD::StaticClass();
}

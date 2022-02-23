// Copyright Epic Games, Inc. All Rights Reserved.

#include "learnUnrealForOnceGameMode.h"
#include "learnUnrealForOnceCharacter.h"
#include "UObject/ConstructorHelpers.h"

AlearnUnrealForOnceGameMode::AlearnUnrealForOnceGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPersonCPP/Blueprints/ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}

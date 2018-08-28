// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Pinball.h"
#include "PinballGameMode.h"
#include "PinballBall.h"

APinballGameMode::APinballGameMode(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// set default pawn class to our ball
	DefaultPawnClass = APinballBall::StaticClass();
}

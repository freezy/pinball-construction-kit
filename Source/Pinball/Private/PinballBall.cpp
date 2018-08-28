// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Pinball.h"
#include "PinballBall.h"

APinballBall::APinballBall(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Create mesh component for the ball
	Ball = ObjectInitializer.CreateDefaultSubobject<UStaticMeshComponent>(this, TEXT("Ball0"));
	RootComponent = Ball;

	// Set up forces
	RollTorque = 50000000.0f;
}
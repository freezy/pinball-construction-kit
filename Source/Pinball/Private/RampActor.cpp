// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "Pinball.h"
#include "RampActor.h"

ARampActor::ARampActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;
}

// Sets default values
ARampActor::ARampActor()
{
	// turned this off to improve performance 
	PrimaryActorTick.bCanEverTick = false;
}

// Called when the game starts or when spawned
void ARampActor::BeginPlay()
{
	Super::BeginPlay();
}



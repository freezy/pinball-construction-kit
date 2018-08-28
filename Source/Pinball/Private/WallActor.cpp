// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "Pinball.h"
#include "WallActor.h"

AWallActor::AWallActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;
}

// Sets default values
AWallActor::AWallActor()
{
 	// turned this off to improve performance 
	PrimaryActorTick.bCanEverTick = false;
}

// Called when the game starts or when spawned
void AWallActor::BeginPlay()
{
	Super::BeginPlay();
}



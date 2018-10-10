// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SplineActor.h"
#include "RampActor.generated.h"

/**
 * 
 */
UCLASS()
class PINBALL_API ARampActor : public ASplineActor
{
	GENERATED_UCLASS_BODY()
	
public:
	// Sets default values for this actor's properties
	ARampActor();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	
};

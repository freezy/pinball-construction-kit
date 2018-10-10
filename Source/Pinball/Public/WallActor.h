// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SplineActor.h"
#include "WallActor.generated.h"


UCLASS()
class PINBALL_API AWallActor : public ASplineActor
{
	GENERATED_UCLASS_BODY()
	
public:	
	// Sets default values for this actor's properties
	AWallActor();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;	


};

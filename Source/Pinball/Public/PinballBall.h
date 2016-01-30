// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once
#include "GameFramework/Pawn.h"
#include "PinballBall.generated.h"

UCLASS(config=Game)
class APinballBall : public APawn
{
	GENERATED_UCLASS_BODY()

private:
	/** StaticMesh used for the ball */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Ball, meta = (AllowPrivateAccess = "true"))
	class UStaticMeshComponent* Ball;

public:

	/** Torque to apply when trying to roll ball */
	UPROPERTY(EditAnywhere, Category=Ball)
	float RollTorque;

public:
	/** Returns Ball subobject **/
	FORCEINLINE class UStaticMeshComponent* GetBall() const { return Ball; }

};

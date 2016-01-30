// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/SplineComponent.h"
#include "PinballSplineComponent.generated.h"

/**
 * NOTE: The AddSplinePointAtIndex and RemoveSplinePoint functions will be added to SplineComponent in an upcoming Unreal Engine release, probably 4.11
 */
UCLASS()
class PINBALL_API UPinballSplineComponent : public USplineComponent
{
	GENERATED_BODY()

public:
	/** Adds a point to the spline at the specified index*/
	UFUNCTION(BlueprintCallable, Category = Spline)
	void PinballAddSplinePointAtIndex(const FVector& Position, int32 Index, ESplineCoordinateSpace::Type CoordinateSpace);

	/** Removes point at specified index from the spline */
	UFUNCTION(BlueprintCallable, Category = Spline)
	void PinballRemoveSplinePoint(const int32 Index);
};

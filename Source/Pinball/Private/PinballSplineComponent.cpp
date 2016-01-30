// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "Pinball.h"
#include "PinballSplineComponent.h"



void UPinballSplineComponent::PinballAddSplinePointAtIndex(const FVector& Position, int32 Index, ESplineCoordinateSpace::Type CoordinateSpace)
{
	const FVector TransformedPosition = (CoordinateSpace == ESplineCoordinateSpace::World) ?
		ComponentToWorld.InverseTransformPosition(Position) : Position;

	const float InKey = static_cast<float>(Index);

	if (((Index >= 0) &&
		(Index < SplineInfo.Points.Num())) &&
		(Index < SplineRotInfo.Points.Num()) &&
		(Index < SplineScaleInfo.Points.Num()))
	{
		SplineInfo.Points.Insert(FInterpCurvePoint<FVector>(InKey, TransformedPosition, FVector::ZeroVector, FVector::ZeroVector, CIM_CurveAuto), Index);
		SplineRotInfo.Points.Insert(FInterpCurvePoint<FQuat>(InKey, FQuat::Identity, FQuat::Identity, FQuat::Identity, CIM_CurveAuto), Index);
		SplineScaleInfo.Points.Insert(FInterpCurvePoint<FVector>(InKey, FVector(1.0f), FVector::ZeroVector, FVector::ZeroVector, CIM_CurveAuto), Index);
	}

	UpdateSpline();
}

void UPinballSplineComponent::PinballRemoveSplinePoint(const int32 Index)
{
	int32 Count = 1;

	if ((Index >= 0) &&
		(SplineInfo.Points.Num() >= 0) &&
		(SplineRotInfo.Points.Num() >= 0) &&
		(SplineScaleInfo.Points.Num() >= 0) &&
		(Index + Count <= SplineInfo.Points.Num()) &&
		(Index + Count <= SplineRotInfo.Points.Num()) &&
		(Index + Count <= SplineScaleInfo.Points.Num())
		)
	{
		SplineInfo.Points.RemoveAt(Index, Count, false);
		SplineRotInfo.Points.RemoveAt(Index, Count, false);
		SplineScaleInfo.Points.RemoveAt(Index, Count, false);
	}

	UpdateSpline();
}

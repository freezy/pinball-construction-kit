// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
// D:\P4\UE4\Engine\Source\Runtime\Engine\Classes\Kismet\BlueprintFunctionLibrary.h
#include "Runtime/Engine/Classes/Kismet/BlueprintFunctionLibrary.h"
#include "GeometryBlueprintLibrary.generated.h"



/**
 * Sa
 */
UCLASS()
class PINBALL_API UGeometryBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	// Just calls the same function defined in PaperGeomTools
	UFUNCTION(BlueprintCallable, Category = GeometryUtilities)
	static bool TriangulatePoly(TArray<FVector2D>& OutTris, const TArray<FVector2D>& InPolyVerts, bool bKeepColinearVertices);
	
};

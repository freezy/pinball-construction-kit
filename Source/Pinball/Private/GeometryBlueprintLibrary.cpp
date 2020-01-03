// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "GeometryBlueprintLibrary.h"
#include "GeomTools.h"

bool UGeometryBlueprintLibrary::TriangulatePoly(TArray<FVector2D>& OutTris, const TArray<FVector2D>& InPolyVerts, bool bKeepColinearVertices)
{
	return FGeomTools2D::TriangulatePoly(OutTris, InPolyVerts, bKeepColinearVertices);
}

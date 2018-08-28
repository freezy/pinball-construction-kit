// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "Pinball.h"
#include "GeometryBlueprintLibrary.h"
#include "../Plugins/2D/Paper2D/Source/Paper2D/Public/PaperGeomTools.h"

bool UGeometryBlueprintLibrary::TriangulatePoly(TArray<FVector2D>& OutTris, const TArray<FVector2D>& InPolyVerts, bool bKeepColinearVertices)
{
	return PaperGeomTools::TriangulatePoly(OutTris, InPolyVerts, bKeepColinearVertices);
}

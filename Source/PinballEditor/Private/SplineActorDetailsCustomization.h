// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PropertyEditorModule.h"
#include "IDetailCustomization.h"

class SSplineEditWidget;
class ASplineActor;

enum class ESplineInvertAxis : uint8
{
	InvertX,
	InvertY,
};

//////////////////////////////////////////////////////////////////////////
// FSplineActorDetailsCustomization

class FSplineActorDetailsCustomization : public IDetailCustomization
{
public:
	// Makes a new instance of this detail layout class for a specific detail view requesting it
	static TSharedRef<IDetailCustomization> MakeInstance();

	// IDetailCustomization interface
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailLayout) override;
	// End of IDetailCustomization interface

private:

	IDetailLayoutBuilder* MyDetailLayout;

	/** Invert the spline on the specified axis */
	FReply InvertSpline(ASplineActor* SplineActor, ESplineInvertAxis InvertAxis);

	/** Reset this spline to the default */
	FReply ResetSpline(ASplineActor* SplineActor);

	/** The spline editing widget in the details panel */
	TSharedPtr<SSplineEditWidget> SplineEditWidget;
};

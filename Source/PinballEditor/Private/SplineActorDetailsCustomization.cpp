// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "SplineActorDetailsCustomization.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "PropertyEditing.h"
#include "IPropertyUtilities.h"
#include "SlateBasics.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Fonts/SlateFontInfo.h"
#include "SSplineEditWidget.h"
#include "PropertyCustomizationHelpers.h"
#include "SplineActor.h"
#include "Components/SplineComponent.h"
#include "ScopedTransaction.h"
#include "Widgets/Input/SNumericEntryBox.h"

#define LOCTEXT_NAMESPACE "Pinball"

//////////////////////////////////////////////////////////////////////////
// FSplineActorDetailsCustomization

TSharedRef<IDetailCustomization> FSplineActorDetailsCustomization::MakeInstance()
{
	return MakeShareable(new FSplineActorDetailsCustomization);
}

void FSplineActorDetailsCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
	const TArray< TWeakObjectPtr<UObject> >& SelectedObjects = DetailLayout.GetDetailsView()->GetSelectedObjects();
	if (SelectedObjects.Num() > 1)
	{
		//Can only handle single selection
		return;
	}

	// Don't show Spline Component, Spline Mesh Component, and Reset Spline to Default properties
	DetailLayout.HideCategory(FName(TEXT("Spline")));

	// Add Spline actor category
	IDetailCategoryBuilder& SplineActorCategory = DetailLayout.EditCategory("Spline Actor", FText::GetEmpty(), ECategoryPriority::Important);

	ASplineActor* SplineActor = nullptr;
	for (int32 ObjectIndex = 0; ObjectIndex < SelectedObjects.Num(); ++ObjectIndex)
	{
		UObject* TestObject = SelectedObjects[ObjectIndex].Get();
		if (AActor* CurrentActor = Cast<AActor>(TestObject))
		{
			if (ASplineActor* TestSplineActor = Cast<ASplineActor>(CurrentActor))
			{
				SplineActor = TestSplineActor;
				break;
			}
		}
	}

	if (SplineActor != nullptr)
	{
		FDetailWidgetRow& SplineEditWidgetCustomRow = SplineActorCategory.AddCustomRow(FText::GetEmpty());

		//FQuat2D Rotation = FQuat2D(FMath::DegreesToRadians(90));
		//	/** Initialize the transform using a 2D rotation and a translation. */
		//explicit FTransform2D(const FQuat2D& Rot, const FVector2D& Translation = FVector2D(0.f, 0.f))
		//TOptional<FSlateRenderTransform> RenderTransform = TOptional<FSlateRenderTransform>(FSlateRenderTransform(Rotation));
		SplineEditWidgetCustomRow.WholeRowContent()
			[
				SAssignNew(SplineEditWidget, SSplineEditWidget)
				.SplineActor(SplineActor)
			];

		if (SplineActor->SplineComponent != nullptr)
		{
			// Closed loop checkbox
			TSharedRef<IPropertyHandle> SplinePropertyHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(ASplineActor, SplineComponent));
			//UProperty* Property = FindField<UProperty>(SplineActor->SplineComponent->StaticClass(), "SplineComponent");
			TSharedPtr<IPropertyHandle> ClosedLoopPropertyHandle = SplinePropertyHandle->GetChildHandle(FName(TEXT("bClosedLoop")));
			if (ClosedLoopPropertyHandle.IsValid())
			{
				SplineActorCategory.AddProperty(ClosedLoopPropertyHandle);
			}

			// Update spline meshes checkbox
			TSharedRef<IPropertyHandle> UpdatePropertyHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(ASplineActor, bUpdateSplineMeshes));
			SplineActorCategory.AddProperty(UpdatePropertyHandle);
		}

		// Add a row for hand entry of selected spline point locations
		FDetailWidgetRow& SplinePointLocationRow = SplineActorCategory.AddCustomRow(FText::GetEmpty());
		SplinePointLocationRow.NameContent()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("SplinePoint(s)Location", "Spline Point(s) Location"))
		];
		SplinePointLocationRow.ValueContent()
		.MaxDesiredWidth(10000)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.Padding(5, 0)
			.VAlign(VAlign_Center)
			.AutoWidth()
			[
				SNew(STextBlock)
				.Text(LOCTEXT("SplinePointLocationX", "X"))
			]
			+ SHorizontalBox::Slot()
			[
				SNew(SBox)
				.MinDesiredWidth(5000)
				[
					SNew(SNumericEntryBox<float>)
					.Delta(0.01f)
					.Value(SplineEditWidget.Get(), &SSplineEditWidget::OnGetSelectedSplinePointLocation, EAxis::X)
					//.OnValueChanged(SplineEditWidget.Get(), &SSplineEditWidget::OnSelectedSplinePointLocationChanged, EAxis::X)
					.OnValueCommitted(SplineEditWidget.Get(), &SSplineEditWidget::OnSelectedSplinePointLocationCommitted, EAxis::X)
					.IsEnabled(SplineEditWidget.Get(), &SSplineEditWidget::OnGetCanEditSelectedSplinePointLocationAndTangent)
					.UndeterminedString(LOCTEXT("MultipleValues", "Multiple Values"))
					//.AllowSpin(true)
				]
			]
			+ SHorizontalBox::Slot()
			.Padding(5, 0)
			.VAlign(VAlign_Center)
			.AutoWidth()
			[
				SNew(STextBlock)
				.Text(LOCTEXT("SplinePointLocationY", "Y"))
			]
			+ SHorizontalBox::Slot()
			[
				SNew(SBox)
				.MinDesiredWidth(5000)
				[
					SNew(SNumericEntryBox<float>)
					.Delta(0.01f)
					.Value(SplineEditWidget.Get(), &SSplineEditWidget::OnGetSelectedSplinePointLocation, EAxis::Y)
					//.OnValueChanged(SplineEditWidget.Get(), &SSplineEditWidget::OnSelectedSplinePointLocationChanged, EAxis::Y)
					.OnValueCommitted(SplineEditWidget.Get(), &SSplineEditWidget::OnSelectedSplinePointLocationCommitted, EAxis::Y)
					.IsEnabled(SplineEditWidget.Get(), &SSplineEditWidget::OnGetCanEditSelectedSplinePointLocationAndTangent)
					.UndeterminedString(LOCTEXT("MultipleValues", "Multiple Values"))
					//.AllowSpin(true)
				]
			]
				
		];

		// Add a row for hand entry of selected spline point tangents
		FDetailWidgetRow& SplinePointTangentsRow = SplineActorCategory.AddCustomRow(FText::GetEmpty());
		SplinePointTangentsRow.NameContent()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("SplinePoint(s)Tangent", "Spline Point(s) Tangent"))
		];
		SplinePointTangentsRow.ValueContent()
		.MaxDesiredWidth(10000)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.Padding(5, 0)
			.VAlign(VAlign_Center)
			.AutoWidth()
			[
				SNew(STextBlock)
				.Text(LOCTEXT("SplinePointTangentX", "X"))
			]
			+ SHorizontalBox::Slot()
			[
				SNew(SBox)
				.MinDesiredWidth(5000)
				[
					SNew(SNumericEntryBox<float>)
					.Delta(0.01f)
					.Value(SplineEditWidget.Get(), &SSplineEditWidget::OnGetSelectedSplinePointTangent, EAxis::X)
					//.OnValueChanged(SplineEditWidget.Get(), &SSplineEditWidget::OnSelectedSplinePointTangentChanged, EAxis::X)
					.OnValueCommitted(SplineEditWidget.Get(), &SSplineEditWidget::OnSelectedSplinePointTangentCommitted, EAxis::X)
					.IsEnabled(SplineEditWidget.Get(), &SSplineEditWidget::OnGetCanEditSelectedSplinePointLocationAndTangent)
					.UndeterminedString(LOCTEXT("MultipleValues", "Multiple Values"))
					//.AllowSpin(true)
				]
			]
			+ SHorizontalBox::Slot()
			.Padding(5, 0)
			.VAlign(VAlign_Center)
			.AutoWidth()
			[
				SNew(STextBlock)
				.Text(LOCTEXT("SplinePointTangentY", "Y"))
			]
			+ SHorizontalBox::Slot()
			[
				SNew(SBox)
				.MinDesiredWidth(5000)
				[
					SNew(SNumericEntryBox<float>)
					.Delta(0.01f)
					.Value(SplineEditWidget.Get(), &SSplineEditWidget::OnGetSelectedSplinePointTangent, EAxis::Y)
					//.OnValueChanged(SplineEditWidget.Get(), &SSplineEditWidget::OnSelectedSplinePointTangentChanged, EAxis::Y)
					.OnValueCommitted(SplineEditWidget.Get(), &SSplineEditWidget::OnSelectedSplinePointTangentCommitted, EAxis::Y)
					.IsEnabled(SplineEditWidget.Get(), &SSplineEditWidget::OnGetCanEditSelectedSplinePointLocationAndTangent)
					.UndeterminedString(LOCTEXT("MultipleValues", "Multiple Values"))
					//.AllowSpin(true)
				]
			]

		];

		// Add button to invert the spline on the X axis
		FDetailWidgetRow& SplineInvertXCustomRow = SplineActorCategory.AddCustomRow(FText::GetEmpty());
		SplineInvertXCustomRow.WholeRowContent()
			[
				SNew(SButton)
				.Text(LOCTEXT("InvertSplineX", "Invert Spline X"))
				.OnClicked(FOnClicked::CreateSP(this, &FSplineActorDetailsCustomization::InvertSpline, SplineActor, ESplineInvertAxis::InvertX))
			];

		// Add button to invert the spline on the Y axis
		FDetailWidgetRow& SplineInvertYCustomRow = SplineActorCategory.AddCustomRow(FText::GetEmpty());
		SplineInvertYCustomRow.WholeRowContent()
			[
				SNew(SButton)
				.Text(LOCTEXT("InvertSplineY", "Invert Spline Y"))
				.OnClicked(FOnClicked::CreateSP(this, &FSplineActorDetailsCustomization::InvertSpline, SplineActor, ESplineInvertAxis::InvertY))
			];

		// Reset spline
		FDetailWidgetRow& SplineResetRow = SplineActorCategory.AddCustomRow(FText::GetEmpty());
		SplineResetRow.WholeRowContent()
			[
				SNew(SButton)
				.Text(LOCTEXT("ResetSplineToDefault", "Reset Spline To Default"))
				.OnClicked(FOnClicked::CreateSP(this, &FSplineActorDetailsCustomization::ResetSpline, SplineActor))
			];
	}
	

	MyDetailLayout = &DetailLayout;
}

FReply FSplineActorDetailsCustomization::InvertSpline(ASplineActor* SplineActor, ESplineInvertAxis InvertAxis)
{
	// Scoped transaction for the undo buffer
	const FScopedTransaction Transaction(LOCTEXT("EditSplinePointType", "Invert Spline"));

	// Notify the component and actor that they will be modified
	SplineActor->SplineComponent->Modify();
	if (AActor* Owner = SplineActor->SplineComponent->GetOwner())
	{
		Owner->Modify();
	}

	if (SplineActor->SplineComponent != nullptr)
	{
		TArray<FVector> SplinePointLocations;
		TArray<FVector> SplinePointTangents;
		TArray<FVector> SplinePointScales;
		TArray<ESplinePointType::Type> SplinePointTypes;

		// Save info about the spline points
		for (int32 SplinePointIndex = SplineActor->SplineComponent->GetNumberOfSplinePoints()-1; SplinePointIndex >= 0; --SplinePointIndex)
		{
			FVector SplinePointLocation = SplineActor->SplineComponent->GetLocationAtSplinePoint(SplinePointIndex, ESplineCoordinateSpace::Local);

			// Invert the desired axis
			if (InvertAxis == ESplineInvertAxis::InvertX)
			{
				SplinePointLocation.X = -SplinePointLocation.X;
			}
			else if (InvertAxis == ESplineInvertAxis::InvertY)
			{
				SplinePointLocation.Y = -SplinePointLocation.Y;
			}

			SplinePointLocations.Add(SplinePointLocation);
			FVector SplinePointTangent = SplineActor->SplineComponent->GetTangentAtSplinePoint(SplinePointIndex, ESplineCoordinateSpace::Local);

			// Invert the desired axis
			// NOTE: When inverting the spline X axis, we invert the Y tangent, and vice-versa
			if (InvertAxis == ESplineInvertAxis::InvertX)
			{
				SplinePointTangent.Y = -SplinePointTangent.Y;
			}
			if (InvertAxis == ESplineInvertAxis::InvertY)
			{
				SplinePointTangent.X = -SplinePointTangent.X;
			}
			SplinePointTangents.Add(SplinePointTangent);

			FVector SplinePointScale = SplineActor->SplineComponent->GetScaleAtSplinePoint(SplinePointIndex);
			SplinePointScales.Add(SplinePointScale);
			ESplinePointType::Type SplinePointType = SplineActor->SplineComponent->GetSplinePointType(SplinePointIndex);
			SplinePointTypes.Add(SplinePointType);
		}

		// Clear all the spline points temporarily
		SplineActor->SplineComponent->ClearSplinePoints(false);

		// Add the spline points back in the reverse order (necessary so the triangles are calculated in the right order to display the cap mesh on the top facing upwards)
		for (int32 SplinePointIndex = 0; SplinePointIndex < SplinePointLocations.Num(); ++SplinePointIndex)
		{
			SplineActor->SplineComponent->AddSplinePoint(SplinePointLocations[SplinePointIndex], ESplineCoordinateSpace::Local);
			SplineActor->SplineComponent->SetTangentAtSplinePoint(SplinePointIndex, SplinePointTangents[SplinePointIndex], ESplineCoordinateSpace::Local);
			SplineActor->SplineComponent->SetSplinePointType(SplinePointIndex, SplinePointTypes[SplinePointIndex]);

			// TODO: SetScale functions don't exist? Some information is getting lost without this...
			//SplineActor->SplineComponent->SetScaleAtSplinePoint(SplinePointIndex, SplinePointScales[SplinePointIndex]);
		}
	}

	// Notify of change so any CS is re-run
	// Too slow to run every frame
	SplineActor->PostEditMove(false);

	// Update the viewport
	GEditor->RedrawLevelEditingViewports(true);

	// Rebuild the spline data
	SplineEditWidget->Rebuild2dSplineData(true, false);

	return FReply::Handled();
}

FReply FSplineActorDetailsCustomization::ResetSpline(ASplineActor* SplineActor)
{
	// Scoped transaction for the undo buffer
	const FScopedTransaction Transaction(LOCTEXT("ResetSplineTransaction", "Reset Spline Point"));
	SplineActor->SplineComponent->Modify();
	if (AActor* Owner = SplineActor->SplineComponent->GetOwner())
	{
		Owner->Modify();
	}

	// The default is stored in the blueprint, so we just let the blueprint know to set it to the default with this boolean
	SplineActor->bResetSplineToDefault = true;

	SplineActor->SplineComponent->bSplineHasBeenEdited = true;

	// Notify that the spline has been modified
	// Don't call this because it ends up forcing the spline edit widget to recalculate zoom & offset, so it makes you lose your place if you have zoomed or panned
	//UClass* ActorClass = ASplineActor::StaticClass();
	//UProperty* SplineInfoProperty = FindField<UProperty>(USplineComponent::StaticClass(), GET_MEMBER_NAME_CHECKED(USplineComponent, SplineInfo));
	//FPropertyChangedEvent SplineInfoPropertyChangedEvent(SplineInfoProperty);
	//Self->SplineActor->PostEditChangeProperty(SplineInfoPropertyChangedEvent);

	// Notify of change so any CS is re-run
	// Too slow to run every frame
	SplineActor->PostEditMove(false);

	GEditor->RedrawLevelEditingViewports(true);

	SplineEditWidget->Rebuild2dSplineData(true, true);

	SplineActor->bResetSplineToDefault = false;
	
	return FReply::Handled();
}

////////////////////////////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE

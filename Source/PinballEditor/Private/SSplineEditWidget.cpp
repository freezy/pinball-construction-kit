// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#include "PinballEditor.h"
#include "SSplineEditWidget.h"
#include "SplineActor.h"
#include "Components/SplineComponent.h"
#include "MessageLog.h"
#include "ScopedTransaction.h"


#define LOCTEXT_NAMESPACE "SSplineEditWidget"


namespace SSplineEditWidgetDefs
{
	/** Minimum pixels the user must drag the cursor before a drag+drop starts on a spline point */
	const float MinCursorDistanceForDraggingSplinePoint = 3.0f;
	const float WidgetWidth = 400;
	const float WidgetHeight = 400;
	const FVector2D PointSize = FVector2D(20.0f,20.0f);
	const float ZoomFactorIncrementMultiplier = 2;
	const float MinZoom = 0.01;
	const float MaxZoom = 10;

	/** Don't allow deleting spline points if it would result in a number of spline points lower than this */
	const int32 MinNumSplinePoints = 3;

	// Some customizable settings for hovering over splines
	const float SplineHoverTolerance = 5.0f;// 2.0f;
	const float WireThickness = 5.0f;

	/** Blend color for spline points that are currently selected by the user */
	const FLinearColor& GetSplinePointSelectionColor()
	{
		return GetDefault<UEditorStyleSettings>()->SelectionColor;
	}

	/** Blend color for splines that the user is hovering over with the mouse */
	const FLinearColor& GetSplineHoverColor()
	{
		return GetDefault<UEditorStyleSettings>()->SelectionColor;
	}

	/** Blend color for spline points that are currently NOT selected by the user */
	const FLinearColor SplinePointColor = FColor::White;

	/** Blend color for splines that the user is NOT hovering over with the mouse */
	const FLinearColor SplineColor = FColor::White;
}

// Types of spline point that are useful for pinball spline actors
enum class EPinballSplinePointCurveType
{
	RoundSplinePoint,
	SharpCornerSplinePoint,
};

void SSplineEditWidget::Construct( const FArguments& InArgs )
{
	SplineActor = InArgs._SplineActor.Get();

	// Start with default zoom of 1
	ZoomFactor = FVector2D(1,1);

	Rebuild2dSplineData(true, true);

	// Dummy brush
	BackgroundImage = FEditorStyle::GetBrush("ProgressBar.ThinBackground");

	FString SplinePointImagePath = FPaths::ProjectContentDir() / TEXT("Editor/Slate/Icons/SplinePoint_Normal.png");
	FName SplinePointBrushName = FName(*SplinePointImagePath);
	SplinePointImageBrushPtr = MakeShareable(new FSlateDynamicImageBrush(SplinePointBrushName, SSplineEditWidgetDefs::PointSize));
	SplinePointImageBrush = SplinePointImageBrushPtr.Get();

	FString HoveredSplinePointImagePath = FPaths::ProjectContentDir() / TEXT("Editor/Slate/Icons/SplinePoint_Hover.png");
	FName HoveredSplinePointBrushName = FName(*HoveredSplinePointImagePath);
	HoveredSplinePointImageBrushPtr = MakeShareable(new FSlateDynamicImageBrush(HoveredSplinePointBrushName, SSplineEditWidgetDefs::PointSize));
	HoveredSplinePointImageBrush = HoveredSplinePointImageBrushPtr.Get();

	RelativeDragStartMouseCursorPos = FVector2D::ZeroVector;
	RelativeMouseCursorPos = FVector2D::ZeroVector;

	SplinePointUnderMouse = NULL;
	PointBeingDragged = NULL;
	bStartedDrag = false;
	bPanningWithMouse = false;
	DragPointDistance = 0.0f;
	MousePanDistance = 0.0f;

	// Register to be notified when properties are edited
	FCoreUObjectDelegates::FOnObjectPropertyChanged::FDelegate OnPropertyChangedDelegate = FCoreUObjectDelegates::FOnObjectPropertyChanged::FDelegate::CreateRaw(this, &SSplineEditWidget::OnPropertyChanged);
	OnPropertyChangedHandle = FCoreUObjectDelegates::OnObjectPropertyChanged.Add(OnPropertyChangedDelegate);
}

SSplineEditWidget::~SSplineEditWidget()
{
	// Unregister the property modification handler
	FCoreUObjectDelegates::OnObjectPropertyChanged.Remove(OnPropertyChangedHandle);
}

void SSplineEditWidget::Rebuild2dSplineData(bool bRecalculatePositionOffset, bool bRecalcuateZoomFactor)
{
	if (SplineActor != nullptr)
	{
		int32 SplinePointCount = SplineActor->SplineComponent->GetNumberOfSplinePoints();
		if (SplinePointCount != SplinePoints.Num())
		{
			// Clear the list of selected points if the number of spline points changed
			// Indices might have changed
			ClearSelectedSplinePointIndices();

		}

		SplinePoints.Empty();

		// Should we auto-set the appropriate zoom level?
		if (bRecalcuateZoomFactor)
		{
			// Find min and max points
			float MinX = INT_MAX, MinY = INT_MAX;
			float MaxX = INT_MIN, MaxY = INT_MIN;
			for (int32 SplinePointIndex = 0; SplinePointIndex < SplinePointCount; ++SplinePointIndex)
			{
				FVector SplinePointLocation = SplineActor->SplineComponent->GetLocationAtSplinePoint(SplinePointIndex, ESplineCoordinateSpace::Local);

				MinX = FMath::Min(SplinePointLocation.X, MinX);
				MinY = FMath::Min(SplinePointLocation.Y, MinY);
				MaxX = FMath::Max(SplinePointLocation.X, MaxX);
				MaxY = FMath::Max(SplinePointLocation.Y, MaxY);
			}

			float XSpan = MaxX - MinX;
			float YSpan = MaxY - MinY;

			float RequiredXZoomFactor = 1;
			float RequiredYZoomFactor = 1;

			// If too big in either x or y
			if (XSpan > (SSplineEditWidgetDefs::WidgetWidth * 0.9) || YSpan > (SSplineEditWidgetDefs::WidgetHeight * 0.9f))
			{
				// Calculate how much we need to zoom out for each (zoom factor will be < 1)
				RequiredXZoomFactor = (SSplineEditWidgetDefs::WidgetWidth * 0.9) / (XSpan + SSplineEditWidgetDefs::PointSize.X);
				RequiredYZoomFactor = (SSplineEditWidgetDefs::WidgetHeight * 0.9f) / (YSpan + SSplineEditWidgetDefs::PointSize.Y);
				
				// Keep Zoom Factor the same in X & Y to avoid distortion, set it to the minimum (most zoomed out) of X and Y
				float ResultZoomFactor = FMath::Min(RequiredXZoomFactor, RequiredYZoomFactor);
				ZoomFactor = FVector2D(ResultZoomFactor, ResultZoomFactor);
			}
			// Otherwise if x or y span is smaller than 1/4 of the widget size
			else if (XSpan < SSplineEditWidgetDefs::WidgetWidth / 4 || YSpan < (SSplineEditWidgetDefs::WidgetHeight) / 4)
			{
				// Calculate the zoom necessary to make it 90% of the widget size
				RequiredXZoomFactor = (SSplineEditWidgetDefs::WidgetWidth * 0.9f) / XSpan;
				RequiredYZoomFactor = (SSplineEditWidgetDefs::WidgetHeight * 0.9f) / YSpan;

				// Keep Zoom Factor the same in X & Y to avoid distortion, set it to the minimum (most zoomed out) of X and Y
				float ResultZoomFactor = FMath::Min(RequiredXZoomFactor, RequiredYZoomFactor);
				ZoomFactor = FVector2D(ResultZoomFactor, ResultZoomFactor);
			}

			// Enforce min and max zooms
			ZoomFactor.X = FMath::Max(ZoomFactor.X, SSplineEditWidgetDefs::MinZoom);
			ZoomFactor.Y = FMath::Max(ZoomFactor.Y, SSplineEditWidgetDefs::MinZoom);
			ZoomFactor.X = FMath::Min(ZoomFactor.X, SSplineEditWidgetDefs::MaxZoom);
			ZoomFactor.Y = FMath::Min(ZoomFactor.Y, SSplineEditWidgetDefs::MaxZoom);
		}

		// Position offset depends on zoom, so calculate it after calculating zoom
		if (bRecalculatePositionOffset)
		{
			PositionOffset = FVector2D(0.0f, 0.0f);

			float MinX = INT_MAX, MinY = INT_MAX;
			float MaxX = INT_MIN, MaxY = INT_MIN;

			// Find center point of spline and of the widget itself
			FVector2D SplineActorCenterPoint = FVector2D(0.0f, 0.0f);
			FVector2D WidgetCenterPoint = FVector2D(SSplineEditWidgetDefs::WidgetWidth/2, SSplineEditWidgetDefs::WidgetHeight/2);
			for (int32 SplinePointIndex = 0; SplinePointIndex < SplinePointCount; ++SplinePointIndex)
			{
				FVector SplinePointLocation = SplineActor->SplineComponent->GetLocationAtSplinePoint(SplinePointIndex, ESplineCoordinateSpace::Local);

				MinX = FMath::Min(SplinePointLocation.X * ZoomFactor.X, MinX);
				MinY = FMath::Min(SplinePointLocation.Y * ZoomFactor.Y, MinY);
				MaxX = FMath::Max(SplinePointLocation.X * ZoomFactor.X, MaxX);
				MaxY = FMath::Max(SplinePointLocation.Y * ZoomFactor.Y, MaxY);
			}

			float HalfWidthX = (MaxX - MinX)/2;
			float HalfHeightY = (MaxY - MinY)/2;

			SplineActorCenterPoint.X = MinX + HalfWidthX;
			SplineActorCenterPoint.Y = MinY + HalfHeightY;

			// This is the offset necessary to make the spline actor center point be the same as the widget center point
			PositionOffset = WidgetCenterPoint - SplineActorCenterPoint - (SSplineEditWidgetDefs::PointSize / 2);
		}

		for (int32 SplinePointIndex = 0; SplinePointIndex < SplinePointCount; ++SplinePointIndex)
		{
			FVector SplinePointLocation = SplineActor->SplineComponent->GetLocationAtSplinePoint(SplinePointIndex, ESplineCoordinateSpace::Local);
			FVector SplinePointDirection = SplineActor->SplineComponent->GetTangentAtSplinePoint(SplinePointIndex, ESplineCoordinateSpace::Local);

			// Spline point info represents our spline point in 2D
			FSplinePoint2D NewSplinePoint;
			// Zoom and offset are already reflected in the position stored here.  Then it can always be used as is.  Just have to remember about the offset when modifying
			NewSplinePoint.Position = FVector2D((SplinePointLocation.X * ZoomFactor.X) + PositionOffset.X, (SplinePointLocation.Y * ZoomFactor.Y) + PositionOffset.Y);
			// Zoom already reflected in the position stored here.  Then it can always be used as is.  Just have to remember about the offset when modifying
			NewSplinePoint.Direction = FVector2D(SplinePointDirection.X * ZoomFactor.X, SplinePointDirection.Y * ZoomFactor.Y);
			NewSplinePoint.Index = SplinePointIndex;
			SplinePoints.Add(NewSplinePoint);
		}
	}
}

bool SSplineEditWidget::OnGetCanEditSelectedSplinePointLocationAndTangent() const
{
	if (SelectedSplinePointIndices.Num() > 0)
	{
		return true;
	}

	return false;
}

TOptional<float> SSplineEditWidget::OnGetSelectedSplinePointLocation(EAxis::Type Axis) const
{
	if (SelectedSplinePointIndices.Num() == 0)
	{
		return 0.0f;
	}

	// Some invalid default value
	float NumericVal = -FLT_MAX;

	if (Axis != EAxis::X && Axis != EAxis::Y)
	{
		// Unsupported axis
		return NumericVal;
	}

	// Check everything that is currently selected
	for (int32 SelectedSplinePointIndex : SelectedSplinePointIndices)
	{
		if (SelectedSplinePointIndex >= 0 && SelectedSplinePointIndex < SplineActor->SplineComponent->GetNumberOfSplinePoints())
		{
			FVector SplinePointPos = SplineActor->SplineComponent->GetLocationAtSplinePoint(SelectedSplinePointIndex, ESplineCoordinateSpace::Local);

			float CurrentSplinePointValue = -FLT_MAX;

			switch (Axis)
			{
			case EAxis::X:
				CurrentSplinePointValue = SplinePointPos.X;
				break;
			case EAxis::Y:
				CurrentSplinePointValue = SplinePointPos.Y;
				break;
			default:
				break;
			}

			if (NumericVal == -FLT_MAX)
			{
				// First time this is set
				NumericVal = CurrentSplinePointValue;
			}
			// Has been set before, and doesn't match
			else if (NumericVal != CurrentSplinePointValue)
			{
				// Return an unset value so it displays the "multiple values" indicator instead
				return TOptional<float>();
			}
			else
			{
				// Multiple items selected, but they have the same value
			}
		}
	}

	// Return an unset value so it displays the "multiple values" indicator instead
	return NumericVal;
}

void SSplineEditWidget::OnSelectedSplinePointLocationChanged(float NewValue, EAxis::Type Axis)
{
	if (Axis != EAxis::X && Axis != EAxis::Y)
	{
		// Unsupported axis
		return;
	}

	if (SelectedSplinePointIndices.Num() > 0 && SplineActor != nullptr && SplineActor->SplineComponent != nullptr)
	{
		// Scoped transaction for the undo buffer
		const FScopedTransaction Transaction(LOCTEXT("SetSplinePointPosition", "Set Spline Point(s) Position"));

		SplineActor->SplineComponent->Modify();
		if (AActor* Owner = SplineActor->SplineComponent->GetOwner())
		{
			Owner->Modify();
		}

		// Move everything that is currently selected
		for (int32 SelectedSplinePointIndex : SelectedSplinePointIndices)
		{
			if (SelectedSplinePointIndex >= 0 && SelectedSplinePointIndex < SplineActor->SplineComponent->GetNumberOfSplinePoints())
			{
				FVector SplinePointPos = SplineActor->SplineComponent->GetLocationAtSplinePoint(SelectedSplinePointIndex, ESplineCoordinateSpace::Local);

				switch (Axis)
				{
				case EAxis::X:
					SplinePointPos.X = NewValue;
					break;
				case EAxis::Y:
					SplinePointPos.Y = NewValue;
					break;
				default:
					break;
				}

				// Position stored in the spline point is post offset and zoom, so undo these before storing the value in the spline point
				SplineActor->SplineComponent->SetLocationAtSplinePoint(SelectedSplinePointIndex, SplinePointPos, ESplineCoordinateSpace::Local);
				SplineActor->SplineComponent->bSplineHasBeenEdited = true;
			}
		}

		// Notify of change so any ConstructionScript is re-run
		SplineActor->PostEditMove(false);

		GEditor->RedrawLevelEditingViewports(true);

		Rebuild2dSplineData(false, false);
	}
}


void SSplineEditWidget::OnSelectedSplinePointLocationCommitted(float InNewValue, ETextCommit::Type CommitType, EAxis::Type Axis)
{
	OnSelectedSplinePointLocationChanged(InNewValue, Axis);
}

TOptional<float> SSplineEditWidget::OnGetSelectedSplinePointTangent(EAxis::Type Axis) const
{
	if (SelectedSplinePointIndices.Num() == 0)
	{
		return 0.0f;
	}

	// Some invalid default value
	float NumericVal = -FLT_MAX;

	if (Axis != EAxis::X && Axis != EAxis::Y)
	{
		// Unsupported axis
		return NumericVal;
	}

	// Check tangent for everything that is currently selected
	for (int32 SelectedSplinePointIndex : SelectedSplinePointIndices)
	{
		if (SelectedSplinePointIndex >= 0 && SelectedSplinePointIndex < SplineActor->SplineComponent->GetNumberOfSplinePoints())
		{
			FVector SplinePointTangent = SplineActor->SplineComponent->GetTangentAtSplinePoint(SelectedSplinePointIndex, ESplineCoordinateSpace::Local);

			float CurrentSplinePointValue = -FLT_MAX;

			switch (Axis)
			{
			case EAxis::X:
				CurrentSplinePointValue = SplinePointTangent.X;
				break;
			case EAxis::Y:
				CurrentSplinePointValue = SplinePointTangent.Y;
				break;
			default:
				break;
			}

			if (NumericVal == -FLT_MAX)
			{
				// First time this is set
				NumericVal = CurrentSplinePointValue;
			}
			// Has been set before, and doesn't match
			else if (NumericVal != CurrentSplinePointValue)
			{
				// Return an unset value so it displays the "multiple values" indicator instead
				return TOptional<float>();
			}
			else
			{
				// Multiple items selected, but they have the same value
			}
		}
	}

	// Return an unset value so it displays the "multiple values" indicator instead
	return NumericVal;
}

void SSplineEditWidget::OnSelectedSplinePointTangentChanged(float NewValue, EAxis::Type Axis)
{
	if (Axis != EAxis::X && Axis != EAxis::Y)
	{
		// Unsupported axis
		return;
	}

	if (SelectedSplinePointIndices.Num() > 0 && SplineActor != nullptr && SplineActor->SplineComponent != nullptr)
	{
		// Scoped transaction for the undo buffer
		const FScopedTransaction Transaction(LOCTEXT("SetSplinePointTangent", "Set Spline Point(s) Tangent"));

		SplineActor->SplineComponent->Modify();
		if (AActor* Owner = SplineActor->SplineComponent->GetOwner())
		{
			Owner->Modify();
		}

		// Move everything that is currently selected
		for (int32 SelectedSplinePointIndex : SelectedSplinePointIndices)
		{
			if (SelectedSplinePointIndex >= 0 && SelectedSplinePointIndex < SplineActor->SplineComponent->GetNumberOfSplinePoints())
			{
				FVector SplinePointTangent = SplineActor->SplineComponent->GetTangentAtSplinePoint(SelectedSplinePointIndex, ESplineCoordinateSpace::Local);

				switch (Axis)
				{
				case EAxis::X:
					SplinePointTangent.X = NewValue;
					break;
				case EAxis::Y:
					SplinePointTangent.Y = NewValue;
					break;
				default:
					break;
				}

				// Position stored in the spline point is post offset and zoom, so undo these before storing the value in the spline point
				SplineActor->SplineComponent->SetTangentAtSplinePoint(SelectedSplinePointIndex, SplinePointTangent, ESplineCoordinateSpace::Local);
				SplineActor->SplineComponent->bSplineHasBeenEdited = true;
			}
		}

		// Notify of change so any ConstructionScript is re-run
		SplineActor->PostEditMove(false);

		GEditor->RedrawLevelEditingViewports(true);

		Rebuild2dSplineData(false, false);
	}
}

void SSplineEditWidget::OnSelectedSplinePointTangentCommitted(float InNewValue, ETextCommit::Type CommitType, EAxis::Type Axis)
{
	OnSelectedSplinePointTangentChanged(InNewValue, Axis);
}

void SSplineEditWidget::OnPropertyChanged(UObject* ObjectBeingModified, FPropertyChangedEvent& PropertyChangedEvent)
{
	if (SplineActor != nullptr && SplineActor->SplineComponent != nullptr)
	{
		// Only care about changes to our spline actor and it's spline component
		if (ObjectBeingModified == SplineActor || ObjectBeingModified == SplineActor->SplineComponent)
		{
			// Don't rebuild if we are currently dragging
			if (PointBeingDragged == nullptr)
			{
				// If we are hovering over a spline point after the OnPropertyChanged event, we must have just finished dragging in the widget
				bool bJustFinishedWidgetDrag = !(SplinePointUnderMouse == nullptr);
				Rebuild2dSplineData(!bJustFinishedWidgetDrag, !bJustFinishedWidgetDrag);
			}
		}
	}
}



int32 SSplineEditWidget::OnPaint( const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& WidgetClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const
{
	OutDrawElements.PushClip(FSlateClippingZone(AllottedGeometry));

	// Draw background border layer
	{
		const FSlateBrush* ThisBackgroundImage = BackgroundImage.Get();
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(),
			ThisBackgroundImage,
			ESlateDrawEffect::None,
			InWidgetStyle.GetColorAndOpacityTint() * ThisBackgroundImage->TintColor.GetColor( InWidgetStyle )
			);
	}

	// Draw spline segments (lines)
	for (int32 SegmentIndex = 0; SplinePoints.Num() > 1 && SegmentIndex < SplinePoints.Num(); ++SegmentIndex)
	{
		const FVector2D DragStartCursorPos = RelativeDragStartMouseCursorPos;

		int32 StartSplinePointIndex = SegmentIndex - 1;
		int32 EndSplinePointIndex = SegmentIndex;

		// loop back to the beginning if this is a closed loop
		if (StartSplinePointIndex < 0)
		{
			if (SplineActor && SplineActor->SplineComponent && SplineActor->SplineComponent->IsClosedLoop())
			{
				StartSplinePointIndex = SplinePoints.Num() - 1;
			}
			else
			{
				continue;
			}
		}

		const FVector2D SplineStart = SplinePoints[StartSplinePointIndex].Position + (SSplineEditWidgetDefs::PointSize * 0.5);
		const FVector2D SplineEnd = SplinePoints[EndSplinePointIndex].Position + (SSplineEditWidgetDefs::PointSize * 0.5);
		const FVector2D SplineStartDir = SplinePoints[StartSplinePointIndex].Direction;
		const FVector2D SplineEndDir = SplinePoints[EndSplinePointIndex].Direction;

		// Check to see which spline we are currently hovering over
		FSplinePoint2D* HoverSplineStart = nullptr;
		FSplinePoint2D* HoverSplineEnd = nullptr;
		SplineOverlapResult.GetPoints(HoverSplineStart, HoverSplineEnd);
		bool bHoveringOverSpline = &SplinePoints[StartSplinePointIndex] == HoverSplineStart;

		// Draw two passes, the first one is an drop shadow
		for (auto SplineLayerIndex = 0; SplineLayerIndex < 2; ++SplineLayerIndex)
		{
			++LayerId;

			const float ShadowOpacity = 0.5f;
			const float SplineThickness = (SplineLayerIndex == 0) ? 5.0f : 4.0f;
			const auto ShadowOffset = (SplineLayerIndex == 0) ? FVector2D(-1.0f, 1.0f) : FVector2D::ZeroVector;

			// If we are currently hovering over this spline, draw it in a different color
			const auto ColorToUse = bHoveringOverSpline ? SSplineEditWidgetDefs::GetSplineHoverColor() : SSplineEditWidgetDefs::SplineColor;

			// Separate color for spline shadow
			const auto SplineColorScale = (SplineLayerIndex == 0) ? FLinearColor(0.0f, 0.0f, 0.0f, ShadowOpacity) : ColorToUse;

			FSlateDrawElement::MakeSpline(
				OutDrawElements,
				LayerId,
				AllottedGeometry.ToPaintGeometry(ShadowOffset, FVector2D(1.0, 1.0f)),
				SplineStart,
				SplineStartDir,
				SplineEnd,
				SplineEndDir,
				SplineThickness,
				ESlateDrawEffect::None,
				InWidgetStyle.GetColorAndOpacityTint() * FColor::White * SplineColorScale);
		}
	}

	// Debug code to draw the bounding box of the spline(s) we are currently hovering over
#if SPLINE_EDIT_WIDGET_DEBUG_SPLINE_HOVER
#define DrawSpaceLine(Point1, Point2, DebugWireColor) {const FVector2D FakeTangent = (Point2 - Point1).GetSafeNormal(); FSlateDrawElement::MakeSpline(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(FVector2D::ZeroVector, FVector2D(1.0, 1.0f)), Point1, FakeTangent, Point2, FakeTangent, WidgetClippingRect, 1.0f, ESlateDrawEffect::None, DebugWireColor); }

	++LayerId;
	for (DebugSplinesLine DebugLine : DebugSplineLines)
	{
		DrawSpaceLine(DebugLine.Start, DebugLine.End, DebugLine.Color);
	}

	DebugSplineLines.Empty();

#endif
	
	++LayerId;

	for (auto SplinePointIndex = 0; SplinePointIndex < SplinePoints.Num(); ++SplinePointIndex)
	{
		const FSplinePoint2D* CurrentSplinePoint = &SplinePoints[SplinePointIndex];
		const bool bIsMouseOverPoint = (SplinePointUnderMouse != NULL && SplinePointUnderMouse == CurrentSplinePoint) || (PointBeingDragged != nullptr && PointBeingDragged == CurrentSplinePoint);

		const auto SplinePointPaintGeometry = AllottedGeometry.ToPaintGeometry(CurrentSplinePoint->Position, SSplineEditWidgetDefs::PointSize);

		/** Different blend color for spline points that are currently selected */
		FLinearColor BlendColor = SelectedSplinePointIndices.Contains(CurrentSplinePoint->Index) ? SSplineEditWidgetDefs::GetSplinePointSelectionColor() : SSplineEditWidgetDefs::SplinePointColor;
		FLinearColor DrawColor = InWidgetStyle.GetColorAndOpacityTint() * BlendColor;
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId,
			SplinePointPaintGeometry,
			bIsMouseOverPoint ? HoveredSplinePointImageBrush.Get() : SplinePointImageBrush.Get(),
			ESlateDrawEffect::None,
			DrawColor
			);
	}
	OutDrawElements.PopClip();

	return LayerId;
}



FVector2D SSplineEditWidget::ComputeDesiredSize(float LayoutScaleMultiplier) const
{
	// no desired size -- their size is always determined by their container
	return LayoutScaleMultiplier * FVector2D(SSplineEditWidgetDefs::WidgetWidth, SSplineEditWidgetDefs::WidgetHeight);
}


FReply SSplineEditWidget::OnMouseMove( const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent )
{
	// Update window-relative cursor position
	RelativeMouseCursorPos = InMyGeometry.AbsoluteToLocal( InMouseEvent.GetScreenSpacePosition() );

	// Check which spline point is under the mouse (can be null)
	SplinePointUnderMouse =  FindSplinePointUnderCursor(InMyGeometry, InMouseEvent.GetScreenSpacePosition());

	// Are we currently dragging to pan with the mouse?
	if (bPanningWithMouse)
	{
		MousePanDistance += InMouseEvent.GetCursorDelta().Size();
		if (MousePanDistance >= SSplineEditWidgetDefs::MinCursorDistanceForDraggingSplinePoint)
		{
			// Update our position offset and rebuild the 2D view
			PositionOffset += InMouseEvent.GetCursorDelta();
			Rebuild2dSplineData(false, false);
		}
	}

	// Dragging a spline point
	if (PointBeingDragged != nullptr && SplineActor != nullptr && SplineActor->SplineComponent != nullptr)
	{
		DragPointDistance += InMouseEvent.GetCursorDelta().Size();

		// Only start a drag if the distance moved is over the minimum
		if (DragPointDistance >= SSplineEditWidgetDefs::MinCursorDistanceForDraggingSplinePoint)
		{
			// First time the distance moved is over the minimum, we start a drag and a new undo transaction
			if (!bStartedDrag)
			{
				bStartedDrag = true;
				if (GEditor && GEditor->Trans && !GEditor->bIsSimulatingInEditor)
				{
					// For Undo
					FText SessionName = LOCTEXT("SplineEditWidgetMoveSplinePoint", "Move Spline Point");
					FSlateApplication::Get().OnLogSlateEvent(EEventLog::BeginTransaction, SessionName);
					GEditor->BeginTransaction(TEXT(""), SessionName, Cast<UObject>(SplineActor));
					SplineActor->SplineComponent->Modify();
					if (AActor* Owner = SplineActor->SplineComponent->GetOwner())
					{
						Owner->Modify();
					}
				}

				// If there is a current selection, can use shift or control to add to the selection (but not remove if we are starting a drag)
				if (InMouseEvent.IsShiftDown() || InMouseEvent.IsControlDown())
				{
					if (!SelectedSplinePointIndices.Contains(PointBeingDragged->Index))
					{
						AddToSelectedSplinePointIndices(PointBeingDragged->Index);
					}
				}
				// If not holding shift/ctrl clear selection and add this to selection
				else
				{
					if (!SelectedSplinePointIndices.Contains(PointBeingDragged->Index))
					{
						ClearSelectedSplinePointIndices();
						AddToSelectedSplinePointIndices(PointBeingDragged->Index);
					}
					// Otherwise, if the spline point we are dragging is already part of the selection, drag the whole selection
				}
			}

			// Move everything that is currently selected
			for (int32 SelectedSplinePointIndex : SelectedSplinePointIndices)
			{
				if (SelectedSplinePointIndex >= 0 && SelectedSplinePointIndex < SplineActor->SplineComponent->GetNumberOfSplinePoints())
				{
					FSplinePoint2D* CurrentSplinePoint = nullptr;

					// Index in the array of SplinePoint2Ds doesn't necessarily match the index in the SplineComponent
					for (int32 SplinePointIndex = 0; SplinePointIndex < SplinePoints.Num(); ++SplinePointIndex)
					{
						CurrentSplinePoint = &SplinePoints[SplinePointIndex];
						if (SelectedSplinePointIndex == CurrentSplinePoint->Index)
						{
							break;
						}
					}
					
					if (CurrentSplinePoint != nullptr)
					{
						CurrentSplinePoint->Position += InMouseEvent.GetCursorDelta();

						UClass* ActorClass = ASplineActor::StaticClass();
						UProperty* Property = FindField<UProperty>(ActorClass, "SplineComponent");
						FVector SplinePointPos = SplineActor->SplineComponent->GetLocationAtSplinePoint(SelectedSplinePointIndex, ESplineCoordinateSpace::Local);

						// Position stored in the spline point is post offset and zoom, so undo these before storing the value in the spline point
						SplineActor->SplineComponent->SetLocationAtSplinePoint(SelectedSplinePointIndex, FVector((CurrentSplinePoint->Position.X - PositionOffset.X) / ZoomFactor.X, (CurrentSplinePoint->Position.Y - PositionOffset.Y) / ZoomFactor.Y, SplinePointPos.Z), ESplineCoordinateSpace::Local);

						SplineActor->SplineComponent->bSplineHasBeenEdited = true;

						// Notify that the spline has been modified
						FPropertyChangedEvent PropertyChangedEvent(Property);
						SplineActor->SplineComponent->PostEditChangeProperty(PropertyChangedEvent);

						// Don't call PostEditMove here, too slow to re-run the ConstructionScript every frame
						//SplineActor->PostEditMove(false);
					}
					
				}
			}
			GEditor->RedrawLevelEditingViewports(true);
		}
	}

	// See if mouse is hovering over spline
	// Borrowed logic from DrawConnection() function in Connection Drawing Policy of the Graph Editor module.
	// This is the same basic logic that the blueprint editor (and other graph editors) uses to detect if the mouse is hovering over a connection between graph nodes
	{
		// Reset since we are checking spline overlap again
		SplineOverlapResult = FSplineOverlapResult();

		for (int32 SegmentIndex = 0; SplinePoints.Num() > 1 && SegmentIndex < SplinePoints.Num(); ++SegmentIndex)
		{
			const FVector2D DragStartCursorPos = RelativeDragStartMouseCursorPos;

			int32 StartSplinePointIndex = SegmentIndex - 1;
			int32 EndSplinePointIndex = SegmentIndex;

			// loop back to the beginning if this is a closed loop
			if (StartSplinePointIndex < 0)
			{
				if (SplineActor && SplineActor->SplineComponent && SplineActor->SplineComponent->IsClosedLoop())
				{
					StartSplinePointIndex = SplinePoints.Num() - 1;
				}
				else
				{
					continue;
				}
			}

			// This segment's start and end position and direction (spline tangent)
			const FVector2D SplineStart = SplinePoints[StartSplinePointIndex].Position + (SSplineEditWidgetDefs::PointSize * 0.5);
			const FVector2D SplineEnd = SplinePoints[EndSplinePointIndex].Position + (SSplineEditWidgetDefs::PointSize * 0.5);
			const FVector2D SplineStartDir = SplinePoints[StartSplinePointIndex].Direction;
			const FVector2D SplineEndDir = SplinePoints[EndSplinePointIndex].Direction;

			// Distance to consider as an overlap
			const float QueryDistanceTriggerThresholdSquared = FMath::Square(SSplineEditWidgetDefs::SplineHoverTolerance + SSplineEditWidgetDefs::WireThickness * 0.5f);

			// Distance to pass the bounding box cull test
			const float QueryDistanceToBoundingBoxSquared = QueryDistanceTriggerThresholdSquared;

			bool bCloseToSpline = false;
			{
				// The curve will include the endpoints but can extend out of a tight bounds because of the tangents
				// P0Tangent coefficient maximizes to 4/27 at a=1/3, and P1Tangent minimizes to -4/27 at a=2/3.
				//const float MaximumTangentContribution = 4.0f / 27.0f;
				// Wasn't quite big enough for some splines, so just made it 50% bigger for now
				const float MaximumTangentContribution = 1.5 * (4.0f / 27.0f);

				FBox2D Bounds(ForceInit);

				Bounds += FVector2D(SplineStart);
				Bounds += FVector2D(SplineStart + MaximumTangentContribution * SplineStartDir);
				Bounds += FVector2D(SplineEnd);
				Bounds += FVector2D(SplineEnd - MaximumTangentContribution * SplineEndDir);

				bCloseToSpline = Bounds.ComputeSquaredDistanceToPoint(RelativeMouseCursorPos) < QueryDistanceToBoundingBoxSquared;

				// Draw the bounding box for debugging
	#if SPLINE_EDIT_WIDGET_DEBUG_SPLINE_HOVER

				if (bCloseToSpline)
				{
					const FLinearColor BoundsWireColor = bCloseToSpline ? FLinearColor::Green : FLinearColor::White;

					FVector2D TL = Bounds.Min;
					FVector2D BR = Bounds.Max;
					FVector2D TR = FVector2D(Bounds.Max.X, Bounds.Min.Y);
					FVector2D BL = FVector2D(Bounds.Min.X, Bounds.Max.Y);

					DebugSplineLines.Add(DebugSplinesLine(TL, TR, BoundsWireColor));
					DebugSplineLines.Add(DebugSplinesLine(TR, BR, BoundsWireColor));
					DebugSplineLines.Add(DebugSplinesLine(BR, BL, BoundsWireColor));
					DebugSplineLines.Add(DebugSplinesLine(BL, TL, BoundsWireColor));
				}
	#endif
			}

			if (bCloseToSpline)
			{
				// Find the closest approach to the spline
				FVector2D ClosestPoint(ForceInit);
				float ClosestDistanceSquared = FLT_MAX;

				const int32 NumStepsToTest = 16;
				const float StepInterval = 1.0f / (float)NumStepsToTest;
				FVector2D Point1 = FMath::CubicInterp(SplineStart, SplineStartDir, SplineEnd, SplineEndDir, 0.0f);
				for (float TestAlpha = 0.0f; TestAlpha < 1.0f; TestAlpha += StepInterval)
				{
					const FVector2D Point2 = FMath::CubicInterp(SplineStart, SplineStartDir, SplineEnd, SplineEndDir, TestAlpha + StepInterval);

					const FVector2D ClosestPointToSegment = FMath::ClosestPointOnSegment2D(RelativeMouseCursorPos, Point1, Point2);
					const float DistanceSquared = (RelativeMouseCursorPos - ClosestPointToSegment).SizeSquared();

					if (DistanceSquared < ClosestDistanceSquared)
					{
						ClosestDistanceSquared = DistanceSquared;
						ClosestPoint = ClosestPointToSegment;
					}

					Point1 = Point2;
				}

				// Record the overlap
				if (ClosestDistanceSquared < QueryDistanceTriggerThresholdSquared)
				{
					if (ClosestDistanceSquared < SplineOverlapResult.GetDistanceSquared())
					{
						FSplinePoint2D& StartPoint = SplinePoints[StartSplinePointIndex];
						FSplinePoint2D& EndPoint = SplinePoints[EndSplinePointIndex];

						SplineOverlapResult = FSplineOverlapResult(&StartPoint, &EndPoint, ClosestDistanceSquared, ClosestPoint);
					}
				}
				}

			if (SplineOverlapResult.IsValid())
			{
				// Only allow spline overlaps when there is no point under the cursor 
				if (SplinePointUnderMouse != nullptr)
				{
					SplineOverlapResult = FSplineOverlapResult();
				}
			}
		}
	}

	return FReply::Handled();
}

FReply SSplineEditWidget::OnMouseButtonDown( const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent )
{
	// Figure out what was clicked on
	FSplinePoint2D* SplinePointUnderCursor = FindSplinePointUnderCursor(InMyGeometry, InMouseEvent.GetScreenSpacePosition());
		
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		if (SplinePointUnderCursor != NULL)
		{
			// Left Mouse Button was pressed on a spline point, start drag
			PointBeingDragged = SplinePointUnderCursor;
			DragPointDistance = 0.0f;
			RelativeDragStartMouseCursorPos = InMyGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
		}
	}
	// Right mouse button pressed, start panning with mouse
	// (if the user right clicks and releases with out moving enough to trigger a drag, we show a context menu instead)
	else if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		bPanningWithMouse = true;
	}

	return FReply::Handled().CaptureMouse(AsShared());;
}


FReply SSplineEditWidget::OnMouseButtonUp( const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent )
{
	FSplinePoint2D* SplinePointUnderCursor = FindSplinePointUnderCursor(InMyGeometry, InMouseEvent.GetScreenSpacePosition());

	// Left mouse button release
	if( InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton )
	{
		FSplinePoint2D* DroppedPoint = PointBeingDragged;
		PointBeingDragged = nullptr;

		if (DroppedPoint != nullptr)
		{
			if (DragPointDistance >= SSplineEditWidgetDefs::MinCursorDistanceForDraggingSplinePoint)
			{
				bStartedDrag = false;

				// When we capture the mouse (to prevent this widget from losing focus if you drag outside the bounds of the widget),
				// The capture mouse mode is high precision, and the amount the cursor moves no longer lines up with the point being dragged.
				// As a solution for this, we hide the mouse cursor via OnCursorQuery() when you are dragging, and move it to the correct position after you 
				// release the drag.
				FVector2D AbsolutePos = InMyGeometry.LocalToAbsolute(DroppedPoint->Position + (SSplineEditWidgetDefs::PointSize / 2));
				FSlateApplication::Get().SetCursorPos(AbsolutePos);

				SplinePointUnderMouse = DroppedPoint;

				UClass* ActorClass = ASplineActor::StaticClass();
				UProperty* SplineInfoProperty = FindField<UProperty>(USplineComponent::StaticClass(), GET_MEMBER_NAME_CHECKED(USplineComponent, SplineCurves));
				SplineActor->SplineComponent->bSplineHasBeenEdited = true;

				// Notify that the spline has been modified
				FPropertyChangedEvent SplineInfoPropertyChangedEvent(SplineInfoProperty);
				SplineActor->SplineComponent->PostEditChangeProperty(SplineInfoPropertyChangedEvent);

				// Notify of change so any CS is re-run
				SplineActor->PostEditMove(false);

				// Finish the transaction we started earlier when we detected a drag
				GEditor->EndTransaction();
				
				// Force the level editing viewport to update
				GEditor->RedrawLevelEditingViewports(true);
			}
			else
			{
				if (SplinePointUnderCursor != nullptr)
				{
					// If there is a current selection, can use shift or control to add/remove from the selection
					if (InMouseEvent.IsShiftDown() || InMouseEvent.IsControlDown())
					{
						if (!SelectedSplinePointIndices.Contains(SplinePointUnderCursor->Index))
						{
							AddToSelectedSplinePointIndices(SplinePointUnderCursor->Index);
						}
						else
						{
							RemoveFromSelectedSplinePointIndices(SplinePointUnderCursor->Index);

						}
					}
					// If not holding shift/ctrl, clear selection and add this to selection
					else
					{

						ClearSelectedSplinePointIndices();
						AddToSelectedSplinePointIndices(SplinePointUnderCursor->Index);
					}
				}
			}
		}
		else
		{
			// if we weren't dragging anything, clear the selection on mouse up
			ClearSelectedSplinePointIndices();
		}

		DragPointDistance = 0.0f;

		if (GEditor && GEditor->Trans && !GEditor->bIsSimulatingInEditor && GEditor->Trans->IsActive())
		{
			FSlateApplication::Get().OnLogSlateEvent(EEventLog::EndTransaction);
			GEditor->EndTransaction();
		}
	}
	else if (bPanningWithMouse && MousePanDistance >= SSplineEditWidgetDefs::MinCursorDistanceForDraggingSplinePoint)
	{
		bPanningWithMouse = false;
		MousePanDistance = 0.0f;
	}
	else if( InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton )
	{
		if (SplinePointUnderCursor != nullptr)
		{
			// If there is a current selection, use shift or control to add to the selection
			if (InMouseEvent.IsShiftDown() || InMouseEvent.IsControlDown())
			{
				if (!SelectedSplinePointIndices.Contains(SplinePointUnderCursor->Index))
				{
					AddToSelectedSplinePointIndices(SplinePointUnderCursor->Index);
				}
				else
				{
					 // Don't want to remove in this case
				}
			}
			// If not holding shift/ctrl...
			else
			{
				// ...If this is not in the selection already, it becomes the only selection
				if (!SelectedSplinePointIndices.Contains(SplinePointUnderCursor->Index))
				{
					ClearSelectedSplinePointIndices();
					AddToSelectedSplinePointIndices(SplinePointUnderCursor->Index);
				}
				// ...Otherwise, we keep the current selection
			}
		}

		// Show the right click contextual menu
		FWidgetPath WidgetPath = InMouseEvent.GetEventPath() != nullptr ? *InMouseEvent.GetEventPath() : FWidgetPath();
		ShowOptionsMenuAt( InMouseEvent.GetScreenSpacePosition(), WidgetPath );

		bPanningWithMouse = false;
	}

	PointBeingDragged = nullptr;

	return FReply::Handled().ReleaseMouseCapture();
}

FReply SSplineEditWidget::OnMouseWheel( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	// Multiply or divide the zoom factor by the increment based on wheel direction
	if (MouseEvent.GetWheelDelta() > 0)
	{
		ZoomFactor *= SSplineEditWidgetDefs::ZoomFactorIncrementMultiplier;
	}
	else if (MouseEvent.GetWheelDelta() < 0)
	{
		ZoomFactor /= SSplineEditWidgetDefs::ZoomFactorIncrementMultiplier;
	}

	// Enforce min and max zooms
	ZoomFactor.X = FMath::Max(ZoomFactor.X, SSplineEditWidgetDefs::MinZoom);
	ZoomFactor.Y = FMath::Max(ZoomFactor.Y, SSplineEditWidgetDefs::MinZoom);
	ZoomFactor.X = FMath::Min(ZoomFactor.X, SSplineEditWidgetDefs::MaxZoom);
	ZoomFactor.Y = FMath::Min(ZoomFactor.Y, SSplineEditWidgetDefs::MaxZoom);

	Rebuild2dSplineData(false, false);

	return FReply::Handled();
}

FCursorReply SSplineEditWidget::OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const
{
	FCursorReply CursorReply = FCursorReply::Cursor(EMouseCursor::Default);
	if (PointBeingDragged != nullptr)
	{
		CursorReply = FCursorReply::Cursor(EMouseCursor::None);
	}
	return CursorReply;
}

FSplinePoint2D* SSplineEditWidget::FindSplinePointUnderCursor(const FGeometry& MyGeometry, const FVector2D& ScreenSpaceCursorPosition)
{
	const FVector2D LocalCursorPosition = MyGeometry.AbsoluteToLocal( ScreenSpaceCursorPosition );

	for (FSplinePoint2D& SplinePoint : SplinePoints)
	{
		const FSlateRect SplinePointRect = FSlateRect(SplinePoint.Position, SplinePoint.Position + SSplineEditWidgetDefs::PointSize);
		if (SplinePointRect.ContainsPoint(LocalCursorPosition))
		{
			return &SplinePoint;
		}
	}

	return NULL;
}

void SSplineEditWidget::ShowOptionsMenuAt(const FVector2D& ScreenSpacePosition, const FWidgetPath& WidgetPath)
{
	// Local struct that houses all the functions that are called from the right click menu
	// Maybe these should be separated out into SSplineEditWidget member functions, struct has gotten pretty large...
	// A lot of them have similar logic of, preparing a transaction, performing some modification, ending the transaction and updating everything
	// Maybe part of these can be generalized out into a separate function in the future
	struct Local
	{
		// Make an advanced menu of options for changing the spline type for this spline point
		static void MakeAdvancedSplineTypeMenu(FMenuBuilder& MenuBuilder, int32 SplinePointIndex, SSplineEditWidget* Self)
		{
			int32 MatchingIndex = INDEX_NONE;
			for (int CurrentIndex = 0; CurrentIndex < Self->SplinePoints.Num(); ++CurrentIndex)
			{
				FSplinePoint2D& SplinePoint = Self->SplinePoints[CurrentIndex];
				if (SplinePoint.Index == SplinePointIndex)
				{
					MatchingIndex = CurrentIndex;
				}
			}

			if (MatchingIndex == INDEX_NONE)
			{
				return;
			}

			FSplinePoint2D& SplinePoint = Self->SplinePoints[MatchingIndex];

			TArray<FString> CurveModeNames;

			// TODO: Why no support for CurveBreak in ESplinePointType?
			///** A curve like CIM_Curve, but the arrive and leave tangents are not forced to be the same, so you can create a 'corner' at this key. */
			//CurveModeNames.Add("CIM_CurveBreak");

			///** A straight line between two keypoint values. 
			//maps to EInterpCurveMode CIM_Linear */
			CurveModeNames.Add("Linear");

			///** A cubic-hermite curve between two keypoints, using Arrive/Leave tangents. These tangents will be automatically
			//updated when points are moved, etc.  Tangents are unclamped and will plateau at curve start and end points. 
			//maps to EInterpCurveMode CIM_CurveAuto */
			CurveModeNames.Add("Curve");

			///** The out value is held constant until the next key, then will jump to that value. 
			//maps to EInterpCurveMode CIM_Constant */
			CurveModeNames.Add("Constant");

			///** A cubic-hermite curve between two keypoints, using Arrive/Leave tangents. These tangents will be automatically
			//updated when points are moved, etc.  Tangents are clamped and will plateau at curve start and end points. 
			//maps to EInterpCurveMode CIM_CurveAutoClamped*/
			CurveModeNames.Add("CurveClamped");

			///** A smooth curve just like CIM_Curve, but tangents are not automatically updated so you can have manual control over them (eg. in Curve Editor). 
			//maps to EInterpCurveMode CIM_CurveUser*/
			CurveModeNames.Add("CurveCustomTangent");

			for (int SplineTypeIndex = 0; SplineTypeIndex <= (int32)ESplinePointType::CurveCustomTangent; ++SplineTypeIndex)
			{
				ESplinePointType::Type SplineType = (ESplinePointType::Type)SplineTypeIndex;

					MenuBuilder.AddMenuEntry(
						FText::FromString(CurveModeNames[SplineTypeIndex]),
						FText(),	// No tooltip (intentional)
						FSlateIcon(),	// Icon
						FUIAction(
						FExecuteAction::CreateStatic(&Local::EditSplinePointCurveType_Execute, SplinePointIndex, SplineType, Self),
						FCanExecuteAction(),
						FIsActionChecked()),
						NAME_None,	// Extension point
						EUserInterfaceActionType::Button );
			}
		}	

		// Perform the action of changing a spline point's curve type
		static void EditSplinePointCurveType_Execute(int32 SplinePointIndex, ESplinePointType::Type SplineType, SSplineEditWidget* Self)
		{
			if (Self->SplineActor != nullptr && Self->SplineActor->SplineComponent != nullptr)
			{
				if (Self->SelectedSplinePointIndices.Num() > 0)
				{
					// Scoped transaction for the undo buffer
					const FScopedTransaction Transaction(LOCTEXT("EditSplinePointType", "Change Spline Point(s) Type"));
					Self->SplineActor->SplineComponent->Modify();
					if (AActor* Owner = Self->SplineActor->SplineComponent->GetOwner())
					{
						Owner->Modify();
					}

					for (int32 SelectedSplinePointIndex : Self->SelectedSplinePointIndices)
					{
						if (SelectedSplinePointIndex > 0 || SelectedSplinePointIndex < Self->SplineActor->SplineComponent->GetNumberOfSplinePoints())
						{
							// Set the spline point type
							Self->SplineActor->SplineComponent->SetSplinePointType(SelectedSplinePointIndex, SplineType);
						}
					}

					Self->SplineActor->SplineComponent->bSplineHasBeenEdited = true;

					// Don't call PostEditChangeProperty here because it ends up forcing the spline edit widget to recalculate zoom & offset, so it makes you lose your place if you have zoomed or panned

					// Notify of change so any CS is re-run
					Self->SplineActor->PostEditMove(false);

					GEditor->RedrawLevelEditingViewports(true);

					Self->Rebuild2dSplineData(false, false);
				}
			}
		}

		// Perform the action of changing a spline point's curve type to round
		static void RoundPoint_Execute(int32 SplinePointIndex,  SSplineEditWidget* Self)
		{
			if (Self->SplineActor != nullptr && Self->SplineActor->SplineComponent != nullptr)
			{
				if (Self->SelectedSplinePointIndices.Num() > 0)
				{
					// Scoped transaction for the undo buffer
					const FScopedTransaction Transaction(LOCTEXT("EditSplinePointType", "Make Spline Point(s) Round"));
					Self->SplineActor->SplineComponent->Modify();
					if (AActor* Owner = Self->SplineActor->SplineComponent->GetOwner())
					{
						Owner->Modify();
					}

					for (int32 SelectedSplinePointIndex : Self->SelectedSplinePointIndices)
					{
						if (SelectedSplinePointIndex > 0 || SelectedSplinePointIndex < Self->SplineActor->SplineComponent->GetNumberOfSplinePoints())
						{
							// Set the spline point type
							Self->SplineActor->SplineComponent->SetSplinePointType(SelectedSplinePointIndex, ESplinePointType::Curve);
						}
					}

					Self->SplineActor->SplineComponent->bSplineHasBeenEdited = true;

					// Don't call PostEditChangeProperty here because it ends up forcing the spline edit widget to recalculate zoom & offset, so it makes you lose your place if you have zoomed or panned

					// Notify of change so any CS is re-run
					Self->SplineActor->PostEditMove(false);

					GEditor->RedrawLevelEditingViewports(true);

					Self->Rebuild2dSplineData(false, false);
				}
			}
		}

		// Perform the action of changing a spline point's curve type to sharp corner
		static void SharpPoint_Execute(int32 SplinePointIndex, SSplineEditWidget* Self)
		{
			if (Self->SplineActor != nullptr && Self->SplineActor->SplineComponent != nullptr)
			{
				if (Self->SelectedSplinePointIndices.Num() > 0)
				{
					// Scoped transaction for the undo buffer
					const FScopedTransaction Transaction(LOCTEXT("EditSplinePointType", "Make Spline Point(s) Sharp Corners"));
					Self->SplineActor->SplineComponent->Modify();
					if (AActor* Owner = Self->SplineActor->SplineComponent->GetOwner())
					{
						Owner->Modify();
					}

					for (int32 SelectedSplinePointIndex : Self->SelectedSplinePointIndices)
					{
						if (SelectedSplinePointIndex > 0 || SelectedSplinePointIndex < Self->SplineActor->SplineComponent->GetNumberOfSplinePoints())
						{
							FVector PreviousTangent = Self->SplineActor->SplineComponent->GetTangentAtSplinePoint(SelectedSplinePointIndex, ESplineCoordinateSpace::Local);
							// Set the spline point type
							Self->SplineActor->SplineComponent->SetSplinePointType(SelectedSplinePointIndex, ESplinePointType::CurveClamped);
							// Set the right tangent for a sharp corner point
							FVector NewTangent = PreviousTangent;
							NewTangent.X = NewTangent.Y = 0.0001f;	// UE-19183, tangents of 0 leave holes in a mesh. Make it just above 0 for now
							Self->SplineActor->SplineComponent->SetTangentAtSplinePoint(SelectedSplinePointIndex, NewTangent, ESplineCoordinateSpace::Local);
						}
					}

					Self->SplineActor->SplineComponent->bSplineHasBeenEdited = true;

					// Don't call PostEditChangeProperty here because it ends up forcing the spline edit widget to recalculate zoom & offset, so it makes you lose your place if you have zoomed or panned

					// Notify of change so any CS is re-run
					Self->SplineActor->PostEditMove(false);
					GEditor->RedrawLevelEditingViewports(true);
					Self->Rebuild2dSplineData(false, false);
				}
			}
		}

		// Perform the action of deleting a spline point
		static void DeleteSplinePoint_Execute(int32 SplinePointIndex, SSplineEditWidget* Self)
		{
			// Find the matching point
			int32 MatchingIndex = INDEX_NONE;
			for (int CurrentIndex = 0; CurrentIndex < Self->SplinePoints.Num(); ++CurrentIndex)
			{
				FSplinePoint2D& SplinePoint = Self->SplinePoints[CurrentIndex];
				if (SplinePoint.Index == SplinePointIndex)
				{
					MatchingIndex = CurrentIndex;
				}
			}

			if (MatchingIndex == INDEX_NONE)
			{
				return;
			}

			// Scoped transaction for the undo buffer
			const FScopedTransaction Transaction(LOCTEXT("DeleteSplinePoint", "Delete Spline Point"));

			Self->SplineActor->SplineComponent->Modify();
			if (AActor* Owner = Self->SplineActor->SplineComponent->GetOwner())
			{
				Owner->Modify();
			}

			// Delete the spline point
			Self->SplineActor->SplineComponent->RemoveSplinePoint(MatchingIndex);
			Self->SplineActor->SplineComponent->bSplineHasBeenEdited = true;

			// Don't call PostEditChangeProperty here because it ends up forcing the spline edit widget to recalculate zoom & offset, so it makes you lose your place if you have zoomed or panned

			// Notify of change so any CS is re-run
			Self->SplineActor->PostEditMove(false);
			GEditor->RedrawLevelEditingViewports(true);
			Self->Rebuild2dSplineData(false, false);
		}

		/** Perform the action of adding a new spline point
		*	@param InsertIndex Where in the array of spline points to insert
		*	@param NewSplinePointLocation The X and Y location of where to put this new spline point
		*	@param PointCurveType Which type of curve we want for the new spline
		*	@param Self Reference to this SplineEditWidget
		*
		*/
		static void InsertSplinePoint_Execute(int32 InsertIndex, FVector2D NewSplinePointLocation, EPinballSplinePointCurveType PointCurveType, SSplineEditWidget* Self)
		{
			// Find the matching point
			int32 MatchingIndex = INDEX_NONE;
			for (int CurrentIndex = 0; CurrentIndex < Self->SplinePoints.Num(); ++CurrentIndex)
			{
				FSplinePoint2D& SplinePoint = Self->SplinePoints[CurrentIndex];
				if (SplinePoint.Index == InsertIndex)
				{
					MatchingIndex = CurrentIndex;
				}
			}

			if (MatchingIndex == INDEX_NONE)
			{
				return;
			}

			FSplinePoint2D& PointToEdit = Self->SplinePoints[MatchingIndex];
			FVector PreviousPointPosition = Self->SplineActor->SplineComponent->GetLocationAtSplinePoint(PointToEdit.Index, ESplineCoordinateSpace::Local);

			// Position stored in the spline point is post offset and zoom, so undo these before storing the value in the spline point
			FVector NewSplinePointLocation3D;
			NewSplinePointLocation3D.X = ((NewSplinePointLocation.X - Self->PositionOffset.X) / Self->ZoomFactor.X) - ((SSplineEditWidgetDefs::PointSize.X / 2) / Self->ZoomFactor.X);
			NewSplinePointLocation3D.Y = ((NewSplinePointLocation.Y - Self->PositionOffset.Y) / Self->ZoomFactor.Y) - ((SSplineEditWidgetDefs::PointSize.Y / 2) / Self->ZoomFactor.Y);
			NewSplinePointLocation3D.Z = PreviousPointPosition.Z;

			// Scoped transaction for the undo buffer
			const FScopedTransaction Transaction(LOCTEXT("InsertSplinePoint", "Insert Spline Point"));

			Self->SplineActor->SplineComponent->Modify();
			if (AActor* Owner = Self->SplineActor->SplineComponent->GetOwner())
			{
				Owner->Modify();
			}

			// Add New point
			int32 IndexToInsert = PointToEdit.Index + 1;
			// Insert or add at end?
			if (IndexToInsert < Self->SplineActor->SplineComponent->GetNumberOfSplinePoints())
			{
				
				Self->SplineActor->SplineComponent->AddSplinePointAtIndex(NewSplinePointLocation3D, IndexToInsert, ESplineCoordinateSpace::Local);
			}
			else
			{
				Self->SplineActor->SplineComponent->AddSplinePoint(NewSplinePointLocation3D, ESplineCoordinateSpace::Local);
			}

			// Set to the desired curve type
			switch (PointCurveType)
			{
			case EPinballSplinePointCurveType::RoundSplinePoint:
				Self->SplineActor->SplineComponent->SetSplinePointType(IndexToInsert, ESplinePointType::Curve);
				break;
			case EPinballSplinePointCurveType::SharpCornerSplinePoint:
			{
				FVector PreviousTangent = Self->SplineActor->SplineComponent->GetTangentAtSplinePoint(IndexToInsert, ESplineCoordinateSpace::Local);
				Self->SplineActor->SplineComponent->SetSplinePointType(IndexToInsert, ESplinePointType::CurveClamped);
				FVector NewTangent = PreviousTangent;
				NewTangent.X = NewTangent.Y = 0.0001f;	// UE-19183, tangents of 0 leave holes in a mesh. Make it just above 0 for now
				Self->SplineActor->SplineComponent->SetTangentAtSplinePoint(IndexToInsert, NewTangent, ESplineCoordinateSpace::Local);
				break;
			}
			default:
				break;
			}
			
			Self->SplineActor->SplineComponent->bSplineHasBeenEdited = true;

			// Don't call PostEditChangeProperty here because it ends up forcing the spline edit widget to recalculate zoom & offset, so it makes you lose your place if you have zoomed or panned

			// Notify of change so any CS is re-run
			Self->SplineActor->PostEditMove(false);
			GEditor->RedrawLevelEditingViewports(true);
			Self->Rebuild2dSplineData(false, false);
		}
	};

	const bool bShouldCloseMenuAfterSelection = true;
	FMenuBuilder OptionsMenuBuilder( bShouldCloseMenuAfterSelection, NULL );

	FSplinePoint2D* SplineStartPoint = nullptr;
	FSplinePoint2D* SplineEndPoint = nullptr;
	if (SplineOverlapResult.IsValid())
	{
		SplineOverlapResult.GetPoints(SplineStartPoint, SplineEndPoint);
	}

	// Did we right click on something?
	if (SplinePointUnderMouse != NULL || // Right click on point
		(SplineStartPoint != nullptr && SplineEndPoint != nullptr))	// Right click on spline
	{
		// Right clicked on a point
		if (SplinePointUnderMouse != NULL)
		{
			// Spline Point editing options
			OptionsMenuBuilder.BeginSection(NAME_None, LOCTEXT("EditSplinePointAttributesSection", "Edit Spline Point"));
			{
				OptionsMenuBuilder.AddMenuEntry(
					LOCTEXT("RoundSplinePoint", "Make Round"),
					LOCTEXT("RoundSplinePointTooltip", "Make this spline point round."),
					FSlateIcon(),	// Icon
					FUIAction(
					FExecuteAction::CreateStatic(&Local::RoundPoint_Execute, SplinePointUnderMouse->Index, this),
					FCanExecuteAction(),
					FIsActionChecked()),
					NAME_None,	// Extension point
					EUserInterfaceActionType::Button);

				OptionsMenuBuilder.AddMenuEntry(
					LOCTEXT("SharpSplinePoint", "Make Sharp Corner"),
					LOCTEXT("SharpSplinePointTooltip", "Make this spline point round."),
					FSlateIcon(),	// Icon
					FUIAction(
					FExecuteAction::CreateStatic(&Local::SharpPoint_Execute, SplinePointUnderMouse->Index, this),
					FCanExecuteAction(),
					FIsActionChecked()),
					NAME_None,	// Extension point
					EUserInterfaceActionType::Button);

				// Advanced curve type sub menu
				OptionsMenuBuilder.AddSubMenu(
					LOCTEXT("AdancedCurveType", "Advanced Curve Type"),
					LOCTEXT("AdvancedCurveType_ToolTip", "Advanced curve type settings of this point"),
					FNewMenuDelegate::CreateStatic(&Local::MakeAdvancedSplineTypeMenu, SplinePointUnderMouse->Index, this));

				// Don't allow deleting spline points if it would result in less than MinNumSplinePoints
				if (SplineActor && SplineActor->SplineComponent && SplineActor->SplineComponent->GetNumberOfSplinePoints() > SSplineEditWidgetDefs::MinNumSplinePoints)
				{
					OptionsMenuBuilder.AddMenuEntry(
						LOCTEXT("RemoveSplinePoint", "Remove Spline Point"),
						LOCTEXT("RemoveSplinePointTooltip", "Remove this spline point from the spline."),
						FSlateIcon(),	// Icon
						FUIAction(
						FExecuteAction::CreateStatic(&Local::DeleteSplinePoint_Execute, SplinePointUnderMouse->Index, this),
						FCanExecuteAction(),
						FIsActionChecked()),
						NAME_None,	// Extension point
						EUserInterfaceActionType::Button);
				}
			}
		}
		// Right clicked on Spline
		else
		{
			// Spline point creation options
			OptionsMenuBuilder.BeginSection(NAME_None, LOCTEXT("CreateNewSplinePointSection", "New Spline Point"));
			{
				OptionsMenuBuilder.AddMenuEntry(
					LOCTEXT("AddRoundSplinePointHere", "Add Round Spline Point Here"),
					LOCTEXT("AddRoundSplinePointHereTooltip", "Create a round spline point at the location closest to the mouse on the spline point."),
					FSlateIcon(),	// Icon
					FUIAction(
					FExecuteAction::CreateStatic(&Local::InsertSplinePoint_Execute, SplineStartPoint->Index, SplineOverlapResult.GetClosestPoint(), EPinballSplinePointCurveType::RoundSplinePoint, this),
					FCanExecuteAction(),
					FIsActionChecked()),
					NAME_None,	// Extension point
					EUserInterfaceActionType::Button);
				OptionsMenuBuilder.AddMenuEntry(
					LOCTEXT("AddSharpSplinePointHere", "Add Sharp Corner Spline Point Here"),
					LOCTEXT("AddSharpSplinePointHereTooltip", "Create a sharp corner spline point at the location closest to the mouse on the spline point."),
					FSlateIcon(),	// Icon
					FUIAction(
					FExecuteAction::CreateStatic(&Local::InsertSplinePoint_Execute, SplineStartPoint->Index, SplineOverlapResult.GetClosestPoint(), EPinballSplinePointCurveType::SharpCornerSplinePoint, this),
					FCanExecuteAction(),
					FIsActionChecked()),
					NAME_None,	// Extension point
					EUserInterfaceActionType::Button);
			}
			OptionsMenuBuilder.EndSection();
		}
		
		TSharedRef< SWidget > WindowContent =
			SNew(SBorder)
			[
				OptionsMenuBuilder.MakeWidget()
			];

		const bool bFocusImmediately = true;
		FSlateApplication::Get().PushMenu(AsShared(), WidgetPath, WindowContent, ScreenSpacePosition, FPopupTransitionEffect::ContextMenu, bFocusImmediately);
	}
	else
	{
		// If we right clicked on nothing, clear selection
		ClearSelectedSplinePointIndices();
	}
	
}

//////////////////////////////////////////////////////////////////////////
// FGraphSplineOverlapResult

bool FSplineOverlapResult::GetPoints(FSplinePoint2D*& OutPoint1, FSplinePoint2D*& OutPoint2) const
{
	if (SplinePoint1 != nullptr || SplinePoint2 != nullptr)
	{
		OutPoint1 = SplinePoint1;
		OutPoint2 = SplinePoint2;

		return true;
	}

	return false;
}

#undef LOCTEXT_NAMESPACE
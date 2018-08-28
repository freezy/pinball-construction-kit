// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SCompoundWidget.h"
struct FSlateBrush;
class ASplineActor;

// Draw bounding box of the spline we are currently hovering over
#define SPLINE_EDIT_WIDGET_DEBUG_SPLINE_HOVER 0

/**
* 2D (X&Y) Spline point info for a point in the spline
*/
struct FSplinePoint2D
{
	FSplinePoint2D()
		: Position(0.0f, 0.0f)
		, Direction(0.0f, 0.0f)
		, Index(INDEX_NONE)
	{}

	/** Position for this spline point */
	FVector2D Position;

	/** Direction (tangent) for this spline point */
	FVector2D Direction;

	/** The index of this point in the SplineComponent */
	int32 Index;
};


/** Results for mouse overlapping spline */
struct FSplineOverlapResult
{
public:
	FSplineOverlapResult()
		: SplinePoint1(nullptr)
		, SplinePoint2(nullptr)
		, DistanceSquared(FLT_MAX)
		, ClosesetPoint(FVector2D(0,0))
	{
	}

	FSplineOverlapResult(FSplinePoint2D* InPin1, FSplinePoint2D* InPin2, float InDistanceSquared, FVector2D InClosesetPoint)
		: SplinePoint1(InPin1)
		, SplinePoint2(InPin2)
		, DistanceSquared(InDistanceSquared)
		, ClosesetPoint(InClosesetPoint)
	{
	}

	bool IsValid() const
	{
		return DistanceSquared < FLT_MAX;
	}

	float GetDistanceSquared() const
	{
		return DistanceSquared;
	}

	bool GetPoints(FSplinePoint2D*& OutPin1, FSplinePoint2D*& OutPin2) const;

	FVector2D GetClosestPoint()
	{
		return ClosesetPoint;
	}

protected:
	FSplinePoint2D* SplinePoint1;
	FSplinePoint2D* SplinePoint2;
	float DistanceSquared;
	FVector2D ClosesetPoint;
};

/**
 * Widget for editing a SplineComponent in 2D
 */
class SSplineEditWidget : public SCompoundWidget
{

public:
	
	SLATE_BEGIN_ARGS( SSplineEditWidget )
		: _SplineActor(nullptr)
	{}

		/** Pointer to the Spline Actor in question */
		SLATE_ATTRIBUTE( ASplineActor*, SplineActor )

	SLATE_END_ARGS()

	/**
	 * Construct the widget
	 *
	 * @param	InArgs				A declaration from which to construct the widget
	 */
	void Construct( const FArguments& InArgs );
	virtual ~SSplineEditWidget();

	/** SWidget overrides */
	virtual int32 OnPaint( const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const override;
	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override;
	virtual FReply OnMouseButtonDown( const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent ) override;
	virtual FReply OnMouseButtonUp( const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent ) override;
	virtual FCursorReply OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const override;
	virtual FReply OnMouseMove(const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	/** Manipulate the Selected Spline Point indices */
	void AddToSelectedSplinePointIndices(int32 IndexToAdd)
	{
		SelectedSplinePointIndices.Add(IndexToAdd);
	}
	void RemoveFromSelectedSplinePointIndices(int32 IndexToRemove)
	{
		SelectedSplinePointIndices.Remove(IndexToRemove);
	}
	void ClearSelectedSplinePointIndices()
	{
		SelectedSplinePointIndices.Empty();
	}

	/*
	*	Delegate that is called when the properties of an object are changed
	*	Used to notify the Spline Edit Widget if properties of the Spline are changed in the 3D viewport
	*/
	void OnPropertyChanged(UObject* ObjectBeingModified, FPropertyChangedEvent& PropertyChangedEvent);

	/** 
	* Rebuild representation of the spline 
	* Called when the Spline component has been changed and we need to update the 2D representation to match
	*/
	void Rebuild2dSplineData(bool bRecalculatePositionOffset, bool bRecalculateZoomFactor);

	/** Whether or not we can adjust the location and rotation of the currently selected spline points */
	bool OnGetCanEditSelectedSplinePointLocationAndTangent() const;

	/** Used to display and set the location of the selected spline points */
	TOptional<float> OnGetSelectedSplinePointLocation(EAxis::Type Axis) const;
	void OnSelectedSplinePointLocationChanged(float NewValue, EAxis::Type Axis);
	void OnSelectedSplinePointLocationCommitted(float InNewValue, ETextCommit::Type CommitType, EAxis::Type Axis);
	/** Used to display and set the tangent of the selected spline points */
	TOptional<float> OnGetSelectedSplinePointTangent(EAxis::Type Axis) const;
	void OnSelectedSplinePointTangentChanged(float NewValue, EAxis::Type Axis);
	void OnSelectedSplinePointTangentCommitted(float InNewValue, ETextCommit::Type CommitType, EAxis::Type Axis);

protected:

	/** Finds the spline point that's under the cursor */
	struct FSplinePoint2D* FindSplinePointUnderCursor(const FGeometry& MyGeometry, const FVector2D& ScreenSpaceCursorPosition);

	/** Handle to our delegate for notifications when properties get changed (need to hold on to unregister it) */
	FDelegateHandle OnPropertyChangedHandle;

private:

	ASplineActor* SplineActor;

	/** Displays a context menu at the specified location with options for modifying the spline actor's spline */
	void ShowOptionsMenuAt(const FVector2D& ScreenSpacePosition, const FWidgetPath& WidgetPath);

	 // Visual 

	/** Background image to use for the background canvas area */
	TAttribute< const FSlateBrush* > BackgroundImage;

	/** Background to use for each spline point */
	TAttribute< const FSlateBrush* > SplinePointImageBrush;
	TSharedPtr<FSlateDynamicImageBrush> SplinePointImageBrushPtr;

	/** Background to use for spline point that the mouse is hovering over */
	TAttribute< const FSlateBrush* > HoveredSplinePointImageBrush;
	TSharedPtr<FSlateDynamicImageBrush> HoveredSplinePointImageBrushPtr;

	/** Border Padding */
	TAttribute<FVector2D> BorderPadding;


	/** Array of Spline Points with 2D positions */
	TArray<struct FSplinePoint2D> SplinePoints;

	/** This is the position offset to make sure the spline points are centered, also used for panning with the mouse */
	FVector2D PositionOffset;

	/** This is the zoom factor to make sure the spline can be seen at a reasonable size, can be changed with mouse wheel */
	FVector2D ZoomFactor;

	// Mouse Interaction variables

	/** The SplinePoint that the mouse was over last */
	FSplinePoint2D* SplinePointUnderMouse;

	/** Indices of spline points that the user has currently selected */
	TArray<int32> SelectedSplinePointIndices;

	/** Point that we're currently dragging around.  This points to an entry in the SplinePoints array! */
	FSplinePoint2D* PointBeingDragged;

	/** Whether or not the user has dragged a spline point far enough to start a drag transaction*/
	bool bStartedDrag;

	/** Whether the user is panning with the mouse or not */
	bool bPanningWithMouse;

	/** Distance we've RMB+dragged the cursor.  Used to determine if we're panning the view */
	float MousePanDistance;

	/** Distance we've LMB+dragged on a point.  Used to determine if we're ready to start dragging and dropping */
	float DragPointDistance;

	/** Position of the mouse cursor relative to the widget, where the user picked up a dragged spline point */
	FVector2D RelativeDragStartMouseCursorPos;

	/** Position of the mouse cursor relative to the widget, the last time it moved.  Used for drag and drop. */
	FVector2D RelativeMouseCursorPos;



	/** The result of checking what spline the mouse is overlapping */
	FSplineOverlapResult SplineOverlapResult;

// Draw the bounding box of the spline we are currently hovering over, for debugging
#if SPLINE_EDIT_WIDGET_DEBUG_SPLINE_HOVER
	struct DebugSplinesLine
	{
		DebugSplinesLine(FVector2D InStart, FVector2D InEnd, FLinearColor InColor) : Start(InStart), End(InEnd), Color(InColor) {}

		FVector2D Start;
		FVector2D End;
		FLinearColor Color;
	};
	mutable TArray<DebugSplinesLine> DebugSplineLines;
#endif
};

#pragma once

#if WITH_EDITOR

#include "CoreMinimal.h"
#include "Input/DragAndDrop.h"
#include "GameplayTagContainer.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/SBoxPanel.h"
#include "Styling/AppStyle.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Framework/Application/SlateApplication.h"

/** Helper to get a pill-shaped brush (cached static) */
inline const FSlateBrush* GetPillBrush()
{
	static FSlateRoundedBoxBrush Brush(FLinearColor::White, 8.0f);
	return &Brush;
}

/**
 * Drag-drop operation that carries a GameplayTag name.
 */
class FMaterialTagDragDropOp : public FDragDropOperation
{
public:
	DRAG_DROP_OPERATOR_TYPE(FMaterialTagDragDropOp, FDragDropOperation)

	FString TagName;
	FString SlotHint;

	static TSharedRef<FMaterialTagDragDropOp> New(const FString& InTagName, const FString& InSlotHint)
	{
		TSharedRef<FMaterialTagDragDropOp> Op = MakeShareable(new FMaterialTagDragDropOp());
		Op->TagName = InTagName;
		Op->SlotHint = InSlotHint;
		Op->Construct();
		return Op;
	}

	virtual TSharedPtr<SWidget> GetDefaultDecorator() const override
	{
		return SNew(SBorder)
			.BorderImage(GetPillBrush())
			.BorderBackgroundColor(FLinearColor(0.25f, 0.25f, 0.28f, 0.95f))
			.Padding(FMargin(10, 4))
			[
				SNew(STextBlock)
				.Text(FText::FromString(TagName))
				.ColorAndOpacity(FLinearColor::White)
			];
	}
};

/**
 * A draggable tag pill widget (pill-shaped, UE-native style).
 */
class STagPill : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(STagPill)
		: _TagName()
		, _SlotHint()
	{}
		SLATE_ARGUMENT(FString, TagName)
		SLATE_ARGUMENT(FString, SlotHint)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		TagName = InArgs._TagName;
		SlotHint = InArgs._SlotHint;

		ChildSlot
		[
			SNew(SBorder)
			.BorderImage(GetPillBrush())
			.BorderBackgroundColor(FLinearColor(0.22f, 0.22f, 0.25f, 1.0f))
			.Padding(FMargin(10, 4))
			.ToolTipText(FText::FromString(FString::Printf(TEXT("Slots: %s\nDrag onto a material slot entry"), *SlotHint)))
			[
				SNew(STextBlock)
				.Text(FText::FromString(TagName))
				.ColorAndOpacity(FLinearColor(0.85f, 0.85f, 0.85f))
			]
		];

		SetCursor(EMouseCursor::GrabHand);
	}

	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
		{
			return FReply::Handled().DetectDrag(SharedThis(this), EKeys::LeftMouseButton);
		}
		return FReply::Unhandled();
	}

	virtual FReply OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		return FReply::Handled().BeginDragDrop(FMaterialTagDragDropOp::New(TagName, SlotHint));
	}

private:
	FString TagName;
	FString SlotHint;
};

/**
 * A static (non-draggable) tag pill with an X button to remove it.
 */
class SRemovableTagPill : public SCompoundWidget
{
public:
	DECLARE_DELEGATE_OneParam(FOnRemoveTag, const FString& /*TagName*/);

	SLATE_BEGIN_ARGS(SRemovableTagPill)
		: _TagName()
	{}
		SLATE_ARGUMENT(FString, TagName)
		SLATE_EVENT(FOnRemoveTag, OnRemove)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		StoredTagName = InArgs._TagName;
		OnRemove = InArgs._OnRemove;

		ChildSlot
		[
			SNew(SBorder)
			.BorderImage(GetPillBrush())
			.BorderBackgroundColor(FLinearColor(0.18f, 0.18f, 0.20f, 1.0f))
			.Padding(FMargin(8, 2, 4, 2))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(0, 0, 4, 0)
				[
					SNew(STextBlock)
					.Text(FText::FromString(StoredTagName))
					.ColorAndOpacity(FLinearColor(0.85f, 0.85f, 0.85f))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(SButton)
					.ButtonStyle(FCoreStyle::Get(), "NoBorder")
					.ContentPadding(FMargin(2, 0))
					.OnClicked(this, &SRemovableTagPill::HandleRemoveClicked)
					.ToolTipText(FText::FromString(TEXT("Remove tag")))
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("x")))
						.ColorAndOpacity(FLinearColor(0.6f, 0.6f, 0.6f))
					]
				]
			]
		];
	}

private:
	FReply HandleRemoveClicked()
	{
		OnRemove.ExecuteIfBound(StoredTagName);
		return FReply::Handled();
	}

	FString StoredTagName;
	FOnRemoveTag OnRemove;
};

/**
 * A drop target wrapper widget. Accepts FMaterialTagDragDropOp drops.
 */
class STagDropTarget : public SCompoundWidget
{
public:
	DECLARE_DELEGATE_OneParam(FOnTagDropped, const FString& /*TagName*/);

	SLATE_BEGIN_ARGS(STagDropTarget) {}
		SLATE_DEFAULT_SLOT(FArguments, Content)
		SLATE_EVENT(FOnTagDropped, OnTagDropped)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		OnTagDropped = InArgs._OnTagDropped;

		ChildSlot
		[
			InArgs._Content.Widget
		];

		// Visible and hit-testable so drag-drop events reach us
		SetVisibility(EVisibility::Visible);
	}

	virtual bool OnIsActiveDropTarget(TSharedPtr<FDragDropOperation> DragDropOperation) const
	{
		return DragDropOperation.IsValid() && DragDropOperation->IsOfType<FMaterialTagDragDropOp>();
	}

	virtual FReply OnDragOver(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override
	{
		TSharedPtr<FMaterialTagDragDropOp> TagOp = DragDropEvent.GetOperationAs<FMaterialTagDragDropOp>();
		if (TagOp.IsValid())
		{
			// Auto-scroll nearest parent SScrollBox when dragging near edges
			AutoScrollOnDrag(MyGeometry, DragDropEvent);
			return FReply::Handled();
		}
		return FReply::Unhandled();
	}

	virtual FReply OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override
	{
		TSharedPtr<FMaterialTagDragDropOp> TagOp = DragDropEvent.GetOperationAs<FMaterialTagDragDropOp>();
		if (TagOp.IsValid() && !TagOp->TagName.IsEmpty())
		{
			OnTagDropped.ExecuteIfBound(TagOp->TagName);
			return FReply::Handled();
		}
		return FReply::Unhandled();
	}

	virtual void OnDragEnter(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override
	{
		TSharedPtr<FMaterialTagDragDropOp> TagOp = DragDropEvent.GetOperationAs<FMaterialTagDragDropOp>();
		if (TagOp.IsValid())
		{
			bIsDragOver = true;
		}
	}

	virtual void OnDragLeave(const FDragDropEvent& DragDropEvent) override
	{
		bIsDragOver = false;
	}

private:
	void AutoScrollOnDrag(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
	{
		// Find nearest parent SScrollBox by walking up the widget tree
		if (!CachedScrollBox.IsValid())
		{
			TSharedPtr<SWidget> Current = GetParentWidget();
			while (Current.IsValid())
			{
				TSharedPtr<SScrollBox> AsScrollBox = StaticCastSharedPtr<SScrollBox>(Current);
				if (Current->GetType() == FName(TEXT("SScrollBox")))
				{
					CachedScrollBox = StaticCastSharedPtr<SScrollBox>(Current);
					break;
				}
				Current = Current->GetParentWidget();
			}
		}

		TSharedPtr<SScrollBox> ScrollBox = CachedScrollBox.Pin();
		if (!ScrollBox.IsValid()) return;

		// Get the scroll box geometry and cursor position in its local space
		const FGeometry& ScrollGeo = ScrollBox->GetCachedGeometry();
		FVector2D LocalPos = ScrollGeo.AbsoluteToLocal(DragDropEvent.GetScreenSpacePosition());
		float ScrollHeight = ScrollGeo.GetLocalSize().Y;

		const float EdgeZone = 40.0f;
		const float ScrollSpeed = 15.0f;

		if (LocalPos.Y < EdgeZone)
		{
			float Factor = 1.0f - (LocalPos.Y / EdgeZone);
			ScrollBox->SetScrollOffset(ScrollBox->GetScrollOffset() - ScrollSpeed * Factor);
		}
		else if (LocalPos.Y > ScrollHeight - EdgeZone)
		{
			float Factor = 1.0f - ((ScrollHeight - LocalPos.Y) / EdgeZone);
			ScrollBox->SetScrollOffset(ScrollBox->GetScrollOffset() + ScrollSpeed * Factor);
		}
	}

	FOnTagDropped OnTagDropped;
	TWeakPtr<SScrollBox> CachedScrollBox;
	bool bIsDragOver = false;
};

#endif // WITH_EDITOR

#include "UI/InteractionPromptWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

UInteractionPromptWidget::UInteractionPromptWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UInteractionPromptWidget::NativeConstruct()
{
    Super::NativeConstruct();
    BuildWidgetTree();
    SetPromptVisible(false);
}

void UInteractionPromptWidget::SetPromptText(const FText& InPromptText)
{
    BuildWidgetTree();
    if (PromptText)
    {
        PromptText->SetText(InPromptText);
    }
}

void UInteractionPromptWidget::SetPromptVisible(bool bVisible)
{
    BuildWidgetTree();

    if (PromptBorder)
    {
        PromptBorder->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
    }
}

void UInteractionPromptWidget::BuildWidgetTree()
{
    if (!WidgetTree || WidgetTree->RootWidget)
    {
        return;
    }

    UCanvasPanel* CanvasRoot = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("CanvasRoot"));
    WidgetTree->RootWidget = CanvasRoot;

    RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RootBox"));
    if (UCanvasPanelSlot* RootSlot = CanvasRoot->AddChildToCanvas(RootBox))
    {
        RootSlot->SetAnchors(FAnchors(0.5f, 0.5f));
        RootSlot->SetAlignment(FVector2D(0.5f, 0.5f));
        RootSlot->SetAutoSize(true);
        RootSlot->SetPosition(FVector2D::ZeroVector);
    }

    ReticleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ReticleText"));
    ReticleText->SetText(FText::FromString(TEXT("+")));
    ReticleText->SetJustification(ETextJustify::Center);
    ReticleText->SetFont(FSlateFontInfo(TEXT("/Engine/EngineFonts/Roboto"), 30));
    ReticleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.97f, 0.92f, 1.0f)));
    ReticleText->SetShadowOffset(FVector2D(1.0f, 1.0f));
    ReticleText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.55f));
    if (UVerticalBoxSlot* ReticleSlot = RootBox->AddChildToVerticalBox(ReticleText))
    {
        ReticleSlot->SetHorizontalAlignment(HAlign_Center);
        ReticleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
    }

    PromptBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PromptBorder"));
    PromptBorder->SetBrushColor(FLinearColor(0.03f, 0.05f, 0.04f, 0.84f));
    PromptBorder->SetPadding(FMargin(12.0f, 7.0f));
    if (UVerticalBoxSlot* PromptSlot = RootBox->AddChildToVerticalBox(PromptBorder))
    {
        PromptSlot->SetHorizontalAlignment(HAlign_Center);
    }

    PromptText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PromptText"));
    PromptText->SetText(FText::GetEmpty());
    PromptText->SetJustification(ETextJustify::Center);
    PromptText->SetFont(FSlateFontInfo(TEXT("/Engine/EngineFonts/Roboto"), 16));
    PromptText->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.97f, 0.92f, 1.0f)));
    PromptText->SetShadowOffset(FVector2D(1.0f, 1.0f));
    PromptText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.45f));
    PromptBorder->SetContent(PromptText);

    SetVisibility(ESlateVisibility::HitTestInvisible);
}

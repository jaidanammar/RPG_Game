#include "UI/EnemyStatusBarWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

void UEnemyStatusBarWidget::NativeConstruct()
{
    Super::NativeConstruct();
    BuildWidgetTree();
}

UEnemyStatusBarWidget::UEnemyStatusBarWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UEnemyStatusBarWidget::SetHealthValues(float CurrentHealth, float MaxHealth)
{
    BuildWidgetTree();

    const float SafeMaxHealth = FMath::Max(1.0f, MaxHealth);
    const float Percent = FMath::Clamp(CurrentHealth / SafeMaxHealth, 0.0f, 1.0f);

    if (HealthBar)
    {
        HealthBar->SetPercent(Percent);
    }
}

void UEnemyStatusBarWidget::SetFocused(bool bInFocused)
{
    BuildWidgetTree();

    if (FocusDiamond)
    {
        FocusDiamond->SetVisibility(bInFocused ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
    }

    if (HealthBar)
    {
        HealthBar->SetFillColorAndOpacity(bInFocused
            ? FLinearColor(0.72f, 0.16f, 0.10f, 1.0f)
            : FLinearColor(0.52f, 0.08f, 0.07f, 0.95f));
    }
}

void UEnemyStatusBarWidget::BuildWidgetTree()
{
    if (!WidgetTree || WidgetTree->RootWidget)
    {
        return;
    }

    UVerticalBox* RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RootBox"));
    WidgetTree->RootWidget = RootBox;

    USizeBox* DiamondSlotBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("DiamondSlotBox"));
    DiamondSlotBox->SetWidthOverride(12.0f);
    DiamondSlotBox->SetHeightOverride(12.0f);
    if (UVerticalBoxSlot* DiamondSlot = RootBox->AddChildToVerticalBox(DiamondSlotBox))
    {
        DiamondSlot->SetHorizontalAlignment(HAlign_Center);
        DiamondSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 3.0f));
    }

    FocusDiamond = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("FocusDiamond"));
    FocusDiamond->SetBrushColor(FLinearColor(0.88f, 0.76f, 0.44f, 0.96f));
    FocusDiamond->SetVisibility(ESlateVisibility::Collapsed);
    FocusDiamond->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
    FocusDiamond->SetRenderTransform(FWidgetTransform(FVector2D::ZeroVector, FVector2D::UnitVector, FVector2D::ZeroVector, 45.0f));
    DiamondSlotBox->SetContent(FocusDiamond);

    USizeBox* DiamondShapeSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("DiamondShapeSize"));
    DiamondShapeSize->SetWidthOverride(7.0f);
    DiamondShapeSize->SetHeightOverride(7.0f);
    FocusDiamond->SetContent(DiamondShapeSize);

    USizeBox* BarSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("BarSize"));
    BarSize->SetWidthOverride(100.0f);
    BarSize->SetHeightOverride(6.0f);
    if (UVerticalBoxSlot* BarSlot = RootBox->AddChildToVerticalBox(BarSize))
    {
        BarSlot->SetHorizontalAlignment(HAlign_Center);
    }

    HealthBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("HealthBar"));
    HealthBar->SetPercent(1.0f);
    HealthBar->SetFillColorAndOpacity(FLinearColor(0.52f, 0.08f, 0.07f, 0.95f));
    HealthBar->SetWidgetStyle(FProgressBarStyle()
        .SetBackgroundImage(FSlateColorBrush(FLinearColor(0.14f, 0.11f, 0.09f, 0.42f)))
        .SetFillImage(FSlateColorBrush(FLinearColor(0.52f, 0.08f, 0.07f, 0.95f))));
    BarSize->SetContent(HealthBar);
}

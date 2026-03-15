#include "UI/DamageNumberWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"

UDamageNumberWidget::UDamageNumberWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UDamageNumberWidget::NativeConstruct()
{
    Super::NativeConstruct();
    BuildWidgetTree();
}

void UDamageNumberWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (!DamageText || LifetimeSeconds <= 0.0f)
    {
        return;
    }

    ElapsedSeconds = FMath::Min(ElapsedSeconds + InDeltaTime, LifetimeSeconds);
    const float Alpha = ElapsedSeconds / LifetimeSeconds;
    DamageText->SetRenderTranslation(FVector2D(0.0f, -FloatDistance * Alpha));
    DamageText->SetRenderOpacity(1.0f - Alpha);
}

void UDamageNumberWidget::SetDamageValue(float InDamage)
{
    BuildWidgetTree();
    ElapsedSeconds = 0.0f;

    if (DamageText)
    {
        DamageText->SetText(FText::AsNumber(FMath::RoundToInt(InDamage)));
        DamageText->SetRenderTranslation(FVector2D::ZeroVector);
        DamageText->SetRenderOpacity(1.0f);
    }
}

void UDamageNumberWidget::BuildWidgetTree()
{
    if (!WidgetTree || WidgetTree->RootWidget)
    {
        return;
    }

    DamageText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DamageText"));
    DamageText->SetText(FText::FromString(TEXT("0")));
    DamageText->SetColorAndOpacity(FSlateColor(FLinearColor(0.90f, 0.78f, 0.48f, 0.96f)));
    DamageText->SetShadowColorAndOpacity(FLinearColor(0.07f, 0.04f, 0.02f, 0.85f));
    DamageText->SetShadowOffset(FVector2D(1.0f, 1.0f));
    DamageText->SetJustification(ETextJustify::Center);
    WidgetTree->RootWidget = DamageText;
}

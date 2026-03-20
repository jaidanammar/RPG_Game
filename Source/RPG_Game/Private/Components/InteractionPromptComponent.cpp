#include "Components/InteractionPromptComponent.h"

#include "Blueprint/UserWidget.h"
#include "Components/InteractionScannerComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "UI/InteractionPromptWidget.h"

UInteractionPromptComponent::UInteractionPromptComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
    InteractKeyText = FText::FromString(TEXT("E"));
}

void UInteractionPromptComponent::BeginPlay()
{
    Super::BeginPlay();

    CachedScanner = GetOwner() ? GetOwner()->FindComponentByClass<UInteractionScannerComponent>() : nullptr;
    EnsurePromptWidget();
}

void UInteractionPromptComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    EnsurePromptWidget();
    RefreshPrompt();
}

bool UInteractionPromptComponent::EnsurePromptWidget()
{
    if (PromptWidget)
    {
        return true;
    }

    if (!CachedScanner.IsValid())
    {
        CachedScanner = GetOwner() ? GetOwner()->FindComponentByClass<UInteractionScannerComponent>() : nullptr;
        if (!CachedScanner.IsValid())
        {
            return false;
        }
    }

    APawn* OwnerPawn = Cast<APawn>(GetOwner());
    APlayerController* PlayerController = OwnerPawn ? Cast<APlayerController>(OwnerPawn->GetController()) : nullptr;
    if (!PlayerController)
    {
        PlayerController = UGameplayStatics::GetPlayerController(this, 0);
    }

    if (!PlayerController || !PlayerController->IsLocalController())
    {
        return false;
    }

    UClass* WidgetClass = PromptWidgetClass ? PromptWidgetClass.Get() : UInteractionPromptWidget::StaticClass();
    PromptWidget = CreateWidget<UInteractionPromptWidget>(PlayerController, WidgetClass);
    if (!PromptWidget)
    {
        return false;
    }

    PromptWidget->AddToViewport(50);
    PromptWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
    PromptWidget->SetPromptVisible(false);
    return true;
}

void UInteractionPromptComponent::RefreshPrompt()
{
    if (!PromptWidget || !CachedScanner.IsValid())
    {
        return;
    }

    const FText InteractionLabel = CachedScanner->GetCurrentInteractionLabel();
    const bool bShouldShow = !InteractionLabel.IsEmpty();

    if (!bShouldShow)
    {
        PromptWidget->SetPromptVisible(false);
        return;
    }

    const FText PromptText = FText::Format(
        NSLOCTEXT("InteractionPrompt", "PromptFormat", "Press '{0}' to {1}"),
        InteractKeyText,
        InteractionLabel);

    PromptWidget->SetPromptText(PromptText);
    PromptWidget->SetPromptVisible(true);
}

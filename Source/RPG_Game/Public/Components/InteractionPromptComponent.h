#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InteractionPromptComponent.generated.h"

class UInteractionPromptWidget;
class UInteractionScannerComponent;

UCLASS(ClassGroup=(RPG), BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class RPG_GAME_API UInteractionPromptComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UInteractionPromptComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
    TSubclassOf<UInteractionPromptWidget> PromptWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
    FText InteractKeyText;

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    bool EnsurePromptWidget();
    void RefreshPrompt();

    TWeakObjectPtr<UInteractionScannerComponent> CachedScanner;

    UPROPERTY(Transient)
    TObjectPtr<UInteractionPromptWidget> PromptWidget = nullptr;
};

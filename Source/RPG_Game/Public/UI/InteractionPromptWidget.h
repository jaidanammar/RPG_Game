#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InteractionPromptWidget.generated.h"

class UBorder;
class UTextBlock;
class UVerticalBox;

UCLASS()
class RPG_GAME_API UInteractionPromptWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UInteractionPromptWidget(const FObjectInitializer& ObjectInitializer);

    virtual void NativeConstruct() override;

    UFUNCTION(BlueprintCallable, Category = "Interaction")
    void SetPromptText(const FText& InPromptText);

    UFUNCTION(BlueprintCallable, Category = "Interaction")
    void SetPromptVisible(bool bVisible);

protected:
    void BuildWidgetTree();

    UPROPERTY(Transient)
    TObjectPtr<UVerticalBox> RootBox = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> ReticleText = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UBorder> PromptBorder = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> PromptText = nullptr;
};

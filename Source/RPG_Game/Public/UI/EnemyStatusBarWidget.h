#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EnemyStatusBarWidget.generated.h"

class UBorder;
class UProgressBar;

UCLASS()
class RPG_GAME_API UEnemyStatusBarWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UEnemyStatusBarWidget(const FObjectInitializer& ObjectInitializer);

    void SetHealthValues(float CurrentHealth, float MaxHealth);
    void SetFocused(bool bInFocused);

protected:
    virtual void NativeConstruct() override;

private:
    UPROPERTY(Transient)
    TObjectPtr<UProgressBar> HealthBar = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UBorder> FocusDiamond = nullptr;

    void BuildWidgetTree();
};

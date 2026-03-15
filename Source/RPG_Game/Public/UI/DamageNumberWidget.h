#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DamageNumberWidget.generated.h"

class UTextBlock;

UCLASS()
class RPG_GAME_API UDamageNumberWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UDamageNumberWidget(const FObjectInitializer& ObjectInitializer);

    void SetDamageValue(float InDamage);
    float GetLifetimeSeconds() const { return LifetimeSeconds; }

protected:
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
    UPROPERTY(EditAnywhere, Category = "Damage")
    float LifetimeSeconds = 0.7f;

    UPROPERTY(EditAnywhere, Category = "Damage")
    float FloatDistance = 26.0f;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> DamageText = nullptr;

    float ElapsedSeconds = 0.0f;

    void BuildWidgetTree();
};

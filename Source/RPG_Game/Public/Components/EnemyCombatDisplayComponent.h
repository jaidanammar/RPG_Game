#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/RPGCombatTypes.h"
#include "EnemyCombatDisplayComponent.generated.h"

class ACharacter;
class UPlayerStatsComponent;
class UTargetLockComponent;
class UWidgetComponent;

UCLASS(ClassGroup=(RPG), BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class RPG_GAME_API UEnemyCombatDisplayComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UEnemyCombatDisplayComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy UI")
    bool bShowHealthBar = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy UI")
    bool bShowOnlyWhenRelevant = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy UI")
    float VisibleAfterHitDuration = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy UI")
    FVector StatusWidgetOffset = FVector(0.0f, 0.0f, 206.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy UI")
    FVector DamageNumberOffset = FVector(0.0f, 0.0f, 182.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy UI")
    FVector2D StatusWidgetDrawSize = FVector2D(104.0f, 20.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy UI")
    FVector2D DamageWidgetDrawSize = FVector2D(64.0f, 20.0f);

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    TWeakObjectPtr<ACharacter> CachedCharacter;
    TWeakObjectPtr<UPlayerStatsComponent> CachedStats;
    TWeakObjectPtr<UTargetLockComponent> CachedPlayerTargetLock;

    UPROPERTY(Transient)
    TObjectPtr<UWidgetComponent> StatusWidgetComponent = nullptr;

    double LastHitDisplayTime = -100.0;

    UFUNCTION()
    void HandleHealthChanged(float CurrentHealth, float MaxHealth);

    UFUNCTION()
    void HandleHitReceived(FRPGDamageSpec DamageSpec, float DamageApplied, float NewHealth, float MaxHealth);

    UFUNCTION()
    void HandleDeath();

    void EnsureStatusWidget();
    void UpdateFocusedState();
    void UpdateVisibility(bool bIsFocused, double WorldTime);
    void SpawnDamageNumber(float DamageApplied, const FVector& HitLocation);
    UTargetLockComponent* ResolvePlayerTargetLock();
};

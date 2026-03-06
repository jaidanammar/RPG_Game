#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatStateComponent.generated.h"

class UPlayerStatsComponent;

UENUM(BlueprintType)
enum class ERPGCombatState : uint8
{
    Idle UMETA(DisplayName = "Idle"),
    AttackStartup UMETA(DisplayName = "Attack Startup"),
    AttackActive UMETA(DisplayName = "Attack Active"),
    AttackRecovery UMETA(DisplayName = "Attack Recovery"),
    Guard UMETA(DisplayName = "Guard"),
    Hitstun UMETA(DisplayName = "Hitstun"),
    Dead UMETA(DisplayName = "Dead")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCombatStateChanged, ERPGCombatState, OldState, ERPGCombatState, NewState);

UCLASS(ClassGroup=(RPG), BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class RPG_GAME_API UCombatStateComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UCombatStateComponent();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CombatState")
    ERPGCombatState CurrentState = ERPGCombatState::Idle;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatState")
    bool bEnforceTransitionRules = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatState")
    bool bLockStateWhenDead = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatState|Guard")
    bool bAllowGuardState = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatState|Guard", meta=(ClampMin="0.0"))
    float GuardStaminaDrainPerSecond = 12.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatState|Guard")
    bool bGuardDrainUsesPercentOfMaxStamina = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatState|Guard", meta=(ClampMin="0.0", ClampMax="100.0"))
    float GuardStaminaDrainPercentPerSecond = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatState|Guard")
    bool bDisableStaminaRegenWhileGuarding = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatState|Guard", meta=(ClampMin="0.01"))
    float GuardDrainTickInterval = 0.05f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatState|Guard", meta=(ClampMin="1", ClampMax="10"))
    int32 MaxGuardDrainTicksPerFrame = 3;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatState|Guard")
    bool bBreakGuardWhenStaminaDepleted = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatState|Guard", meta=(ClampMin="0.01"))
    float GuardBreakHitstunDuration = 0.35f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatState|Hitstun")
    bool bEnterHitstunOnDamage = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatState|Hitstun", meta=(ClampMin="0.01"))
    float DefaultHitstunDuration = 0.25f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatState|Hitstun")
    bool bGuardPreventsHitstun = true;

    UPROPERTY(BlueprintAssignable, Category = "CombatState|Events")
    FOnCombatStateChanged OnCombatStateChanged;

    UFUNCTION(BlueprintPure, Category = "CombatState")
    ERPGCombatState GetCombatState() const { return CurrentState; }

    UFUNCTION(BlueprintPure, Category = "CombatState")
    bool IsInState(ERPGCombatState QueryState) const { return CurrentState == QueryState; }

    UFUNCTION(BlueprintPure, Category = "CombatState")
    bool CanTransitionTo(ERPGCombatState NewState) const;

    UFUNCTION(BlueprintCallable, Category = "CombatState")
    bool RequestState(ERPGCombatState NewState, bool bForce = false);

    UFUNCTION(BlueprintCallable, Category = "CombatState|Guard")
    bool StartGuard();

    UFUNCTION(BlueprintCallable, Category = "CombatState|Guard")
    bool StopGuard();

    UFUNCTION(BlueprintCallable, Category = "CombatState|Hitstun")
    bool ApplyHitstun(float Duration = -1.0f);

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    TWeakObjectPtr<UPlayerStatsComponent> CachedStats;
    FTimerHandle HitstunTimerHandle;
    float GuardDrainAccumulator = 0.0f;

    UFUNCTION()
    void HandleOwnerDeath();

    UFUNCTION()
    void HandleOwnerDamaged(float Damage, float NewHealth, float MaxHealth);

    void EndHitstun();
    void TickGuardStamina(float DeltaTime);
    bool HasGuardStamina() const;
};

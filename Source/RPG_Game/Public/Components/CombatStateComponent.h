#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/RPGCombatTypes.h"
#include "CombatStateComponent.generated.h"

class AActor;
class UCharacterMovementComponent;
class UPlayerStatsComponent;
class ULocomotionComponent;

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
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGuardStateChanged, bool, bIsGuarding);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnParryWindowChanged, bool, bIsActive);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnParrySuccess, AActor*, AttackerActor, bool, bIsPerfectParry);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnParryFailed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHitReactionUpdated, ERPGHitReactionStrength, ReactionStrength, ERPGHitDirection, HitDirection);

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

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatState|Guard")
    bool bLimitMovementWhileGuarding = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatState|Guard", meta=(ClampMin="0.0", ClampMax="1.0"))
    float GuardWalkSpeedMultiplier = 0.55f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatState|Guard", meta=(ClampMin="0.01"))
    float GuardDrainTickInterval = 0.05f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatState|Guard", meta=(ClampMin="1", ClampMax="10"))
    int32 MaxGuardDrainTicksPerFrame = 3;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatState|Guard")
    bool bBreakGuardWhenStaminaDepleted = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatState|Guard", meta=(ClampMin="0.01"))
    float GuardBreakHitstunDuration = 0.35f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatState|Parry")
    bool bAllowParry = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatState|Parry", meta=(ClampMin="0.01"))
    float ParryWindowDuration = 0.15f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatState|Parry", meta=(ClampMin="0.0"))
    float PerfectParryWindowDuration = 0.06f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatState|Parry", meta=(ClampMin="0.0"))
    float ParryCooldown = 0.3f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatState|Parry", meta=(ClampMin="0.0"))
    float ParryStaminaCost = 8.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatState|Parry", meta=(ClampMin="0.0"))
    float ParryFailStaminaCost = 4.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatState|Parry")
    bool bEnterGuardStateOnParrySuccess = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatState|Parry")
    bool bBeginParryOnGuardPressed = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CombatState|Parry")
    bool bParryWindowActive = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CombatState|Parry")
    bool bParryOnCooldown = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatState|Hitstun")
    bool bEnterHitstunOnDamage = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatState|Hitstun", meta=(ClampMin="0.01"))
    float DefaultHitstunDuration = 0.25f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatState|Hitstun")
    bool bGuardPreventsHitstun = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatState|Guard", meta=(ClampMin="0.0", ClampMax="1.0"))
    float GuardChipDamageMultiplier = 0.1f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatState|Guard", meta=(ClampMin="0.0"))
    float GuardStaminaDamageMultiplier = 1.25f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CombatState|HitReaction")
    ERPGHitReactionStrength LastHitReactionStrength = ERPGHitReactionStrength::None;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CombatState|HitReaction")
    ERPGHitDirection LastHitDirection = ERPGHitDirection::Front;

    UPROPERTY(BlueprintAssignable, Category = "CombatState|Events")
    FOnCombatStateChanged OnCombatStateChanged;

    UPROPERTY(BlueprintAssignable, Category = "CombatState|Events")
    FOnGuardStateChanged OnGuardStateChanged;

    UPROPERTY(BlueprintAssignable, Category = "CombatState|Events")
    FOnParryWindowChanged OnParryWindowChanged;

    UPROPERTY(BlueprintAssignable, Category = "CombatState|Events")
    FOnParrySuccess OnParrySuccess;

    UPROPERTY(BlueprintAssignable, Category = "CombatState|Events")
    FOnParryFailed OnParryFailed;

    UPROPERTY(BlueprintAssignable, Category = "CombatState|Events")
    FOnHitReactionUpdated OnHitReactionUpdated;

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

    UFUNCTION(BlueprintCallable, Category = "CombatState|Guard")
    void HandleGuardPressed();

    UFUNCTION(BlueprintCallable, Category = "CombatState|Guard")
    void HandleGuardReleased();

    UFUNCTION(BlueprintCallable, Category = "CombatState|Parry")
    bool BeginParryAttempt();

    UFUNCTION(BlueprintPure, Category = "CombatState|Parry")
    bool IsParryWindowCurrentlyActive() const { return bParryWindowActive; }

    UFUNCTION(BlueprintPure, Category = "CombatState|Parry")
    bool IsParryOnCooldown() const { return bParryOnCooldown; }

    UFUNCTION(BlueprintCallable, Category = "CombatState|Parry")
    bool TryNegateIncomingDamage(const FRPGDamageSpec& DamageSpec, bool& bOutPerfectParry);

    UFUNCTION(BlueprintCallable, Category = "CombatState|Hitstun")
    bool ApplyHitstun(float Duration = -1.0f);

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    TWeakObjectPtr<UPlayerStatsComponent> CachedStats;
    TWeakObjectPtr<ULocomotionComponent> CachedLocomotion;
    TWeakObjectPtr<UCharacterMovementComponent> CachedMoveComp;
    FTimerHandle HitstunTimerHandle;
    FTimerHandle ParryWindowTimerHandle;
    FTimerHandle ParryCooldownTimerHandle;
    float GuardDrainAccumulator = 0.0f;
    float SavedGuardWalkSpeed = 0.0f;
    bool bGuardWalkSpeedOverrideActive = false;
    bool bGuardInputHeld = false;
    double PerfectParryWindowEndTime = 0.0;

    UFUNCTION()
    void HandleOwnerDeath();

    UFUNCTION()
    void HandleOwnerHitReceived(FRPGDamageSpec DamageSpec, float DamageApplied, float NewHealth, float MaxHealth);

    void EndHitstun();
    void TickGuardStamina(float DeltaTime);
    bool HasGuardStamina() const;
    void ApplyGuardMovementPolicy();
    void RestoreGuardMovementPolicy();
    void EndParryWindow(bool bBroadcastFail);
    void EndParryCooldown();
};










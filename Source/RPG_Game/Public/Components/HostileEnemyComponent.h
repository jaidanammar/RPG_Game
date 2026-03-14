#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/CombatStateComponent.h"
#include "Data/RPGCombatTypes.h"
#include "HostileEnemyComponent.generated.h"

class AActor;
class ACharacter;
class UAttackSystemComponent;
class UCombatStateComponent;
class UPlayerStatsComponent;
class UTargetLockComponent;

UCLASS(ClassGroup=(RPG), BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class RPG_GAME_API UHostileEnemyComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UHostileEnemyComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Aggro")
    bool bBecomeHostileOnDamage = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Aggro")
    bool bFaceTargetWhileHostile = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Combat", meta = (ClampMin = "50.0"))
    float AttackRange = 180.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Combat", meta = (ClampMin = "0.1"))
    float AttackInterval = 1.2f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Combat", meta = (ClampMin = "-1.0", ClampMax = "1.0"))
    float MinFacingDotToAttack = 0.8f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Combat", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float BaseAttackChance = 0.4f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Combat", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float PunishAttackChance = 0.85f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Combat", meta = (ClampMin = "0.05"))
    float MinDecisionInterval = 0.18f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Combat", meta = (ClampMin = "0.05"))
    float MaxDecisionInterval = 0.4f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Combat", meta = (ClampMin = "50.0"))
    float ThreatGuardRange = 220.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Combat", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float GuardAgainstAttackChance = 0.8f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Combat", meta = (ClampMin = "0.05"))
    float MinGuardHoldDuration = 0.25f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Combat", meta = (ClampMin = "0.05"))
    float MaxGuardHoldDuration = 0.6f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Combat", meta = (ClampMin = "0.05"))
    float PostGuardAttackDelay = 0.2f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Movement", meta = (ClampMin = "50.0"))
    float ChaseStopDistance = 140.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Movement", meta = (ClampMin = "50.0"))
    float PreferredCombatDistance = 150.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Movement", meta = (ClampMin = "0.0"))
    float RetreatDistance = 110.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Movement", meta = (ClampMin = "0.0"))
    float RetreatDuration = 0.45f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Movement", meta = (ClampMin = "0.1"))
    float FaceTargetInterpSpeed = 8.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|State")
    bool bIsHostile = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|State")
    TObjectPtr<AActor> CurrentTarget = nullptr;

    UFUNCTION(BlueprintCallable, Category = "Enemy")
    void SetHostileTarget(AActor* NewTarget);

    UFUNCTION(BlueprintCallable, Category = "Enemy")
    void ClearHostileTarget();

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    TWeakObjectPtr<ACharacter> CachedCharacter;
    TWeakObjectPtr<UAttackSystemComponent> CachedAttackSystem;
    TWeakObjectPtr<UCombatStateComponent> CachedCombatState;
    TWeakObjectPtr<UPlayerStatsComponent> CachedStats;
    TWeakObjectPtr<UTargetLockComponent> CachedTargetLock;
    double NextAttackTime = 0.0;
    double NextDecisionTime = 0.0;
    double GuardReleaseTime = 0.0;
    double RetreatEndTime = 0.0;
    ERPGCombatState LastObservedTargetState = ERPGCombatState::Idle;

    UFUNCTION()
    void HandleOwnerHitReceived(FRPGDamageSpec DamageSpec, float DamageApplied, float NewHealth, float MaxHealth);

    void UpdateHostileBehavior(float DeltaTime);
    bool CanAttackTarget() const;
    bool IsFacingTarget(const FVector& DirectionToTarget) const;
    bool IsTargetValid(AActor* TargetActor) const;
    bool TryStartGuardAgainstTargetState(ERPGCombatState TargetState, float DistanceToTarget, double WorldTime);
    void StopGuardIfNeeded(bool bThreatActive, double WorldTime);
    bool ShouldPunishTarget(ERPGCombatState TargetState, float DistanceToTarget) const;
    void StartRetreat(double WorldTime);
    float GetNextDecisionDelay() const;
};
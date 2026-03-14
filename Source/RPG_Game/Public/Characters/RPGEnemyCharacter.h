#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "RPGEnemyCharacter.generated.h"

class UAnimationAsset;
class UAttackSystemComponent;
class UCombatStateComponent;
class UHostileEnemyComponent;
class ULocomotionComponent;
class UPlayerStatsComponent;
class USceneComponent;
class UStaticMeshComponent;
class UTargetLockComponent;
class UWeaponLoadoutComponent;

enum class ERPGHitDirection : uint8;
enum class ERPGHitReactionStrength : uint8;

UCLASS()
class RPG_GAME_API ARPGEnemyCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    ARPGEnemyCharacter();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UPlayerStatsComponent> StatsComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UCombatStateComponent> CombatStateComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UAttackSystemComponent> AttackSystemComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<ULocomotionComponent> LocomotionComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UTargetLockComponent> TargetLockComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UWeaponLoadoutComponent> WeaponLoadoutComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UHostileEnemyComponent> HostileEnemyComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
    TObjectPtr<USceneComponent> WeaponTraceStart;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
    TObjectPtr<USceneComponent> WeaponTraceEnd;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
    TObjectPtr<UStaticMeshComponent> VisibleWeaponMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    FName EquippedWeaponSocketName = TEXT("hand_r");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Reaction")
    bool bPlayHitReactions = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Reaction")
    FName HitReactionSlotName = TEXT("DefaultSlot");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Reaction", meta = (ClampMin = "0.0"))
    float HitReactionBlendInTime = 0.05f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Reaction", meta = (ClampMin = "0.0"))
    float HitReactionBlendOutTime = 0.1f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Death")
    bool bEnableRagdollOnDeath = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Death", meta = (ClampMin = "0.0"))
    float DeathLifeSpan = 10.0f;

protected:
    virtual void BeginPlay() override;

private:
    UFUNCTION()
    void HandleHitReactionUpdated(ERPGHitReactionStrength ReactionStrength, ERPGHitDirection HitDirection);

    UFUNCTION()
    void HandleDeath();

    UAnimationAsset* ResolveHitReactionAnimation(ERPGHitReactionStrength ReactionStrength, ERPGHitDirection HitDirection) const;
    void EnterRagdoll();
};
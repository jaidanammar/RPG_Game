#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Components/CombatStateComponent.h"
#include "Data/RPGWeaponDataAssets.h"
#include "RPGCharacterAnimInstance.generated.h"

class ACharacter;
class UAnimSequenceBase;
class UBlendSpace;
class UCombatStateComponent;
class UCharacterMovementComponent;
class UTargetLockComponent;
class UWeaponLoadoutComponent;

UCLASS(Blueprintable)
class RPG_GAME_API URPGCharacterAnimInstance : public UAnimInstance
{
    GENERATED_BODY()

public:
    virtual void NativeInitializeAnimation() override;
    virtual void NativeUpdateAnimation(float DeltaSeconds) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation")
    float GroundSpeed = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation")
    float ForwardSpeed = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation")
    float RightSpeed = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation")
    float MovementDirectionDegrees = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation")
    bool bIsInAir = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation")
    bool bIsAccelerating = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation")
    bool bIsLockedOn = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation")
    bool bUseStrafeLocomotion = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation")
    bool bIsGuarding = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation")
    bool bIsParryWindowActive = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation")
    ERPGCombatState CombatState = ERPGCombatState::Idle;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation")
    ERPGWeaponType ActiveWeaponType = ERPGWeaponType::Unarmed;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation|Weapon")
    ERPGWeaponType PreviousWeaponType = ERPGWeaponType::Unarmed;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation|Weapon")
    bool bIsWeaponEquipped = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation|Weapon")
    bool bPendingEquipTransition = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation|Weapon")
    bool bPendingUnequipTransition = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation|Weapon")
    TObjectPtr<UAnimationAsset> IdleAnimation = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation|Weapon")
    TObjectPtr<UAnimationAsset> WalkAnimation = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation|Weapon")
    TObjectPtr<UAnimationAsset> RunAnimation = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation|Weapon")
    TObjectPtr<UAnimationAsset> SprintAnimation = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation|Weapon")
    TObjectPtr<UAnimationAsset> GuardIdleAnimation = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation|Weapon")
    TObjectPtr<UAnimationAsset> GuardMoveAnimation = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation|Weapon")
    TObjectPtr<UAnimationAsset> RunStartAnimation = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation|Weapon")
    TObjectPtr<UAnimationAsset> RunStopAnimation = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation|Weapon")
    TObjectPtr<UAnimationAsset> JumpStartAnimation = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation|Weapon")
    TObjectPtr<UAnimationAsset> JumpLoopAnimation = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation|Weapon")
    TObjectPtr<UAnimationAsset> JumpLandAnimation = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation|Weapon")
    TObjectPtr<UAnimationAsset> FallLoopAnimation = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation|Weapon")
    TObjectPtr<UAnimationAsset> GuardEnterAnimation = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation|Weapon")
    TObjectPtr<UAnimationAsset> GuardLoopAnimation = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation|Weapon")
    TObjectPtr<UAnimationAsset> GuardExitAnimation = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation|Weapon")
    TObjectPtr<UAnimationAsset> ParryAnimation = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation|Weapon")
    TObjectPtr<UAnimationAsset> HitLightFrontAnimation = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation|Weapon")
    TObjectPtr<UAnimationAsset> HitLightBackAnimation = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation|Weapon")
    TObjectPtr<UAnimationAsset> HitHeavyFrontAnimation = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation|Weapon")
    TObjectPtr<UAnimationAsset> HitHeavyBackAnimation = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation|Weapon")
    TObjectPtr<UAnimationAsset> GuardBreakAnimation = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation|Weapon")
    TObjectPtr<UAnimationAsset> EquipAnimation = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation|Weapon")
    TObjectPtr<UAnimationAsset> UnequipAnimation = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation|Weapon|Typed")
    TObjectPtr<UAnimSequenceBase> IdleSequence = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation|Weapon|Typed")
    TObjectPtr<UBlendSpace> WalkBlendSpace = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation|Weapon|Typed")
    TObjectPtr<UBlendSpace> RunBlendSpace = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation|Weapon|Typed")
    TObjectPtr<UBlendSpace> SprintBlendSpace = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation|Weapon|Typed")
    TObjectPtr<UAnimSequenceBase> GuardIdleSequence = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation|Weapon|Typed")
    TObjectPtr<UBlendSpace> GuardMoveBlendSpace = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation|Weapon|Typed")
    TObjectPtr<UAnimSequenceBase> RunStartSequence = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation|Weapon|Typed")
    TObjectPtr<UAnimSequenceBase> RunStopSequence = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation|Weapon|Typed")
    TObjectPtr<UAnimSequenceBase> JumpStartSequence = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation|Weapon|Typed")
    TObjectPtr<UAnimSequenceBase> JumpLoopSequence = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation|Weapon|Typed")
    TObjectPtr<UAnimSequenceBase> JumpLandSequence = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation|Weapon|Typed")
    TObjectPtr<UAnimSequenceBase> FallLoopSequence = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation|Weapon|Typed")
    TObjectPtr<UAnimSequenceBase> GuardEnterSequence = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation|Weapon|Typed")
    TObjectPtr<UAnimSequenceBase> GuardLoopSequence = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation|Weapon|Typed")
    TObjectPtr<UAnimSequenceBase> GuardExitSequence = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation|Weapon|Typed")
    TObjectPtr<UAnimSequenceBase> ParrySequence = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation|Weapon|Typed")
    TObjectPtr<UAnimSequenceBase> HitLightFrontSequence = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation|Weapon|Typed")
    TObjectPtr<UAnimSequenceBase> HitLightBackSequence = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation|Weapon|Typed")
    TObjectPtr<UAnimSequenceBase> HitHeavyFrontSequence = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation|Weapon|Typed")
    TObjectPtr<UAnimSequenceBase> HitHeavyBackSequence = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation|Weapon|Typed")
    TObjectPtr<UAnimSequenceBase> GuardBreakSequence = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation|Weapon|Typed")
    TObjectPtr<UAnimSequenceBase> EquipSequence = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation|Weapon|Typed")
    TObjectPtr<UAnimSequenceBase> UnequipSequence = nullptr;

    UFUNCTION(BlueprintPure, Category = "Animation|Weapon")
    UAnimationAsset* GetWeaponAnimationForSlot(ERPGAnimationSlot Slot) const;

    UFUNCTION(BlueprintCallable, Category = "Animation|Weapon")
    void ConsumeWeaponTransitionFlags();

protected:
    void RefreshCachedComponents();
    void RefreshWeaponAnimationSlots();
    void QueueWeaponTransitionFlagConsumption();

    TWeakObjectPtr<ACharacter> CachedCharacter;
    TWeakObjectPtr<UCharacterMovementComponent> CachedMovementComponent;
    TWeakObjectPtr<UCombatStateComponent> CachedCombatState;
    TWeakObjectPtr<UTargetLockComponent> CachedTargetLock;
    TWeakObjectPtr<UWeaponLoadoutComponent> CachedWeaponLoadout;
    int32 WeaponTransitionConsumeFramesRemaining = 0;
};




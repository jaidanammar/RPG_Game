#include "RPGCharacterAnimInstance.h"

#include "Animation/AnimSequence.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/BlendSpace.h"
#include "Components/TargetLockComponent.h"
#include "Components/WeaponLoadoutComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace
{
UAnimSequenceBase* ResolveFullBodySequence(UAnimationAsset* Animation, UAnimSequenceBase* Fallback = nullptr)
{
    UAnimSequenceBase* Sequence = Cast<UAnimSequenceBase>(Animation);
    if (!Sequence)
    {
        return Fallback;
    }

    if (Sequence->IsValidAdditive())
    {
        return Fallback;
    }

    return Sequence;
}
}

void URPGCharacterAnimInstance::NativeInitializeAnimation()
{
    Super::NativeInitializeAnimation();
    RefreshCachedComponents();
    RefreshWeaponAnimationSlots();

    bIsWeaponEquipped = ActiveWeaponType != ERPGWeaponType::Unarmed;
    PreviousWeaponType = ActiveWeaponType;
    bPendingEquipTransition = false;
    bPendingUnequipTransition = false;
}

void URPGCharacterAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
    Super::NativeUpdateAnimation(DeltaSeconds);

    RefreshCachedComponents();

    if (!CachedCharacter.IsValid())
    {
        return;
    }

    const FVector Velocity = CachedCharacter->GetVelocity();
    FVector HorizontalVelocity = Velocity;
    HorizontalVelocity.Z = 0.0f;
    GroundSpeed = HorizontalVelocity.Size();

    if (CachedTargetLock.IsValid())
    {
        ForwardSpeed = CachedTargetLock->GetSignedForwardSpeed();
        RightSpeed = CachedTargetLock->GetSignedRightSpeed();
        MovementDirectionDegrees = CachedTargetLock->GetMovementDirectionDegrees();
        bIsLockedOn = CachedTargetLock->IsLockedOn();
        bUseStrafeLocomotion = CachedTargetLock->ShouldUseStrafeLocomotion();
    }
    else
    {
        ForwardSpeed = GroundSpeed;
        RightSpeed = 0.0f;
        MovementDirectionDegrees = 0.0f;
        bIsLockedOn = false;
        bUseStrafeLocomotion = false;
    }

    if (CachedMovementComponent.IsValid())
    {
        bIsInAir = CachedMovementComponent->IsFalling();
        bIsAccelerating = !CachedMovementComponent->GetCurrentAcceleration().IsNearlyZero();
    }
    else
    {
        bIsInAir = false;
        bIsAccelerating = false;
    }

    if (CachedCombatState.IsValid())
    {
        CombatState = CachedCombatState->GetCombatState();
        bIsGuarding = CachedCombatState->IsInState(ERPGCombatState::Guard);
        bIsParryWindowActive = CachedCombatState->IsParryWindowCurrentlyActive();
    }
    else
    {
        CombatState = ERPGCombatState::Idle;
        bIsGuarding = false;
        bIsParryWindowActive = false;
    }

    const ERPGWeaponType LastWeaponType = ActiveWeaponType;
    UAnimationAsset* LastEquipAnimation = EquipAnimation;
    UAnimationAsset* LastUnequipAnimation = UnequipAnimation;
    UAnimSequenceBase* LastEquipSequence = EquipSequence;
    UAnimSequenceBase* LastUnequipSequence = UnequipSequence;

    if (CachedWeaponLoadout.IsValid())
    {
        ActiveWeaponType = CachedWeaponLoadout->ActiveWeaponType;
    }
    else
    {
        ActiveWeaponType = ERPGWeaponType::Unarmed;
    }

    RefreshWeaponAnimationSlots();

    const bool bWasWeaponEquipped = LastWeaponType != ERPGWeaponType::Unarmed;
    bIsWeaponEquipped = ActiveWeaponType != ERPGWeaponType::Unarmed;

    if (ActiveWeaponType != LastWeaponType)
    {
        PreviousWeaponType = LastWeaponType;

        if (!bWasWeaponEquipped && bIsWeaponEquipped)
        {
            if (!EquipSequence && LastEquipSequence)
            {
                EquipSequence = LastEquipSequence;
                EquipAnimation = LastEquipAnimation;
            }

            bPendingEquipTransition = true;
            bPendingUnequipTransition = false;
            QueueWeaponTransitionFlagConsumption();
        }
        else if (bWasWeaponEquipped && !bIsWeaponEquipped)
        {
            if (!UnequipSequence && LastUnequipSequence)
            {
                UnequipSequence = LastUnequipSequence;
                UnequipAnimation = LastUnequipAnimation;
            }

            bPendingEquipTransition = false;
            bPendingUnequipTransition = true;
            QueueWeaponTransitionFlagConsumption();
        }
        else if (bWasWeaponEquipped && bIsWeaponEquipped)
        {
            if (!EquipSequence && LastEquipSequence)
            {
                EquipSequence = LastEquipSequence;
                EquipAnimation = LastEquipAnimation;
            }

            bPendingEquipTransition = true;
            bPendingUnequipTransition = false;
            QueueWeaponTransitionFlagConsumption();
        }
    }
    else if (WeaponTransitionConsumeFramesRemaining > 0)
    {
        --WeaponTransitionConsumeFramesRemaining;
        if (WeaponTransitionConsumeFramesRemaining <= 0)
        {
            ConsumeWeaponTransitionFlags();
        }
    }
}

UAnimationAsset* URPGCharacterAnimInstance::GetWeaponAnimationForSlot(ERPGAnimationSlot Slot) const
{
    return CachedWeaponLoadout.IsValid() ? CachedWeaponLoadout->ResolveAnimationForSlot(Slot) : nullptr;
}

void URPGCharacterAnimInstance::ConsumeWeaponTransitionFlags()
{
    bPendingEquipTransition = false;
    bPendingUnequipTransition = false;
    WeaponTransitionConsumeFramesRemaining = 0;
}

void URPGCharacterAnimInstance::QueueWeaponTransitionFlagConsumption()
{
    // Give the anim graph a reasonable window to consume the transition request without leaving it latched indefinitely.
    WeaponTransitionConsumeFramesRemaining = 12;
}

void URPGCharacterAnimInstance::RefreshCachedComponents()
{
    if (!CachedCharacter.IsValid())
    {
        CachedCharacter = Cast<ACharacter>(TryGetPawnOwner());
    }

    if (!CachedCharacter.IsValid())
    {
        CachedMovementComponent = nullptr;
        CachedCombatState = nullptr;
        CachedTargetLock = nullptr;
        CachedWeaponLoadout = nullptr;
        return;
    }

    if (!CachedMovementComponent.IsValid())
    {
        CachedMovementComponent = CachedCharacter->GetCharacterMovement();
    }

    if (!CachedCombatState.IsValid())
    {
        CachedCombatState = CachedCharacter->FindComponentByClass<UCombatStateComponent>();
    }

    if (!CachedTargetLock.IsValid())
    {
        CachedTargetLock = CachedCharacter->FindComponentByClass<UTargetLockComponent>();
    }

    if (!CachedWeaponLoadout.IsValid())
    {
        CachedWeaponLoadout = CachedCharacter->FindComponentByClass<UWeaponLoadoutComponent>();
    }
}

void URPGCharacterAnimInstance::RefreshWeaponAnimationSlots()
{
    IdleAnimation = GetWeaponAnimationForSlot(ERPGAnimationSlot::Idle);
    WalkAnimation = GetWeaponAnimationForSlot(ERPGAnimationSlot::Walk);
    RunAnimation = GetWeaponAnimationForSlot(ERPGAnimationSlot::Run);
    SprintAnimation = GetWeaponAnimationForSlot(ERPGAnimationSlot::Sprint);
    GuardIdleAnimation = GetWeaponAnimationForSlot(ERPGAnimationSlot::GuardIdle);
    GuardMoveAnimation = GetWeaponAnimationForSlot(ERPGAnimationSlot::GuardMove);
    RunStartAnimation = GetWeaponAnimationForSlot(ERPGAnimationSlot::RunStart);
    RunStopAnimation = GetWeaponAnimationForSlot(ERPGAnimationSlot::RunStop);
    JumpStartAnimation = GetWeaponAnimationForSlot(ERPGAnimationSlot::JumpStart);
    JumpLoopAnimation = GetWeaponAnimationForSlot(ERPGAnimationSlot::JumpLoop);
    JumpLandAnimation = GetWeaponAnimationForSlot(ERPGAnimationSlot::JumpLand);
    FallLoopAnimation = GetWeaponAnimationForSlot(ERPGAnimationSlot::FallLoop);
    GuardEnterAnimation = GetWeaponAnimationForSlot(ERPGAnimationSlot::GuardEnter);
    GuardLoopAnimation = GetWeaponAnimationForSlot(ERPGAnimationSlot::GuardLoop);
    GuardExitAnimation = GetWeaponAnimationForSlot(ERPGAnimationSlot::GuardExit);
    ParryAnimation = GetWeaponAnimationForSlot(ERPGAnimationSlot::Parry);
    HitLightFrontAnimation = GetWeaponAnimationForSlot(ERPGAnimationSlot::HitLightFront);
    HitLightBackAnimation = GetWeaponAnimationForSlot(ERPGAnimationSlot::HitLightBack);
    HitHeavyFrontAnimation = GetWeaponAnimationForSlot(ERPGAnimationSlot::HitHeavyFront);
    HitHeavyBackAnimation = GetWeaponAnimationForSlot(ERPGAnimationSlot::HitHeavyBack);
    GuardBreakAnimation = GetWeaponAnimationForSlot(ERPGAnimationSlot::GuardBreak);
    EquipAnimation = GetWeaponAnimationForSlot(ERPGAnimationSlot::Equip);
    UnequipAnimation = GetWeaponAnimationForSlot(ERPGAnimationSlot::Unequip);

    IdleSequence = ResolveFullBodySequence(IdleAnimation);
    WalkBlendSpace = Cast<UBlendSpace>(WalkAnimation);
    RunBlendSpace = Cast<UBlendSpace>(RunAnimation);
    SprintBlendSpace = Cast<UBlendSpace>(SprintAnimation);
    GuardIdleSequence = ResolveFullBodySequence(GuardIdleAnimation);
    GuardMoveBlendSpace = Cast<UBlendSpace>(GuardMoveAnimation);
    RunStartSequence = ResolveFullBodySequence(RunStartAnimation);
    RunStopSequence = ResolveFullBodySequence(RunStopAnimation);
    JumpStartSequence = ResolveFullBodySequence(JumpStartAnimation);
    JumpLoopSequence = ResolveFullBodySequence(JumpLoopAnimation);
    FallLoopSequence = ResolveFullBodySequence(FallLoopAnimation, JumpLoopSequence);
    JumpLandSequence = ResolveFullBodySequence(JumpLandAnimation);
    GuardEnterSequence = ResolveFullBodySequence(GuardEnterAnimation);
    GuardLoopSequence = ResolveFullBodySequence(GuardLoopAnimation);
    GuardExitSequence = ResolveFullBodySequence(GuardExitAnimation);
    if (!GuardIdleSequence)
    {
        GuardIdleSequence = GuardLoopSequence;
    }
    ParrySequence = ResolveFullBodySequence(ParryAnimation);
    HitLightFrontSequence = ResolveFullBodySequence(HitLightFrontAnimation);
    HitLightBackSequence = ResolveFullBodySequence(HitLightBackAnimation);
    HitHeavyFrontSequence = ResolveFullBodySequence(HitHeavyFrontAnimation);
    HitHeavyBackSequence = ResolveFullBodySequence(HitHeavyBackAnimation);
    GuardBreakSequence = ResolveFullBodySequence(GuardBreakAnimation);
    EquipSequence = ResolveFullBodySequence(EquipAnimation);
    UnequipSequence = ResolveFullBodySequence(UnequipAnimation);
}





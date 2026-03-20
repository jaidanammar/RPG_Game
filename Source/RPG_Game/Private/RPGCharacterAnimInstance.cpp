#include "RPGCharacterAnimInstance.h"

#include "Animation/AnimSequence.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/AnimMontage.h"
#include "Animation/BlendSpace.h"
#include "Components/LocomotionComponent.h"
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

    DefaultLocomotionBlendSpace = RunBlendSpace ? RunBlendSpace : WalkBlendSpace;
    if (CachedLocomotionComponent.IsValid() && CachedLocomotionComponent->DesiredGait == ERPGLocomotionGait::Sprint && SprintBlendSpace)
    {
        DefaultLocomotionBlendSpace = SprintBlendSpace;
    }
    CurrentLocomotionBlendSpace = DefaultLocomotionBlendSpace;

    const bool bUseFocusLocomotionOverrides = bUseFocusedLocomotionOverrides && bUseStrafeLocomotion && !bIsGuarding;
    if (bUseFocusLocomotionOverrides)
    {
        if (FocusIdleSequence)
        {
            IdleSequence = FocusIdleSequence;
        }

        if (FocusMoveBlendSpace)
        {
            CurrentLocomotionBlendSpace = FocusMoveBlendSpace;
        }
    }

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
        CachedLocomotionComponent = nullptr;
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

    if (!CachedLocomotionComponent.IsValid())
    {
        CachedLocomotionComponent = CachedCharacter->FindComponentByClass<ULocomotionComponent>();
    }

    if (CachedCombatState != BoundCombatState)
    {
        if (BoundCombatState.IsValid())
        {
            BoundCombatState->OnHitReactionUpdated.RemoveDynamic(this, &URPGCharacterAnimInstance::HandleHitReactionUpdated);
            BoundCombatState->OnParrySuccess.RemoveDynamic(this, &URPGCharacterAnimInstance::HandleParrySuccess);
            BoundCombatState->OnParried.RemoveDynamic(this, &URPGCharacterAnimInstance::HandleParried);
        }

        BoundCombatState = CachedCombatState;
        if (BoundCombatState.IsValid())
        {
            BoundCombatState->OnHitReactionUpdated.RemoveDynamic(this, &URPGCharacterAnimInstance::HandleHitReactionUpdated);
            BoundCombatState->OnHitReactionUpdated.AddDynamic(this, &URPGCharacterAnimInstance::HandleHitReactionUpdated);
            BoundCombatState->OnParrySuccess.RemoveDynamic(this, &URPGCharacterAnimInstance::HandleParrySuccess);
            BoundCombatState->OnParried.RemoveDynamic(this, &URPGCharacterAnimInstance::HandleParried);
            BoundCombatState->OnParrySuccess.AddDynamic(this, &URPGCharacterAnimInstance::HandleParrySuccess);
            BoundCombatState->OnParried.AddDynamic(this, &URPGCharacterAnimInstance::HandleParried);
        }
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
    FocusIdleAnimation = GetWeaponAnimationForSlot(ERPGAnimationSlot::FocusIdle);
    FocusMoveAnimation = GetWeaponAnimationForSlot(ERPGAnimationSlot::FocusMove);
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
    ParriedAnimation = GetWeaponAnimationForSlot(ERPGAnimationSlot::Parried);
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
    FocusIdleSequence = ResolveFullBodySequence(FocusIdleAnimation);
    FocusMoveBlendSpace = Cast<UBlendSpace>(FocusMoveAnimation);
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
    ParriedSequence = ResolveFullBodySequence(ParriedAnimation);
    HitLightFrontSequence = ResolveFullBodySequence(HitLightFrontAnimation);
    HitLightBackSequence = ResolveFullBodySequence(HitLightBackAnimation);
    HitHeavyFrontSequence = ResolveFullBodySequence(HitHeavyFrontAnimation);
    HitHeavyBackSequence = ResolveFullBodySequence(HitHeavyBackAnimation);
    GuardBreakSequence = ResolveFullBodySequence(GuardBreakAnimation);
    EquipSequence = ResolveFullBodySequence(EquipAnimation);
    UnequipSequence = ResolveFullBodySequence(UnequipAnimation);
}





void URPGCharacterAnimInstance::HandleHitReactionUpdated(ERPGHitReactionStrength ReactionStrength, ERPGHitDirection HitDirection)
{
    if (!bPlayHitReactions || CombatState == ERPGCombatState::Dead)
    {
        return;
    }

    UAnimSequenceBase* HitReaction = ResolveHitReactionSequence(ReactionStrength, HitDirection);
    if (!HitReaction)
    {
        return;
    }

    PlaySlotAnimationAsDynamicMontage(
        HitReaction,
        HitReactionSlotName,
        HitReactionBlendInTime,
        HitReactionBlendOutTime,
        1.0f,
        1,
        0.0f,
        0.0f);
}

void URPGCharacterAnimInstance::HandleParrySuccess(AActor* AttackerActor, bool bIsPerfectParry)
{
    if (CombatState == ERPGCombatState::Dead || !ParryAnimation)
    {
        return;
    }

    const float PlayRate = bIsPerfectParry ? 1.1f : 1.0f;
    if (UAnimMontage* ParryMontage = Cast<UAnimMontage>(ParryAnimation))
    {
        Montage_Play(ParryMontage, PlayRate);
        return;
    }

    if (!ParrySequence)
    {
        return;
    }

    PlaySlotAnimationAsDynamicMontage(
        ParrySequence,
        HitReactionSlotName,
        0.02f,
        0.08f,
        PlayRate,
        1,
        0.0f,
        0.0f);
}

void URPGCharacterAnimInstance::HandleParried(bool bIsPerfectParry)
{
    if (CombatState == ERPGCombatState::Dead)
    {
        return;
    }

    const float PlayRate = bIsPerfectParry ? 1.05f : 1.0f;
    if (ParriedAnimation)
    {
        if (UAnimMontage* ParriedMontage = Cast<UAnimMontage>(ParriedAnimation))
        {
            Montage_Play(ParriedMontage, PlayRate);
            return;
        }

        if (ParriedSequence)
        {
            PlaySlotAnimationAsDynamicMontage(
                ParriedSequence,
                HitReactionSlotName,
                0.02f,
                0.1f,
                PlayRate,
                1,
                0.0f,
                0.0f);
            return;
        }
    }

    if (GuardBreakSequence)
    {
        PlaySlotAnimationAsDynamicMontage(
            GuardBreakSequence,
            HitReactionSlotName,
            HitReactionBlendInTime,
            HitReactionBlendOutTime,
            PlayRate,
            1,
            0.0f,
            0.0f);
    }
}

UAnimSequenceBase* URPGCharacterAnimInstance::ResolveHitReactionSequence(ERPGHitReactionStrength ReactionStrength, ERPGHitDirection HitDirection) const
{
    const bool bBackHit = HitDirection == ERPGHitDirection::Back;

    if (ReactionStrength == ERPGHitReactionStrength::GuardBreak)
    {
        return GuardBreakSequence;
    }

    if (ReactionStrength == ERPGHitReactionStrength::Heavy)
    {
        return bBackHit ? HitHeavyBackSequence : HitHeavyFrontSequence;
    }

    if (ReactionStrength == ERPGHitReactionStrength::Light)
    {
        return bBackHit ? HitLightBackSequence : HitLightFrontSequence;
    }

    return nullptr;
}















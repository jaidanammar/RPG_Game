#include "Data/RPGWeaponDataAssets.h"

UAnimationAsset* FRPGWeaponAnimationSet::GetAnimationForSlot(ERPGAnimationSlot Slot) const
{
    switch (Slot)
    {
    case ERPGAnimationSlot::Idle:
        return Idle;
    case ERPGAnimationSlot::Walk:
        return Walk;
    case ERPGAnimationSlot::Run:
        return Run;
    case ERPGAnimationSlot::Sprint:
        return Sprint;
    case ERPGAnimationSlot::CrouchIdle:
        return CrouchIdle;
    case ERPGAnimationSlot::CrouchWalk:
        return CrouchWalk;
    case ERPGAnimationSlot::JumpStart:
        return JumpStart;
    case ERPGAnimationSlot::JumpLoop:
        return JumpLoop;
    case ERPGAnimationSlot::JumpLand:
        return JumpLand;
    case ERPGAnimationSlot::FallLoop:
        return FallLoop;
    default:
        return nullptr;
    }
}

void FRPGWeaponAnimationSet::SetAnimationForSlot(ERPGAnimationSlot Slot, UAnimationAsset* Animation)
{
    switch (Slot)
    {
    case ERPGAnimationSlot::Idle:
        Idle = Animation;
        break;
    case ERPGAnimationSlot::Walk:
        Walk = Animation;
        break;
    case ERPGAnimationSlot::Run:
        Run = Animation;
        break;
    case ERPGAnimationSlot::Sprint:
        Sprint = Animation;
        break;
    case ERPGAnimationSlot::CrouchIdle:
        CrouchIdle = Animation;
        break;
    case ERPGAnimationSlot::CrouchWalk:
        CrouchWalk = Animation;
        break;
    case ERPGAnimationSlot::JumpStart:
        JumpStart = Animation;
        break;
    case ERPGAnimationSlot::JumpLoop:
        JumpLoop = Animation;
        break;
    case ERPGAnimationSlot::JumpLand:
        JumpLand = Animation;
        break;
    case ERPGAnimationSlot::FallLoop:
        FallLoop = Animation;
        break;
    default:
        break;
    }
}

FRPGWeaponCombatTuning URPGWeaponInstanceDataAsset::ResolveFinalTuning() const
{
    FRPGWeaponCombatTuning Result;

    if (WeaponTypeProfile)
    {
        Result = WeaponTypeProfile->BaseTuning;
    }

    Result.AttackDamageMultiplier = FMath::Max(0.01f, Result.AttackDamageMultiplier * AttackDamageMultiplierBonus);
    Result.AttackStaminaMultiplier = FMath::Max(0.01f, Result.AttackStaminaMultiplier * AttackStaminaMultiplierBonus);
    Result.EvasionStaminaMultiplier = FMath::Max(0.01f, Result.EvasionStaminaMultiplier * EvasionStaminaMultiplierBonus);
    Result.WalkSpeedMultiplier = FMath::Max(0.01f, Result.WalkSpeedMultiplier * WalkSpeedMultiplierBonus);
    return Result;
}

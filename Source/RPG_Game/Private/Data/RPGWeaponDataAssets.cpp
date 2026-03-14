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
    case ERPGAnimationSlot::GuardIdle:
        return GuardIdle;
    case ERPGAnimationSlot::GuardMove:
        return GuardMove;
    case ERPGAnimationSlot::RunStart:
        return RunStart;
    case ERPGAnimationSlot::RunStop:
        return RunStop;
    case ERPGAnimationSlot::JumpStart:
        return JumpStart;
    case ERPGAnimationSlot::JumpLoop:
        return JumpLoop;
    case ERPGAnimationSlot::JumpLand:
        return JumpLand;
    case ERPGAnimationSlot::FallLoop:
        return FallLoop;
    case ERPGAnimationSlot::GuardEnter:
        return GuardEnter;
    case ERPGAnimationSlot::GuardLoop:
        return GuardLoop;
    case ERPGAnimationSlot::GuardExit:
        return GuardExit;
    case ERPGAnimationSlot::Parry:
        return Parry;
    case ERPGAnimationSlot::HitLightFront:
        return HitLightFront;
    case ERPGAnimationSlot::HitLightBack:
        return HitLightBack;
    case ERPGAnimationSlot::HitHeavyFront:
        return HitHeavyFront;
    case ERPGAnimationSlot::HitHeavyBack:
        return HitHeavyBack;
    case ERPGAnimationSlot::GuardBreak:
        return GuardBreak;
    case ERPGAnimationSlot::Equip:
        return Equip;
    case ERPGAnimationSlot::Unequip:
        return Unequip;
    case ERPGAnimationSlot::Count:
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
    case ERPGAnimationSlot::GuardIdle:
        GuardIdle = Animation;
        break;
    case ERPGAnimationSlot::GuardMove:
        GuardMove = Animation;
        break;
    case ERPGAnimationSlot::RunStart:
        RunStart = Animation;
        break;
    case ERPGAnimationSlot::RunStop:
        RunStop = Animation;
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
    case ERPGAnimationSlot::GuardEnter:
        GuardEnter = Animation;
        break;
    case ERPGAnimationSlot::GuardLoop:
        GuardLoop = Animation;
        break;
    case ERPGAnimationSlot::GuardExit:
        GuardExit = Animation;
        break;
    case ERPGAnimationSlot::Parry:
        Parry = Animation;
        break;
    case ERPGAnimationSlot::HitLightFront:
        HitLightFront = Animation;
        break;
    case ERPGAnimationSlot::HitLightBack:
        HitLightBack = Animation;
        break;
    case ERPGAnimationSlot::HitHeavyFront:
        HitHeavyFront = Animation;
        break;
    case ERPGAnimationSlot::HitHeavyBack:
        HitHeavyBack = Animation;
        break;
    case ERPGAnimationSlot::GuardBreak:
        GuardBreak = Animation;
        break;
    case ERPGAnimationSlot::Equip:
        Equip = Animation;
        break;
    case ERPGAnimationSlot::Unequip:
        Unequip = Animation;
        break;
    case ERPGAnimationSlot::Count:
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




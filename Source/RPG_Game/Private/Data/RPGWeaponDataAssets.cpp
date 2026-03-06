#include "Data/RPGWeaponDataAssets.h"

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
    return Result;
}

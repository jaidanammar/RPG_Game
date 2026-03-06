#include "Components/WeaponLoadoutComponent.h"

#include "Components/AttackSystemComponent.h"
#include "Components/EvasionComponent.h"
#include "Data/RPGWeaponDataAssets.h"

UWeaponLoadoutComponent::UWeaponLoadoutComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UWeaponLoadoutComponent::BeginPlay()
{
    Super::BeginPlay();

    CacheCombatComponents();

    if (EquippedWeaponInstance)
    {
        EquipWeaponInstance(EquippedWeaponInstance, true);
    }
}

bool UWeaponLoadoutComponent::EquipWeaponInstance(URPGWeaponInstanceDataAsset* NewWeaponInstance, bool bResetCombo)
{
    if (!NewWeaponInstance || !NewWeaponInstance->WeaponTypeProfile)
    {
        return false;
    }

    CacheCombatComponents();

    EquippedWeaponInstance = NewWeaponInstance;
    ApplyTypeProfileInternal(NewWeaponInstance->WeaponTypeProfile, bResetCombo);

    const FRPGWeaponCombatTuning FinalTuning = NewWeaponInstance->ResolveFinalTuning();
    if (CachedAttackSystem.IsValid())
    {
        CachedAttackSystem->SetWeaponAttackTuning(FinalTuning.AttackDamageMultiplier, FinalTuning.AttackStaminaMultiplier);
    }

    if (CachedEvasion.IsValid())
    {
        CachedEvasion->SetWeaponEvasionTuning(FinalTuning.EvasionStaminaMultiplier);
    }

    return true;
}

bool UWeaponLoadoutComponent::ApplyWeaponTypeProfile(URPGWeaponTypeDataAsset* NewWeaponTypeProfile, bool bResetCombo)
{
    if (!NewWeaponTypeProfile)
    {
        return false;
    }

    CacheCombatComponents();
    ApplyTypeProfileInternal(NewWeaponTypeProfile, bResetCombo);

    if (CachedAttackSystem.IsValid())
    {
        CachedAttackSystem->SetWeaponAttackTuning(
            NewWeaponTypeProfile->BaseTuning.AttackDamageMultiplier,
            NewWeaponTypeProfile->BaseTuning.AttackStaminaMultiplier);
    }

    if (CachedEvasion.IsValid())
    {
        CachedEvasion->SetWeaponEvasionTuning(NewWeaponTypeProfile->BaseTuning.EvasionStaminaMultiplier);
    }

    return true;
}

void UWeaponLoadoutComponent::CacheCombatComponents()
{
    if (!GetOwner())
    {
        return;
    }

    CachedAttackSystem = GetOwner()->FindComponentByClass<UAttackSystemComponent>();
    CachedEvasion = GetOwner()->FindComponentByClass<UEvasionComponent>();
}

void UWeaponLoadoutComponent::ApplyTypeProfileInternal(const URPGWeaponTypeDataAsset* WeaponTypeProfile, bool bResetCombo) const
{
    if (!WeaponTypeProfile)
    {
        return;
    }

    if (CachedAttackSystem.IsValid() && WeaponTypeProfile->AttackMoveset)
    {
        CachedAttackSystem->ApplyAttackMoveset(WeaponTypeProfile->AttackMoveset, bResetCombo);
    }

    if (CachedEvasion.IsValid())
    {
        CachedEvasion->SetWeaponEvasionProfile(
            WeaponTypeProfile->DodgeDirectionalMontages,
            WeaponTypeProfile->DodgeMontage,
            WeaponTypeProfile->RollDirectionalMontages,
            WeaponTypeProfile->RollMontage,
            WeaponTypeProfile->bUseDirectionalDodgeMontages,
            WeaponTypeProfile->bUseDirectionalRollMontages);
    }
}

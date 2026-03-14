#include "Components/WeaponLoadoutComponent.h"

#include "Components/AttackSystemComponent.h"
#include "Components/EvasionComponent.h"
#include "Data/RPGLocomotionDataAsset.h"
#include "Data/RPGWeaponDataAssets.h"
#include "Components/LocomotionComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogRPGWeaponLoadout, Log, All);

namespace
{
FRPGEvasionDirectionalMontages MergeDirectionalMontages(
    const FRPGEvasionDirectionalMontages& Defaults,
    const FRPGEvasionDirectionalMontages& Overrides)
{
    FRPGEvasionDirectionalMontages Result = Defaults;

    if (Overrides.Forward)
    {
        Result.Forward = Overrides.Forward;
    }

    if (Overrides.ForwardRight)
    {
        Result.ForwardRight = Overrides.ForwardRight;
    }

    if (Overrides.Right)
    {
        Result.Right = Overrides.Right;
    }

    if (Overrides.BackwardRight)
    {
        Result.BackwardRight = Overrides.BackwardRight;
    }

    if (Overrides.Backward)
    {
        Result.Backward = Overrides.Backward;
    }

    if (Overrides.BackwardLeft)
    {
        Result.BackwardLeft = Overrides.BackwardLeft;
    }

    if (Overrides.Left)
    {
        Result.Left = Overrides.Left;
    }

    if (Overrides.ForwardLeft)
    {
        Result.ForwardLeft = Overrides.ForwardLeft;
    }

    return Result;
}
void MergeMissingAnimationSlots(FRPGWeaponAnimationSet& InOutTarget, const FRPGWeaponAnimationSet& Fallback)
{
    for (uint8 SlotIndex = 0; SlotIndex < static_cast<uint8>(ERPGAnimationSlot::Count); ++SlotIndex)
    {
        const ERPGAnimationSlot Slot = static_cast<ERPGAnimationSlot>(SlotIndex);
        if (!InOutTarget.GetAnimationForSlot(Slot))
        {
            InOutTarget.SetAnimationForSlot(Slot, Fallback.GetAnimationForSlot(Slot));
        }
    }
}

void NormalizeAnimationSet(FRPGWeaponAnimationSet& InOutAnimationSet)
{
    if (!InOutAnimationSet.FallLoop)
    {
        InOutAnimationSet.FallLoop = InOutAnimationSet.JumpLoop;
    }

    if (!InOutAnimationSet.JumpLoop)
    {
        InOutAnimationSet.JumpLoop = InOutAnimationSet.FallLoop;
    }
}

URPGWeaponTypeDataAsset* LoadDefaultUnarmedWeaponTypeProfile()
{
    static const TCHAR* UnarmedProfilePath = TEXT("/Game/Core/Combat/Weapons/Unarmed/Data/DA_WeaponType_Unarmed.DA_WeaponType_Unarmed");
    return LoadObject<URPGWeaponTypeDataAsset>(nullptr, UnarmedProfilePath);
}
} // namespace

UWeaponLoadoutComponent::UWeaponLoadoutComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UWeaponLoadoutComponent::BeginPlay()
{
    Super::BeginPlay();

    CacheCombatComponents();
    CacheDefaultsFromCombatComponents();
    ActiveAnimationSet = DefaultAnimationSet;
    NormalizeAnimationSet(ActiveAnimationSet);
    ActiveWeaponType = ERPGWeaponType::Unarmed;

    if (EquippedWeaponInstance)
    {
        const bool bEquipped = EquipWeaponInstance(EquippedWeaponInstance, true);
        UE_LOG(
            LogRPGWeaponLoadout,
            Log,
            TEXT("BeginPlay equip attempt. Owner=%s Instance=%s Result=%s ActiveWeaponType=%s"),
            *GetNameSafe(GetOwner()),
            *GetNameSafe(EquippedWeaponInstance),
            bEquipped ? TEXT("true") : TEXT("false"),
            *StaticEnum<ERPGWeaponType>()->GetNameStringByValue(static_cast<int64>(ActiveWeaponType)));

        if (!bEquipped)
        {
            UE_LOG(LogRPGWeaponLoadout, Warning, TEXT("BeginPlay equip failed. Falling back to unarmed defaults."));
            UnequipWeapon(true);
        }

        return;
    }

    UE_LOG(LogRPGWeaponLoadout, Log, TEXT("BeginPlay without equipped weapon instance. Using unarmed defaults."));
    UnequipWeapon(true);
}

bool UWeaponLoadoutComponent::EquipWeaponInstance(URPGWeaponInstanceDataAsset* NewWeaponInstance, bool bResetCombo)
{
    if (!NewWeaponInstance)
    {
        UE_LOG(LogRPGWeaponLoadout, Warning, TEXT("EquipWeaponInstance called with null instance. Switching to unarmed."));
        return UnequipWeapon(bResetCombo);
    }

    if (!NewWeaponInstance->WeaponTypeProfile)
    {
        UE_LOG(
            LogRPGWeaponLoadout,
            Warning,
            TEXT("EquipWeaponInstance failed: WeaponTypeProfile is null. Instance=%s"),
            *GetNameSafe(NewWeaponInstance));
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

    ApplyWalkSpeedMultiplier(FinalTuning.WalkSpeedMultiplier);

    UE_LOG(
        LogRPGWeaponLoadout,
        Log,
        TEXT("EquipWeaponInstance success. Instance=%s TypeProfile=%s ActiveWeaponType=%s"),
        *GetNameSafe(NewWeaponInstance),
        *GetNameSafe(NewWeaponInstance->WeaponTypeProfile),
        *StaticEnum<ERPGWeaponType>()->GetNameStringByValue(static_cast<int64>(ActiveWeaponType)));

    return true;
}

bool UWeaponLoadoutComponent::UnequipWeapon(bool bResetCombo)
{
    CacheCombatComponents();

    EquippedWeaponInstance = nullptr;
    ActiveWeaponType = ERPGWeaponType::Unarmed;

    URPGCombatMovesetDataAsset* UnarmedMovesetToApply = UnarmedAttackMoveset ? UnarmedAttackMoveset : DefaultAttackMoveset;
    if (CachedAttackSystem.IsValid() && UnarmedMovesetToApply)
    {
        CachedAttackSystem->ApplyAttackMoveset(UnarmedMovesetToApply, bResetCombo);
    }

    if (CachedAttackSystem.IsValid())
    {
        CachedAttackSystem->SetWeaponAttackTuning(1.0f, 1.0f);
    }

    if (CachedEvasion.IsValid())
    {
        CachedEvasion->SetWeaponEvasionProfile(
            DefaultDodgeDirectionalMontages,
            DefaultDodgeMontage,
            DefaultRollDirectionalMontages,
            DefaultRollMontage,
            bDefaultUseDirectionalDodgeMontages,
            bDefaultUseDirectionalRollMontages);
        CachedEvasion->SetWeaponEvasionTuning(1.0f);
    }

    ActiveAnimationSet = DefaultAnimationSet;
    NormalizeAnimationSet(ActiveAnimationSet);

    if (CachedLocomotion.IsValid())
    {
        CachedLocomotion->ResetToDefaultLocomotionData();
        CachedLocomotion->ClearSpeedMultiplier(TEXT("Weapon"));
    }
    else
    {
        ApplyWalkSpeedMultiplier(1.0f);
    }

    BroadcastWeaponAnimationChanges();

    return true;
}

bool UWeaponLoadoutComponent::ApplyWeaponTypeProfile(URPGWeaponTypeDataAsset* NewWeaponTypeProfile, bool bResetCombo)
{
    if (!NewWeaponTypeProfile)
    {
        return false;
    }

    CacheCombatComponents();
    EquippedWeaponInstance = nullptr;
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

    ApplyWalkSpeedMultiplier(NewWeaponTypeProfile->BaseTuning.WalkSpeedMultiplier);

    return true;
}

void UWeaponLoadoutComponent::SetDefaultAnimationSet(const FRPGWeaponAnimationSet& InDefaultAnimationSet)
{
    DefaultAnimationSet = InDefaultAnimationSet;
    NormalizeAnimationSet(DefaultAnimationSet);

    if (!EquippedWeaponInstance || !EquippedWeaponInstance->WeaponTypeProfile)
    {
        ActiveAnimationSet = DefaultAnimationSet;
    NormalizeAnimationSet(ActiveAnimationSet);
        BroadcastWeaponAnimationChanges();
        return;
    }

    RefreshActiveAnimationSet(EquippedWeaponInstance->WeaponTypeProfile);
    BroadcastWeaponAnimationChanges();
}

UAnimationAsset* UWeaponLoadoutComponent::ResolveAnimationForSlot(ERPGAnimationSlot Slot) const
{
    if (UAnimationAsset* ActiveAsset = ActiveAnimationSet.GetAnimationForSlot(Slot))
    {
        return ActiveAsset;
    }

    return DefaultAnimationSet.GetAnimationForSlot(Slot);
}

void UWeaponLoadoutComponent::CacheCombatComponents()
{
    if (!GetOwner())
    {
        return;
    }

    CachedAttackSystem = GetOwner()->FindComponentByClass<UAttackSystemComponent>();
    CachedEvasion = GetOwner()->FindComponentByClass<UEvasionComponent>();
    CachedLocomotion = GetOwner()->FindComponentByClass<ULocomotionComponent>();

    if (ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner()))
    {
        CachedMoveComp = OwnerCharacter->GetCharacterMovement();
    }
}

void UWeaponLoadoutComponent::CacheDefaultsFromCombatComponents()
{
    if (CachedAttackSystem.IsValid())
    {
        DefaultAttackMoveset = CachedAttackSystem->AttackMoveset;
    }

    if (CachedEvasion.IsValid())
    {
        DefaultDodgeDirectionalMontages = CachedEvasion->DodgeDirectionalMontages;
        DefaultDodgeMontage = CachedEvasion->DodgeMontage;
        bDefaultUseDirectionalDodgeMontages = CachedEvasion->bUseDirectionalDodgeMontages;

        DefaultRollDirectionalMontages = CachedEvasion->RollDirectionalMontages;
        DefaultRollMontage = CachedEvasion->RollMontage;
        bDefaultUseDirectionalRollMontages = CachedEvasion->bUseDirectionalRollMontages;
    }

    if (CachedMoveComp.IsValid())
    {
        BaseWalkSpeed = FMath::Max(0.0f, CachedMoveComp->MaxWalkSpeed);
    }

    if (URPGWeaponTypeDataAsset* DefaultUnarmedProfile = LoadDefaultUnarmedWeaponTypeProfile())
    {
        MergeMissingAnimationSlots(DefaultAnimationSet, DefaultUnarmedProfile->AnimationOverrides);

        if (DefaultUnarmedProfile->AttackMoveset)
        {
            UnarmedAttackMoveset = DefaultUnarmedProfile->AttackMoveset;
        }
    }

    NormalizeAnimationSet(DefaultAnimationSet);
}

void UWeaponLoadoutComponent::ApplyTypeProfileInternal(const URPGWeaponTypeDataAsset* WeaponTypeProfile, bool bResetCombo)
{
    if (!WeaponTypeProfile)
    {
        return;
    }

    ActiveWeaponType = WeaponTypeProfile->WeaponType;

    if (CachedAttackSystem.IsValid())
    {
        URPGCombatMovesetDataAsset* MovesetToApply = WeaponTypeProfile->AttackMoveset;
        if (!MovesetToApply)
        {
            MovesetToApply = DefaultAttackMoveset;
        }

        if (MovesetToApply)
        {
            CachedAttackSystem->ApplyAttackMoveset(MovesetToApply, bResetCombo);
        }
    }

    if (CachedEvasion.IsValid())
    {
        const FRPGEvasionDirectionalMontages DodgeMontages = WeaponTypeProfile->bOverrideDodgeProfile
            ? MergeDirectionalMontages(DefaultDodgeDirectionalMontages, WeaponTypeProfile->DodgeDirectionalMontages)
            : DefaultDodgeDirectionalMontages;

        UAnimMontage* DodgeMontageToUse = DefaultDodgeMontage;
        if (WeaponTypeProfile->bOverrideDodgeProfile && WeaponTypeProfile->DodgeMontage)
        {
            DodgeMontageToUse = WeaponTypeProfile->DodgeMontage;
        }

        const bool bUseDirectionalDodge = WeaponTypeProfile->bOverrideDodgeProfile
            ? WeaponTypeProfile->bUseDirectionalDodgeMontages
            : bDefaultUseDirectionalDodgeMontages;

        const FRPGEvasionDirectionalMontages RollMontages = WeaponTypeProfile->bOverrideRollProfile
            ? MergeDirectionalMontages(DefaultRollDirectionalMontages, WeaponTypeProfile->RollDirectionalMontages)
            : DefaultRollDirectionalMontages;

        UAnimMontage* RollMontageToUse = DefaultRollMontage;
        if (WeaponTypeProfile->bOverrideRollProfile && WeaponTypeProfile->RollMontage)
        {
            RollMontageToUse = WeaponTypeProfile->RollMontage;
        }

        const bool bUseDirectionalRoll = WeaponTypeProfile->bOverrideRollProfile
            ? WeaponTypeProfile->bUseDirectionalRollMontages
            : bDefaultUseDirectionalRollMontages;

        CachedEvasion->SetWeaponEvasionProfile(
            DodgeMontages,
            DodgeMontageToUse,
            RollMontages,
            RollMontageToUse,
            bUseDirectionalDodge,
            bUseDirectionalRoll);
    }

    RefreshActiveAnimationSet(WeaponTypeProfile);

    if (CachedLocomotion.IsValid())
    {
        if (WeaponTypeProfile->bOverrideLocomotionData && WeaponTypeProfile->LocomotionData)
        {
            CachedLocomotion->SetLocomotionData(WeaponTypeProfile->LocomotionData);
        }
        else
        {
            CachedLocomotion->ResetToDefaultLocomotionData();
        }
    }


    BroadcastWeaponAnimationChanges();
}

void UWeaponLoadoutComponent::RefreshActiveAnimationSet(const URPGWeaponTypeDataAsset* WeaponTypeProfile)
{
    ActiveAnimationSet = DefaultAnimationSet;
    NormalizeAnimationSet(ActiveAnimationSet);

    if (!WeaponTypeProfile)
    {
        return;
    }

    for (uint8 SlotIndex = 0; SlotIndex < static_cast<uint8>(ERPGAnimationSlot::Count); ++SlotIndex)
    {
        const ERPGAnimationSlot Slot = static_cast<ERPGAnimationSlot>(SlotIndex);
        if (UAnimationAsset* OverrideAsset = WeaponTypeProfile->AnimationOverrides.GetAnimationForSlot(Slot))
        {
            ActiveAnimationSet.SetAnimationForSlot(Slot, OverrideAsset);
        }
    }

    NormalizeAnimationSet(ActiveAnimationSet);
}

void UWeaponLoadoutComponent::ApplyWalkSpeedMultiplier(float InWalkSpeedMultiplier)
{
    const float SafeMultiplier = FMath::Max(0.01f, InWalkSpeedMultiplier);

    if (CachedLocomotion.IsValid())
    {
        CachedLocomotion->SetSpeedMultiplier(TEXT("Weapon"), SafeMultiplier);
        return;
    }

    if (!CachedMoveComp.IsValid())
    {
        return;
    }

    if (BaseWalkSpeed <= 0.0f)
    {
        BaseWalkSpeed = FMath::Max(0.0f, CachedMoveComp->MaxWalkSpeed);
    }

    CachedMoveComp->MaxWalkSpeed = BaseWalkSpeed * SafeMultiplier;
}

void UWeaponLoadoutComponent::BroadcastWeaponAnimationChanges()
{
    OnWeaponProfileChanged.Broadcast(ActiveWeaponType);
    OnWeaponAnimationSetChanged.Broadcast(ActiveAnimationSet);
}


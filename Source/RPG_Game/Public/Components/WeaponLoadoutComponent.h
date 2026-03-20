#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/RPGWeaponDataAssets.h"
#include "WeaponLoadoutComponent.generated.h"

class UAnimationAsset;
class UAttackSystemComponent;
class UCharacterMovementComponent;
class UEvasionComponent;
class UAnimMontage;
class ULocomotionComponent;
class URPGCombatMovesetDataAsset;
class URPGWeaponInstanceDataAsset;
class URPGWeaponTypeDataAsset;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponProfileChanged, ERPGWeaponType, NewWeaponType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponAnimationSetChanged, FRPGWeaponAnimationSet, NewAnimationSet);

UCLASS(ClassGroup=(RPG), BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class RPG_GAME_API UWeaponLoadoutComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UWeaponLoadoutComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
    TObjectPtr<URPGWeaponInstanceDataAsset> EquippedWeaponInstance = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
    TObjectPtr<URPGWeaponInstanceDataAsset> OwnedWeaponInstance = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Animation")
    FRPGWeaponAnimationSet DefaultAnimationSet;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|Animation")
    FRPGWeaponAnimationSet ActiveAnimationSet;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
    ERPGWeaponType ActiveWeaponType = ERPGWeaponType::Unarmed;

    UPROPERTY(BlueprintAssignable, Category = "Weapon|Events")
    FOnWeaponProfileChanged OnWeaponProfileChanged;

    UPROPERTY(BlueprintAssignable, Category = "Weapon|Events")
    FOnWeaponAnimationSetChanged OnWeaponAnimationSetChanged;

    UFUNCTION(BlueprintCallable, Category = "Weapon")
    bool EquipWeaponInstance(URPGWeaponInstanceDataAsset* NewWeaponInstance, bool bResetCombo = true);

    UFUNCTION(BlueprintCallable, Category = "Weapon")
    bool EquipOwnedWeapon(bool bResetCombo = true);

    UFUNCTION(BlueprintCallable, Category = "Weapon")
    bool UnequipWeapon(bool bResetCombo = true);

    UFUNCTION(BlueprintCallable, Category = "Weapon")
    void SetOwnedWeaponInstance(URPGWeaponInstanceDataAsset* NewOwnedWeaponInstance);

    UFUNCTION(BlueprintCallable, Category = "Weapon")
    bool ApplyWeaponTypeProfile(URPGWeaponTypeDataAsset* NewWeaponTypeProfile, bool bResetCombo = true);

    UFUNCTION(BlueprintCallable, Category = "Weapon|Animation")
    void SetDefaultAnimationSet(const FRPGWeaponAnimationSet& InDefaultAnimationSet);

    UFUNCTION(BlueprintPure, Category = "Weapon")
    bool HasOwnedWeapon() const { return OwnedWeaponInstance != nullptr; }

    UFUNCTION(BlueprintPure, Category = "Weapon|Animation")
    UAnimationAsset* ResolveAnimationForSlot(ERPGAnimationSlot Slot) const;

    UFUNCTION(BlueprintPure, Category = "Weapon|Animation")
    FRPGWeaponAnimationSet GetActiveAnimationSetSnapshot() const { return ActiveAnimationSet; }

protected:
    virtual void BeginPlay() override;

private:
    TWeakObjectPtr<UAttackSystemComponent> CachedAttackSystem;
    TWeakObjectPtr<UEvasionComponent> CachedEvasion;
    TWeakObjectPtr<UCharacterMovementComponent> CachedMoveComp;
    TWeakObjectPtr<ULocomotionComponent> CachedLocomotion;

    TObjectPtr<URPGCombatMovesetDataAsset> DefaultAttackMoveset = nullptr;
    TObjectPtr<URPGCombatMovesetDataAsset> UnarmedAttackMoveset = nullptr;
    FRPGEvasionDirectionalMontages DefaultDodgeDirectionalMontages;
    TObjectPtr<UAnimMontage> DefaultDodgeMontage = nullptr;
    bool bDefaultUseDirectionalDodgeMontages = true;
    FRPGEvasionDirectionalMontages DefaultRollDirectionalMontages;
    TObjectPtr<UAnimMontage> DefaultRollMontage = nullptr;
    bool bDefaultUseDirectionalRollMontages = true;
    float BaseWalkSpeed = 0.0f;

    void CacheCombatComponents();
    void CacheDefaultsFromCombatComponents();
    void ApplyTypeProfileInternal(const URPGWeaponTypeDataAsset* WeaponTypeProfile, bool bResetCombo);
    void RefreshActiveAnimationSet(const URPGWeaponTypeDataAsset* WeaponTypeProfile);
    void ApplyWalkSpeedMultiplier(float InWalkSpeedMultiplier);
    void BroadcastWeaponAnimationChanges();
};

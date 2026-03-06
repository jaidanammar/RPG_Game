#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WeaponLoadoutComponent.generated.h"

class UAttackSystemComponent;
class UEvasionComponent;
class URPGWeaponInstanceDataAsset;
class URPGWeaponTypeDataAsset;

UCLASS(ClassGroup=(RPG), BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class RPG_GAME_API UWeaponLoadoutComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UWeaponLoadoutComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
    TObjectPtr<URPGWeaponInstanceDataAsset> EquippedWeaponInstance = nullptr;

    UFUNCTION(BlueprintCallable, Category = "Weapon")
    bool EquipWeaponInstance(URPGWeaponInstanceDataAsset* NewWeaponInstance, bool bResetCombo = true);

    UFUNCTION(BlueprintCallable, Category = "Weapon")
    bool ApplyWeaponTypeProfile(URPGWeaponTypeDataAsset* NewWeaponTypeProfile, bool bResetCombo = true);

protected:
    virtual void BeginPlay() override;

private:
    TWeakObjectPtr<UAttackSystemComponent> CachedAttackSystem;
    TWeakObjectPtr<UEvasionComponent> CachedEvasion;

    void CacheCombatComponents();
    void ApplyTypeProfileInternal(const URPGWeaponTypeDataAsset* WeaponTypeProfile, bool bResetCombo) const;
};

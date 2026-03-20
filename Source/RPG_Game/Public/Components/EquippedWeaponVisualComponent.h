#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/RPGWeaponDataAssets.h"
#include "EquippedWeaponVisualComponent.generated.h"

class USkeletalMeshComponent;
class UStaticMeshComponent;
class UWeaponLoadoutComponent;

UCLASS(ClassGroup=(RPG), BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class RPG_GAME_API UEquippedWeaponVisualComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UEquippedWeaponVisualComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Visual")
    FName DefaultSocketName = TEXT("hand_r");

    UFUNCTION(BlueprintCallable, Category = "Weapon|Visual")
    void RefreshVisual();

    UFUNCTION(BlueprintPure, Category = "Weapon|Visual")
    UStaticMeshComponent* GetManagedVisualMesh() const { return ManagedWeaponMesh; }

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    UFUNCTION()
    void HandleWeaponProfileChanged(ERPGWeaponType NewWeaponType);

    UStaticMeshComponent* EnsureVisualMeshComponent();
    USkeletalMeshComponent* ResolveAttachParentMesh();

    UPROPERTY(Transient)
    TObjectPtr<UStaticMeshComponent> ManagedWeaponMesh = nullptr;

    TWeakObjectPtr<UWeaponLoadoutComponent> CachedWeaponLoadout;
    TWeakObjectPtr<USkeletalMeshComponent> CachedAttachParentMesh;
};

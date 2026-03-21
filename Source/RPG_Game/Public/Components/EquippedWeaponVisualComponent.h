#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/RPGWeaponDataAssets.h"
#include "EquippedWeaponVisualComponent.generated.h"

class USceneComponent;
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

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Carry")
    FName BackCarrySocketName = TEXT("spine_05");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Carry")
    FName HipCarrySocketName = TEXT("pelvis");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Carry")
    FTransform BackCarryAnchorTransform;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Carry")
    FTransform HipLeftCarryAnchorTransform;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Carry")
    FTransform HipRightCarryAnchorTransform;

    UFUNCTION(BlueprintCallable, Category = "Weapon|Visual")
    void RefreshVisual();

    UFUNCTION(BlueprintPure, Category = "Weapon|Visual")
    UStaticMeshComponent* GetManagedVisualMesh() const { return ManagedWeaponMesh; }

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    UFUNCTION()
    void HandleWeaponProfileChanged(ERPGWeaponType NewWeaponType);

    UStaticMeshComponent* EnsureVisualMeshComponent();
    USceneComponent* EnsureAttachmentAnchor(TObjectPtr<USceneComponent>& AnchorRef, const TCHAR* ComponentName);
    USkeletalMeshComponent* ResolvePrimaryAttachMesh() const;
    bool ResolveAttachmentName(const USkeletalMeshComponent* AttachMesh, FName RequestedName, FName FallbackName, FName& OutAttachmentName) const;
    USceneComponent* ResolveCarryAnchor(ERPGWeaponCarryAnchorType CarryAnchorType) const;
    void RefreshCarryAnchors(USkeletalMeshComponent* AttachMesh);

    UPROPERTY(Transient)
    TObjectPtr<UStaticMeshComponent> ManagedWeaponMesh = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<USceneComponent> DrawnWeaponAnchor = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<USceneComponent> BackCarryAnchor = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<USceneComponent> HipLeftCarryAnchor = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<USceneComponent> HipRightCarryAnchor = nullptr;

    TWeakObjectPtr<UWeaponLoadoutComponent> CachedWeaponLoadout;
};

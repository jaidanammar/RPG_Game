#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/RPGInteractable.h"
#include "RPGWeaponPickup.generated.h"

class UStaticMeshComponent;
class USphereComponent;
class UWeaponLoadoutComponent;
class URPGWeaponInstanceDataAsset;

UCLASS(Blueprintable)
class RPG_GAME_API ARPGWeaponPickup : public AActor, public IRPGInteractable
{
    GENERATED_BODY()

public:
    ARPGWeaponPickup();

    virtual void BeginPlay() override;
    virtual void OnConstruction(const FTransform& Transform) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pickup")
    TObjectPtr<USphereComponent> InteractionSphere;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pickup")
    TObjectPtr<UStaticMeshComponent> PickupMesh;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pickup")
    TObjectPtr<URPGWeaponInstanceDataAsset> WeaponInstance = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pickup")
    bool bAutoEquipOnPickup = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pickup")
    bool bDestroyOnPickup = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pickup")
    bool bSyncMeshFromWeaponData = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pickup")
    FText InteractionText;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pickup", meta=(ClampMin="0.0"))
    float InteractionRadius = 75.0f;

    UFUNCTION(BlueprintCallable, Category = "Pickup")
    void RefreshPickupPresentation();

    virtual bool CanInteract_Implementation(AActor* Interactor) override;
    virtual void Interact_Implementation(AActor* Interactor) override;
    virtual FText GetInteractionLabel_Implementation() override;

private:
    UWeaponLoadoutComponent* ResolveLoadoutComponent(AActor* Interactor) const;
};

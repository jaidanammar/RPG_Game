#include "Actors/RPGWeaponPickup.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WeaponLoadoutComponent.h"
#include "Data/RPGWeaponDataAssets.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"

namespace
{
AActor* ResolveInteractionActor(AActor* Interactor)
{
    if (AController* Controller = Cast<AController>(Interactor))
    {
        return Controller->GetPawn();
    }

    return Interactor;
}
}

ARPGWeaponPickup::ARPGWeaponPickup()
{
    PrimaryActorTick.bCanEverTick = false;

    InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
    SetRootComponent(InteractionSphere);
    InteractionSphere->InitSphereRadius(InteractionRadius);
    InteractionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    InteractionSphere->SetCollisionObjectType(ECC_WorldDynamic);
    InteractionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
    InteractionSphere->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    InteractionSphere->SetGenerateOverlapEvents(false);
    InteractionSphere->SetCanEverAffectNavigation(false);

    PickupMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PickupMesh"));
    PickupMesh->SetupAttachment(InteractionSphere);
    PickupMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    PickupMesh->SetGenerateOverlapEvents(false);
    PickupMesh->SetCanEverAffectNavigation(false);
}

void ARPGWeaponPickup::BeginPlay()
{
    Super::BeginPlay();
    RefreshPickupPresentation();
}

void ARPGWeaponPickup::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    RefreshPickupPresentation();
}

void ARPGWeaponPickup::RefreshPickupPresentation()
{
    if (InteractionSphere)
    {
        InteractionSphere->SetSphereRadius(InteractionRadius);
    }

    if (!PickupMesh || !bSyncMeshFromWeaponData)
    {
        return;
    }

    UStaticMesh* MeshToDisplay = WeaponInstance ? WeaponInstance->ResolvePickupMesh() : nullptr;
    PickupMesh->SetStaticMesh(MeshToDisplay);
    PickupMesh->SetRelativeTransform(WeaponInstance ? WeaponInstance->VisualDefinition.PickupRelativeTransform : FTransform::Identity);
}

bool ARPGWeaponPickup::CanInteract_Implementation(AActor* Interactor)
{
    return WeaponInstance != nullptr && ResolveLoadoutComponent(Interactor) != nullptr;
}

void ARPGWeaponPickup::Interact_Implementation(AActor* Interactor)
{
    UWeaponLoadoutComponent* LoadoutComponent = ResolveLoadoutComponent(Interactor);
    if (!LoadoutComponent || !WeaponInstance)
    {
        return;
    }

    LoadoutComponent->SetOwnedWeaponInstance(WeaponInstance);

    const bool bPickupApplied = bAutoEquipOnPickup
        ? LoadoutComponent->EquipWeaponInstance(WeaponInstance, true)
        : LoadoutComponent->UnequipWeapon(true);

    if (!bPickupApplied)
    {
        return;
    }

    if (bDestroyOnPickup)
    {
        Destroy();
        return;
    }

    SetActorEnableCollision(false);
    SetActorHiddenInGame(true);
}

FText ARPGWeaponPickup::GetInteractionLabel_Implementation()
{
    if (!InteractionText.IsEmpty())
    {
        return InteractionText;
    }

    if (WeaponInstance && !WeaponInstance->DisplayName.IsEmpty())
    {
        return FText::Format(
            NSLOCTEXT("RPGWeaponPickup", "PickupWeaponFormat", "pick up {0}"),
            WeaponInstance->DisplayName);
    }

    return NSLOCTEXT("RPGWeaponPickup", "PickupWeaponFallback", "pick up weapon");
}

UWeaponLoadoutComponent* ARPGWeaponPickup::ResolveLoadoutComponent(AActor* Interactor) const
{
    AActor* ActualInteractor = ResolveInteractionActor(Interactor);
    return ActualInteractor ? ActualInteractor->FindComponentByClass<UWeaponLoadoutComponent>() : nullptr;
}

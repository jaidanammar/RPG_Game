#include "Components/EquippedWeaponVisualComponent.h"

#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WeaponLoadoutComponent.h"
#include "GameFramework/Character.h"

UEquippedWeaponVisualComponent::UEquippedWeaponVisualComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UEquippedWeaponVisualComponent::BeginPlay()
{
    Super::BeginPlay();

    if (AActor* Owner = GetOwner())
    {
        CachedWeaponLoadout = Owner->FindComponentByClass<UWeaponLoadoutComponent>();
    }

    if (CachedWeaponLoadout.IsValid())
    {
        CachedWeaponLoadout->OnWeaponProfileChanged.AddDynamic(this, &UEquippedWeaponVisualComponent::HandleWeaponProfileChanged);
    }

    RefreshVisual();
}

void UEquippedWeaponVisualComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (CachedWeaponLoadout.IsValid())
    {
        CachedWeaponLoadout->OnWeaponProfileChanged.RemoveDynamic(this, &UEquippedWeaponVisualComponent::HandleWeaponProfileChanged);
    }

    Super::EndPlay(EndPlayReason);
}

void UEquippedWeaponVisualComponent::RefreshVisual()
{
    UStaticMeshComponent* VisualMesh = EnsureVisualMeshComponent();
    USkeletalMeshComponent* AttachParentMesh = ResolveAttachParentMesh();
    if (!VisualMesh || !AttachParentMesh)
    {
        return;
    }

    const URPGWeaponInstanceDataAsset* EquippedInstance = CachedWeaponLoadout.IsValid()
        ? CachedWeaponLoadout->EquippedWeaponInstance
        : nullptr;
    const URPGWeaponInstanceDataAsset* OwnedInstance = CachedWeaponLoadout.IsValid()
        ? CachedWeaponLoadout->OwnedWeaponInstance
        : nullptr;

    const URPGWeaponInstanceDataAsset* DisplayInstance = EquippedInstance ? EquippedInstance : OwnedInstance;
    UStaticMesh* MeshToDisplay = DisplayInstance ? DisplayInstance->ResolveEquippedMesh() : nullptr;
    if (!MeshToDisplay)
    {
        VisualMesh->SetStaticMesh(nullptr);
        VisualMesh->SetHiddenInGame(true);
        VisualMesh->SetVisibility(false);
        return;
    }

    const bool bDrawn = EquippedInstance != nullptr;
    FName SocketName = bDrawn ? DisplayInstance->ResolveEquippedSocketName() : DisplayInstance->ResolveSheathedSocketName();
    if (SocketName.IsNone())
    {
        SocketName = bDrawn ? DefaultSocketName : FName(TEXT("sheathe"));
    }

    const FTransform RelativeTransform = bDrawn
        ? DisplayInstance->VisualDefinition.EquippedRelativeTransform
        : DisplayInstance->VisualDefinition.SheathedRelativeTransform;

    VisualMesh->SetStaticMesh(MeshToDisplay);
    VisualMesh->AttachToComponent(AttachParentMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, SocketName);
    VisualMesh->SetRelativeTransform(RelativeTransform);
    VisualMesh->SetHiddenInGame(false);
    VisualMesh->SetVisibility(true);
}

void UEquippedWeaponVisualComponent::HandleWeaponProfileChanged(ERPGWeaponType NewWeaponType)
{
    RefreshVisual();
}

UStaticMeshComponent* UEquippedWeaponVisualComponent::EnsureVisualMeshComponent()
{
    if (ManagedWeaponMesh)
    {
        return ManagedWeaponMesh;
    }

    AActor* Owner = GetOwner();
    if (!Owner)
    {
        return nullptr;
    }

    ManagedWeaponMesh = NewObject<UStaticMeshComponent>(Owner, TEXT("EquippedWeaponVisualMesh"));
    if (!ManagedWeaponMesh)
    {
        return nullptr;
    }

    Owner->AddInstanceComponent(ManagedWeaponMesh);
    ManagedWeaponMesh->RegisterComponent();
    ManagedWeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ManagedWeaponMesh->SetGenerateOverlapEvents(false);
    ManagedWeaponMesh->SetCanEverAffectNavigation(false);
    ManagedWeaponMesh->SetHiddenInGame(true);
    ManagedWeaponMesh->SetVisibility(false);
    return ManagedWeaponMesh;
}

USkeletalMeshComponent* UEquippedWeaponVisualComponent::ResolveAttachParentMesh()
{
    if (CachedAttachParentMesh.IsValid())
    {
        return CachedAttachParentMesh.Get();
    }

    if (const ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner()))
    {
        CachedAttachParentMesh = OwnerCharacter->GetMesh();
    }
    else if (AActor* Owner = GetOwner())
    {
        CachedAttachParentMesh = Owner->FindComponentByClass<USkeletalMeshComponent>();
    }

    return CachedAttachParentMesh.Get();
}

#include "Components/EquippedWeaponVisualComponent.h"

#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WeaponLoadoutComponent.h"
#include "GameFramework/Character.h"

DEFINE_LOG_CATEGORY_STATIC(LogRPGEquippedWeaponVisual, Log, All);

namespace
{
bool MeshSupportsAttachmentName(const USkeletalMeshComponent* Mesh, FName AttachmentName)
{
    return IsValid(Mesh)
        && !AttachmentName.IsNone()
        && (Mesh->DoesSocketExist(AttachmentName) || Mesh->GetBoneIndex(AttachmentName) != INDEX_NONE);
}
} // namespace

UEquippedWeaponVisualComponent::UEquippedWeaponVisualComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 0.25f;

    BackCarryAnchorTransform = FTransform(FRotator(-90.0f, 180.0f, 0.0f), FVector(-10.0f, 10.0f, -6.0f));
    HipLeftCarryAnchorTransform = FTransform(FRotator(0.0f, -90.0f, 0.0f), FVector(0.0f, -12.0f, -8.0f));
    HipRightCarryAnchorTransform = FTransform(FRotator(0.0f, 90.0f, 0.0f), FVector(0.0f, 12.0f, -8.0f));
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

void UEquippedWeaponVisualComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!CachedWeaponLoadout.IsValid() || (!CachedWeaponLoadout->EquippedWeaponInstance && !CachedWeaponLoadout->OwnedWeaponInstance))
    {
        return;
    }

    RefreshVisual();
}

void UEquippedWeaponVisualComponent::RefreshVisual()
{
    UStaticMeshComponent* VisualMesh = EnsureVisualMeshComponent();
    USceneComponent* DrawnAnchor = EnsureAttachmentAnchor(DrawnWeaponAnchor, TEXT("DrawnWeaponAnchor"));
    USceneComponent* BackAnchor = EnsureAttachmentAnchor(BackCarryAnchor, TEXT("BackCarryAnchor"));
    USceneComponent* LeftHipAnchor = EnsureAttachmentAnchor(HipLeftCarryAnchor, TEXT("HipLeftCarryAnchor"));
    USceneComponent* RightHipAnchor = EnsureAttachmentAnchor(HipRightCarryAnchor, TEXT("HipRightCarryAnchor"));
    USkeletalMeshComponent* AttachMesh = ResolvePrimaryAttachMesh();
    if (!VisualMesh || !DrawnAnchor || !BackAnchor || !LeftHipAnchor || !RightHipAnchor || !AttachMesh)
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

    FName ResolvedDrawnAttachmentName = NAME_None;
    const FName RequestedDrawnAttachmentName = DisplayInstance ? DisplayInstance->ResolveEquippedSocketName() : DefaultSocketName;
    if (!ResolveAttachmentName(AttachMesh, RequestedDrawnAttachmentName, DefaultSocketName, ResolvedDrawnAttachmentName))
    {
        UE_LOG(
            LogRPGEquippedWeaponVisual,
            Warning,
            TEXT("Unable to resolve a drawn attachment point for %s on %s. Hiding weapon visual."),
            *GetNameSafe(DisplayInstance),
            *GetNameSafe(GetOwner()));

        VisualMesh->SetStaticMesh(nullptr);
        VisualMesh->SetHiddenInGame(true);
        VisualMesh->SetVisibility(false);
        return;
    }

    RefreshCarryAnchors(AttachMesh);

    DrawnAnchor->AttachToComponent(AttachMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, ResolvedDrawnAttachmentName);
    DrawnAnchor->SetRelativeTransform(FTransform::Identity);

    const bool bDrawn = EquippedInstance != nullptr;
    const ERPGWeaponCarryAnchorType CarryAnchorType = DisplayInstance
        ? DisplayInstance->ResolveSheathedCarryAnchor()
        : ERPGWeaponCarryAnchorType::Back;
    USceneComponent* ActiveAnchor = bDrawn ? DrawnAnchor : ResolveCarryAnchor(CarryAnchorType);
    if (!ActiveAnchor)
    {
        VisualMesh->SetStaticMesh(nullptr);
        VisualMesh->SetHiddenInGame(true);
        VisualMesh->SetVisibility(false);
        return;
    }

    const FTransform LocalWeaponTransform = bDrawn
        ? (DisplayInstance ? DisplayInstance->VisualDefinition.EquippedRelativeTransform : FTransform::Identity)
        : (DisplayInstance ? DisplayInstance->VisualDefinition.SheathedMeshRelativeTransform : FTransform::Identity);

    VisualMesh->SetStaticMesh(MeshToDisplay);
    VisualMesh->AttachToComponent(ActiveAnchor, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
    VisualMesh->SetRelativeTransform(LocalWeaponTransform);
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

USceneComponent* UEquippedWeaponVisualComponent::EnsureAttachmentAnchor(TObjectPtr<USceneComponent>& AnchorRef, const TCHAR* ComponentName)
{
    if (AnchorRef)
    {
        return AnchorRef;
    }

    AActor* Owner = GetOwner();
    if (!Owner)
    {
        return nullptr;
    }

    AnchorRef = NewObject<USceneComponent>(Owner, ComponentName);
    if (!AnchorRef)
    {
        return nullptr;
    }

    Owner->AddInstanceComponent(AnchorRef);
    AnchorRef->RegisterComponent();
    return AnchorRef;
}

USkeletalMeshComponent* UEquippedWeaponVisualComponent::ResolvePrimaryAttachMesh() const
{
    if (const ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner()))
    {
        return OwnerCharacter->GetMesh();
    }

    if (const AActor* Owner = GetOwner())
    {
        return Owner->FindComponentByClass<USkeletalMeshComponent>();
    }

    return nullptr;
}

bool UEquippedWeaponVisualComponent::ResolveAttachmentName(
    const USkeletalMeshComponent* AttachMesh,
    FName RequestedName,
    FName FallbackName,
    FName& OutAttachmentName) const
{
    OutAttachmentName = NAME_None;

    if (MeshSupportsAttachmentName(AttachMesh, RequestedName))
    {
        OutAttachmentName = RequestedName;
        return true;
    }

    if (MeshSupportsAttachmentName(AttachMesh, FallbackName))
    {
        OutAttachmentName = FallbackName;
        return true;
    }

    return false;
}

USceneComponent* UEquippedWeaponVisualComponent::ResolveCarryAnchor(ERPGWeaponCarryAnchorType CarryAnchorType) const
{
    switch (CarryAnchorType)
    {
    case ERPGWeaponCarryAnchorType::HipLeft:
        return HipLeftCarryAnchor;
    case ERPGWeaponCarryAnchorType::HipRight:
        return HipRightCarryAnchor;
    case ERPGWeaponCarryAnchorType::Back:
    default:
        return BackCarryAnchor;
    }
}

void UEquippedWeaponVisualComponent::RefreshCarryAnchors(USkeletalMeshComponent* AttachMesh)
{
    FName ResolvedBackSocketName = NAME_None;
    if (ResolveAttachmentName(AttachMesh, BackCarrySocketName, FName(TEXT("spine_03")), ResolvedBackSocketName) && BackCarryAnchor)
    {
        BackCarryAnchor->AttachToComponent(AttachMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, ResolvedBackSocketName);
        BackCarryAnchor->SetRelativeTransform(BackCarryAnchorTransform);
    }

    FName ResolvedHipSocketName = NAME_None;
    if (ResolveAttachmentName(AttachMesh, HipCarrySocketName, FName(TEXT("spine_01")), ResolvedHipSocketName))
    {
        if (HipLeftCarryAnchor)
        {
            HipLeftCarryAnchor->AttachToComponent(AttachMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, ResolvedHipSocketName);
            HipLeftCarryAnchor->SetRelativeTransform(HipLeftCarryAnchorTransform);
        }

        if (HipRightCarryAnchor)
        {
            HipRightCarryAnchor->AttachToComponent(AttachMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, ResolvedHipSocketName);
            HipRightCarryAnchor->SetRelativeTransform(HipRightCarryAnchorTransform);
        }
    }
}



#include "Components/LocomotionComponent.h"

#include "Data/RPGLocomotionDataAsset.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

ULocomotionComponent::ULocomotionComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void ULocomotionComponent::BeginPlay()
{
    Super::BeginPlay();

    EnsureCharacterAndMoveComp();
    ActiveLocomotionData = DefaultLocomotionData;

    if (CachedMoveComp.IsValid())
    {
        if (!bHasCapturedBaseFallbackWalkSpeed)
        {
            BaseFallbackWalkSpeed = FMath::Max(0.0f, CachedMoveComp->MaxWalkSpeed);
            bHasCapturedBaseFallbackWalkSpeed = true;
        }

        if (!bHasCapturedBaseFallbackJumpZVelocity)
        {
            BaseFallbackJumpZVelocity = FMath::Max(0.0f, CachedMoveComp->JumpZVelocity);
            bHasCapturedBaseFallbackJumpZVelocity = true;
        }
    }

    RefreshMovementSettings();
}

void ULocomotionComponent::SetLocomotionData(URPGLocomotionDataAsset* NewData)
{
    ActiveLocomotionData = NewData;
    RefreshMovementSettings();
    OnLocomotionProfileChanged.Broadcast();
}

void ULocomotionComponent::ResetToDefaultLocomotionData()
{
    ActiveLocomotionData = DefaultLocomotionData;
    RefreshMovementSettings();
    OnLocomotionProfileChanged.Broadcast();
}

void ULocomotionComponent::SetDesiredGait(ERPGLocomotionGait NewGait)
{
    DesiredGait = NewGait;
    RefreshMovementSettings();
}

void ULocomotionComponent::SetSpeedMultiplier(FName Source, float Multiplier)
{
    if (Source.IsNone())
    {
        return;
    }

    SpeedMultipliersBySource.Add(Source, FMath::Max(0.0f, Multiplier));
    RefreshMovementSettings();
}

void ULocomotionComponent::ClearSpeedMultiplier(FName Source)
{
    if (Source.IsNone())
    {
        return;
    }

    if (SpeedMultipliersBySource.Remove(Source) > 0)
    {
        RefreshMovementSettings();
    }
}

void ULocomotionComponent::SetCapabilityAllowed(FName Source, ERPGMovementCapability Capability, bool bAllowed)
{
    if (Source.IsNone())
    {
        return;
    }

    TMap<FName, bool>& CapabilityMap = CapabilityOverridesBySource.FindOrAdd(Capability);
    CapabilityMap.Add(Source, bAllowed);
    RefreshMovementSettings();
}

void ULocomotionComponent::ClearCapabilityOverride(FName Source, ERPGMovementCapability Capability)
{
    if (Source.IsNone())
    {
        return;
    }

    if (TMap<FName, bool>* CapabilityMap = CapabilityOverridesBySource.Find(Capability))
    {
        if (CapabilityMap->Remove(Source) > 0)
        {
            RefreshMovementSettings();
        }

        if (CapabilityMap->Num() == 0)
        {
            CapabilityOverridesBySource.Remove(Capability);
        }
    }
}

bool ULocomotionComponent::IsCapabilityAllowed(ERPGMovementCapability Capability) const
{
    return ResolveCapabilityAllowed(Capability);
}

float ULocomotionComponent::GetResolvedSpeedMultiplier() const
{
    float FinalMultiplier = 1.0f;
    for (const TPair<FName, float>& It : SpeedMultipliersBySource)
    {
        FinalMultiplier *= FMath::Max(0.0f, It.Value);
    }

    return FMath::Max(0.0f, FinalMultiplier);
}

float ULocomotionComponent::GetResolvedMaxWalkSpeed() const
{
    ERPGLocomotionGait ResolvedGait = DesiredGait;
    if (ResolvedGait == ERPGLocomotionGait::Sprint && !ResolveCapabilityAllowed(ERPGMovementCapability::Sprint))
    {
        ResolvedGait = ResolveCapabilityAllowed(ERPGMovementCapability::Run) ? ERPGLocomotionGait::Run : ERPGLocomotionGait::Walk;
    }

    if (ResolvedGait == ERPGLocomotionGait::Run && !ResolveCapabilityAllowed(ERPGMovementCapability::Run))
    {
        ResolvedGait = ERPGLocomotionGait::Walk;
    }

    const float BaseSpeed = ResolveCapabilityAllowed(ERPGMovementCapability::Walk) ? GetBaseSpeedForGait(ResolvedGait) : 0.0f;
    return BaseSpeed * GetResolvedSpeedMultiplier();
}

void ULocomotionComponent::RefreshMovementSettings()
{
    EnsureCharacterAndMoveComp();

    if (!CachedMoveComp.IsValid())
    {
        return;
    }

    URPGLocomotionDataAsset* EffectiveData = ActiveLocomotionData ? ActiveLocomotionData : DefaultLocomotionData;
    float JumpZVelocity = bHasCapturedBaseFallbackJumpZVelocity ? BaseFallbackJumpZVelocity : 0.0f;
    if (EffectiveData)
    {
        JumpZVelocity = EffectiveData->JumpZVelocity;
        CachedMoveComp->AirControl = EffectiveData->AirControl;
        CachedMoveComp->MaxAcceleration = EffectiveData->MaxAcceleration;
        CachedMoveComp->BrakingDecelerationWalking = EffectiveData->BrakingDecelerationWalking;
        CachedMoveComp->MaxWalkSpeedCrouched = EffectiveData->Speeds.CrouchSpeed * GetResolvedSpeedMultiplier();
    }

    if (!ResolveCapabilityAllowed(ERPGMovementCapability::Jump))
    {
        JumpZVelocity = 0.0f;
    }

    CachedMoveComp->JumpZVelocity = JumpZVelocity;
    CachedMoveComp->MaxWalkSpeed = GetResolvedMaxWalkSpeed();

    if (CachedCharacter.IsValid())
    {
        const bool bCanCrouch = ResolveCapabilityAllowed(ERPGMovementCapability::Crouch);
        CachedMoveComp->GetNavAgentPropertiesRef().bCanCrouch = bCanCrouch;

        if (!bCanCrouch && CachedCharacter->bIsCrouched)
        {
            CachedCharacter->UnCrouch();
        }
    }
}

float ULocomotionComponent::GetBaseSpeedForGait(ERPGLocomotionGait Gait) const
{
    const URPGLocomotionDataAsset* EffectiveData = ActiveLocomotionData ? ActiveLocomotionData : DefaultLocomotionData;
    if (!EffectiveData)
    {
        return bHasCapturedBaseFallbackWalkSpeed ? BaseFallbackWalkSpeed : 0.0f;
    }

    switch (Gait)
    {
    case ERPGLocomotionGait::Walk:
        return EffectiveData->Speeds.WalkSpeed;
    case ERPGLocomotionGait::Run:
        return EffectiveData->Speeds.RunSpeed;
    case ERPGLocomotionGait::Sprint:
        return EffectiveData->Speeds.SprintSpeed;
    default:
        return EffectiveData->Speeds.RunSpeed;
    }
}

bool ULocomotionComponent::ResolveCapabilityAllowed(ERPGMovementCapability Capability) const
{
    const TMap<FName, bool>* CapabilityMap = CapabilityOverridesBySource.Find(Capability);
    if (!CapabilityMap)
    {
        return true;
    }

    for (const TPair<FName, bool>& It : *CapabilityMap)
    {
        if (!It.Value)
        {
            return false;
        }
    }

    return true;
}

void ULocomotionComponent::EnsureCharacterAndMoveComp()
{
    if (!CachedCharacter.IsValid())
    {
        CachedCharacter = Cast<ACharacter>(GetOwner());
    }

    if (!CachedMoveComp.IsValid() && CachedCharacter.IsValid())
    {
        CachedMoveComp = CachedCharacter->GetCharacterMovement();
    }
}






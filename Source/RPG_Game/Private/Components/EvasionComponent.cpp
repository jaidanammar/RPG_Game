#include "Components/EvasionComponent.h"

#include "Animation/AnimInstance.h"
#include "Components/CombatStateComponent.h"
#include "Components/PlayerStatsComponent.h"
#include "Components/TargetLockComponent.h"
#include "Components/LocomotionComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "TimerManager.h"

UAnimMontage* FRPGEvasionDirectionalMontages::SelectMontage(ERPGEvasionDirection Direction) const
{
    switch (Direction)
    {
    case ERPGEvasionDirection::Forward:
        return Forward;
    case ERPGEvasionDirection::ForwardRight:
        return ForwardRight;
    case ERPGEvasionDirection::Right:
        return Right;
    case ERPGEvasionDirection::BackwardRight:
        return BackwardRight;
    case ERPGEvasionDirection::Backward:
        return Backward;
    case ERPGEvasionDirection::BackwardLeft:
        return BackwardLeft;
    case ERPGEvasionDirection::Left:
        return Left;
    case ERPGEvasionDirection::ForwardLeft:
        return ForwardLeft;
    default:
        return nullptr;
    }
}

UEvasionComponent::UEvasionComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UEvasionComponent::BeginPlay()
{
    Super::BeginPlay();

    CachedCharacter = Cast<ACharacter>(GetOwner());
    CachedStats = GetOwner() ? GetOwner()->FindComponentByClass<UPlayerStatsComponent>() : nullptr;
    CachedCombatState = GetOwner() ? GetOwner()->FindComponentByClass<UCombatStateComponent>() : nullptr;
    CachedSpringArm = GetOwner() ? GetOwner()->FindComponentByClass<USpringArmComponent>() : nullptr;
    CachedTargetLock = GetOwner() ? GetOwner()->FindComponentByClass<UTargetLockComponent>() : nullptr;
    CachedLocomotion = GetOwner() ? GetOwner()->FindComponentByClass<ULocomotionComponent>() : nullptr;
}

bool UEvasionComponent::StartDodge()
{
    return TryStartEvasion(ERPGEvasionType::Dodge);
}

bool UEvasionComponent::StartCombatRoll()
{
    return TryStartEvasion(ERPGEvasionType::Roll);
}

void UEvasionComponent::HandleDodgeInput()
{
    StartDodge();
}

void UEvasionComponent::HandleCombatRollInput()
{
    StartCombatRoll();
}

bool UEvasionComponent::CanCombatRollByWeight() const
{
    return !bUseWeightRestrictionForRoll || CurrentEquipWeight <= MaxWeightForRoll;
}

void UEvasionComponent::SetCurrentEquipWeight(float NewWeight)
{
    CurrentEquipWeight = FMath::Max(0.0f, NewWeight);
}

void UEvasionComponent::SetWeaponEvasionProfile(
    const FRPGEvasionDirectionalMontages& InDodgeDirectionalMontages,
    UAnimMontage* InDodgeMontage,
    const FRPGEvasionDirectionalMontages& InRollDirectionalMontages,
    UAnimMontage* InRollMontage,
    bool bInUseDirectionalDodgeMontages,
    bool bInUseDirectionalRollMontages)
{
    DodgeDirectionalMontages = InDodgeDirectionalMontages;
    DodgeMontage = InDodgeMontage;
    RollDirectionalMontages = InRollDirectionalMontages;
    RollMontage = InRollMontage;
    bUseDirectionalDodgeMontages = bInUseDirectionalDodgeMontages;
    bUseDirectionalRollMontages = bInUseDirectionalRollMontages;
}

void UEvasionComponent::SetWeaponEvasionTuning(float InStaminaMultiplier)
{
    WeaponEvasionStaminaMultiplier = FMath::Max(0.01f, InStaminaMultiplier);
}

bool UEvasionComponent::TryStartEvasion(ERPGEvasionType EvasionType)
{
    if (!CachedCharacter.IsValid())
    {
        BroadcastFail(EvasionType, TEXT("No owning Character found"));
        return false;
    }

    if (bIsEvading)
    {
        BroadcastFail(EvasionType, TEXT("Already evading"));
        return false;
    }

    if (EvasionType == ERPGEvasionType::Dodge && !bCanDodge)
    {
        BroadcastFail(EvasionType, TEXT("Dodge disabled"));
        return false;
    }

    if (EvasionType == ERPGEvasionType::Roll)
    {
        if (!bCanRoll)
        {
            BroadcastFail(EvasionType, TEXT("Combat roll disabled"));
            return false;
        }

        if (!CanCombatRollByWeight())
        {
            BroadcastFail(EvasionType, TEXT("Too heavy to roll"));
            return false;
        }
    }

    if (EvasionType == ERPGEvasionType::Dodge && bDodgeOnCooldown)
    {
        BroadcastFail(EvasionType, TEXT("Dodge is on cooldown"));
        return false;
    }

    if (EvasionType == ERPGEvasionType::Roll && bRollOnCooldown)
    {
        BroadcastFail(EvasionType, TEXT("Combat roll is on cooldown"));
        return false;
    }

    if (IsBlockedByCombatState())
    {
        BroadcastFail(EvasionType, TEXT("Blocked by combat state"));
        return false;
    }

    if (CachedLocomotion.IsValid())
    {
        const ERPGMovementCapability RequiredCapability = EvasionType == ERPGEvasionType::Roll
            ? ERPGMovementCapability::CombatRoll
            : ERPGMovementCapability::Dodge;

        if (!CachedLocomotion->IsCapabilityAllowed(RequiredCapability))
        {
            BroadcastFail(EvasionType, TEXT("Blocked by locomotion capability"));
            return false;
        }
    }

    const float BaseStaminaCost = EvasionType == ERPGEvasionType::Dodge ? DodgeStaminaCost : RollStaminaCost;
    const float StaminaCost = BaseStaminaCost * WeaponEvasionStaminaMultiplier;
    if (!ConsumeStamina(StaminaCost))
    {
        BroadcastFail(EvasionType, TEXT("Not enough stamina"));
        return false;
    }

    if (bCancelGuardOnEvasion && CachedCombatState.IsValid() && CachedCombatState->IsInState(ERPGCombatState::Guard))
    {
        CachedCombatState->StopGuard();
    }

    const FVector Direction = ResolveEvasionDirection();
    const ERPGEvasionDirection DirectionType = ResolveDirectionType(Direction);
    LastEvasionDirection = DirectionType;

    UAnimMontage* EvasionMontage = ResolveEvasionMontage(EvasionType, DirectionType);

    ApplyMovementRotationOverrideForEvasion();
    ApplyAnimRootMotionOverrideForEvasion();
    ApplyEvasionMovement(EvasionType, Direction);
    PlayEvasionMontage(EvasionMontage);

    bIsEvading = true;
    ActiveEvasionType = EvasionType;

    const float Duration = EvasionType == ERPGEvasionType::Dodge ? DodgeDuration : RollDuration;
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(EvasionEndTimerHandle);
        GetWorld()->GetTimerManager().SetTimer(
            EvasionEndTimerHandle,
            this,
            &UEvasionComponent::FinishActiveEvasion,
            FMath::Max(0.01f, Duration),
            false);
    }

    const float InvulnDelay = EvasionType == ERPGEvasionType::Dodge ? DodgeInvulnerabilityStartDelay : RollInvulnerabilityStartDelay;
    const float InvulnDuration = EvasionType == ERPGEvasionType::Dodge ? DodgeInvulnerabilityDuration : RollInvulnerabilityDuration;
    StartInvulnerabilityWindow(InvulnDelay, InvulnDuration);

    StartCooldown(EvasionType);
    OnEvasionStarted.Broadcast(EvasionType);
    return true;
}

bool UEvasionComponent::IsBlockedByCombatState() const
{
    if (!CachedCombatState.IsValid())
    {
        return false;
    }

    if (CachedCombatState->IsInState(ERPGCombatState::Dead)
        || CachedCombatState->IsInState(ERPGCombatState::Hitstun))
    {
        return true;
    }

    const bool bInAttack = CachedCombatState->IsInState(ERPGCombatState::AttackStartup)
        || CachedCombatState->IsInState(ERPGCombatState::AttackActive)
        || CachedCombatState->IsInState(ERPGCombatState::AttackRecovery);

    return bInAttack && !bAllowEvasionDuringAttack;
}

bool UEvasionComponent::ConsumeStamina(float Cost)
{
    if (!CachedStats.IsValid() || Cost <= 0.0f)
    {
        return true;
    }

    if (CachedStats->CurrentStamina < Cost)
    {
        return false;
    }

    CachedStats->DecreaseStamina(Cost);
    return true;
}

FVector UEvasionComponent::ResolveEvasionDirection() const
{
    if (!CachedCharacter.IsValid())
    {
        return -FVector::ForwardVector;
    }

    FVector Direction = FVector::ZeroVector;

    if (const UCharacterMovementComponent* MoveComp = CachedCharacter->GetCharacterMovement())
    {
        Direction = MoveComp->GetCurrentAcceleration();
    }

    if (Direction.IsNearlyZero())
    {
        Direction = CachedCharacter->GetPendingMovementInputVector();
    }

    Direction.Z = 0.0f;
    if (!Direction.IsNearlyZero())
    {
        return Direction.GetSafeNormal();
    }

    if (CachedTargetLock.IsValid() && CachedTargetLock->IsLockedOn())
    {
        FVector AwayFromTarget = CachedCharacter->GetActorLocation() - CachedTargetLock->GetLockTargetLocation();
        AwayFromTarget.Z = 0.0f;
        if (!AwayFromTarget.IsNearlyZero())
        {
            return AwayFromTarget.GetSafeNormal();
        }
    }

    return -CachedCharacter->GetActorForwardVector().GetSafeNormal2D();
}

ERPGEvasionDirection UEvasionComponent::ResolveDirectionType(const FVector& WorldDirection) const
{
    if (!CachedCharacter.IsValid())
    {
        return ERPGEvasionDirection::Forward;
    }

    const FVector Forward = CachedCharacter->GetActorForwardVector().GetSafeNormal2D();
    const FVector Right = CachedCharacter->GetActorRightVector().GetSafeNormal2D();
    const FVector Dir = WorldDirection.GetSafeNormal2D();

    const float ForwardDot = FVector::DotProduct(Dir, Forward);
    const float RightDot = FVector::DotProduct(Dir, Right);

    const float AngleRadians = FMath::Atan2(RightDot, ForwardDot);
    float AngleDegrees = FMath::RadiansToDegrees(AngleRadians);
    if (AngleDegrees < 0.0f)
    {
        AngleDegrees += 360.0f;
    }

    const int32 Sector = FMath::RoundToInt(AngleDegrees / 45.0f) % 8;

    switch (Sector)
    {
    case 0:
        return ERPGEvasionDirection::Forward;
    case 1:
        return ERPGEvasionDirection::ForwardRight;
    case 2:
        return ERPGEvasionDirection::Right;
    case 3:
        return ERPGEvasionDirection::BackwardRight;
    case 4:
        return ERPGEvasionDirection::Backward;
    case 5:
        return ERPGEvasionDirection::BackwardLeft;
    case 6:
        return ERPGEvasionDirection::Left;
    case 7:
    default:
        return ERPGEvasionDirection::ForwardLeft;
    }
}

void UEvasionComponent::ApplyEvasionMovement(ERPGEvasionType EvasionType, const FVector& Direction) const
{
    if (!CachedCharacter.IsValid())
    {
        return;
    }

    const float Distance = EvasionType == ERPGEvasionType::Dodge ? DodgeDistance : RollDistance;
    const float Duration = EvasionType == ERPGEvasionType::Dodge ? DodgeDuration : RollDuration;
    const float Speed = Distance / FMath::Max(0.01f, Duration);

    CachedCharacter->LaunchCharacter(Direction * Speed, true, false);

    if (bRotateActorTowardEvasionDirection)
    {
        const FRotator FacingRotation(0.0f, Direction.Rotation().Yaw, 0.0f);
        CachedCharacter->SetActorRotation(FacingRotation);
    }
}

UAnimMontage* UEvasionComponent::ResolveEvasionMontage(ERPGEvasionType EvasionType, ERPGEvasionDirection DirectionType) const
{
    if (EvasionType == ERPGEvasionType::Dodge)
    {
        if (bUseDirectionalDodgeMontages)
        {
            if (UAnimMontage* DirectionalMontage = DodgeDirectionalMontages.SelectMontage(DirectionType))
            {
                return DirectionalMontage;
            }
        }

        return DodgeMontage;
    }

    if (bUseDirectionalRollMontages)
    {
        if (UAnimMontage* DirectionalMontage = RollDirectionalMontages.SelectMontage(DirectionType))
        {
            return DirectionalMontage;
        }
    }

    return RollMontage;
}

void UEvasionComponent::PlayEvasionMontage(UAnimMontage* Montage) const
{
    if (!CachedCharacter.IsValid() || !CachedCharacter->GetMesh() || !Montage)
    {
        return;
    }

    UAnimInstance* AnimInstance = CachedCharacter->GetMesh()->GetAnimInstance();
    if (!AnimInstance)
    {
        return;
    }

    AnimInstance->Montage_Play(Montage);
}

void UEvasionComponent::StartCooldown(ERPGEvasionType EvasionType)
{
    if (!GetWorld())
    {
        return;
    }

    if (EvasionType == ERPGEvasionType::Dodge)
    {
        bDodgeOnCooldown = true;
        GetWorld()->GetTimerManager().ClearTimer(DodgeCooldownTimerHandle);
        GetWorld()->GetTimerManager().SetTimer(
            DodgeCooldownTimerHandle,
            this,
            &UEvasionComponent::ClearDodgeCooldown,
            FMath::Max(0.01f, DodgeCooldown),
            false);
        return;
    }

    bRollOnCooldown = true;
    GetWorld()->GetTimerManager().ClearTimer(RollCooldownTimerHandle);
    GetWorld()->GetTimerManager().SetTimer(
        RollCooldownTimerHandle,
        this,
        &UEvasionComponent::ClearRollCooldown,
        FMath::Max(0.01f, RollCooldown),
        false);
}

void UEvasionComponent::StartInvulnerabilityWindow(float StartDelay, float Duration)
{
    if (!CachedStats.IsValid() || Duration <= 0.0f || !GetWorld())
    {
        return;
    }

    GetWorld()->GetTimerManager().ClearTimer(InvulnerabilityStartTimerHandle);
    GetWorld()->GetTimerManager().ClearTimer(InvulnerabilityEndTimerHandle);

    if (StartDelay <= 0.0f)
    {
        BeginInvulnerability();
        GetWorld()->GetTimerManager().SetTimer(
            InvulnerabilityEndTimerHandle,
            this,
            &UEvasionComponent::EndInvulnerability,
            Duration,
            false);
        return;
    }

    GetWorld()->GetTimerManager().SetTimer(
        InvulnerabilityStartTimerHandle,
        FTimerDelegate::CreateWeakLambda(this, [this, Duration]()
        {
            BeginInvulnerability();
            if (GetWorld())
            {
                GetWorld()->GetTimerManager().SetTimer(
                    InvulnerabilityEndTimerHandle,
                    this,
                    &UEvasionComponent::EndInvulnerability,
                    Duration,
                    false);
            }
        }),
        StartDelay,
        false);
}

void UEvasionComponent::ApplyCameraLagOverrideForEvasion(ERPGEvasionType EvasionType)
{
    if (EvasionType != ERPGEvasionType::Dodge || !bOverrideCameraLagDuringDodge || !CachedSpringArm.IsValid())
    {
        return;
    }

    USpringArmComponent* SpringArm = CachedSpringArm.Get();
    if (!SpringArm)
    {
        return;
    }

    bSavedCameraLagEnabled = SpringArm->bEnableCameraLag;
    SavedCameraLagSpeed = SpringArm->CameraLagSpeed;

    if (bDisableCameraLagDuringDodge)
    {
        SpringArm->bEnableCameraLag = false;
    }
    else
    {
        SpringArm->bEnableCameraLag = true;
        SpringArm->CameraLagSpeed = FMath::Max(1.0f, DodgeCameraLagSpeedOverride);
    }

    bCameraLagOverrideActive = true;
}

void UEvasionComponent::RestoreCameraLagOverride()
{
    if (!bCameraLagOverrideActive || !CachedSpringArm.IsValid())
    {
        return;
    }

    if (USpringArmComponent* SpringArm = CachedSpringArm.Get())
    {
        SpringArm->bEnableCameraLag = bSavedCameraLagEnabled;
        SpringArm->CameraLagSpeed = SavedCameraLagSpeed;
    }

    bCameraLagOverrideActive = false;
}

void UEvasionComponent::ApplyMovementRotationOverrideForEvasion()
{
    if (!CachedCharacter.IsValid() || !bForceOrientRotationToMovementDuringEvasion)
    {
        return;
    }

    if (UCharacterMovementComponent* MoveComp = CachedCharacter->GetCharacterMovement())
    {
        bSavedOrientRotationToMovement = MoveComp->bOrientRotationToMovement;
        bSavedUseControllerDesiredRotation = MoveComp->bUseControllerDesiredRotation;

        MoveComp->bOrientRotationToMovement = true;
        MoveComp->bUseControllerDesiredRotation = false;
        bMovementRotationOverrideActive = true;
    }
}

void UEvasionComponent::RestoreMovementRotationOverride()
{
    if (!bMovementRotationOverrideActive || !CachedCharacter.IsValid())
    {
        return;
    }

    if (UCharacterMovementComponent* MoveComp = CachedCharacter->GetCharacterMovement())
    {
        MoveComp->bOrientRotationToMovement = bSavedOrientRotationToMovement;
        MoveComp->bUseControllerDesiredRotation = bSavedUseControllerDesiredRotation;
    }

    bMovementRotationOverrideActive = false;
}

void UEvasionComponent::ApplyAnimRootMotionOverrideForEvasion()
{
    if (!bDisableMontageRootMotionDuringEvasion || bAnimRootMotionOverrideActive || !CachedCharacter.IsValid() || !CachedCharacter->GetMesh())
    {
        return;
    }

    if (UAnimInstance* AnimInstance = CachedCharacter->GetMesh()->GetAnimInstance())
    {
        SavedAnimRootMotionMode = static_cast<uint8>(AnimInstance->RootMotionMode);
        AnimInstance->SetRootMotionMode(ERootMotionMode::NoRootMotionExtraction);
        bAnimRootMotionOverrideActive = true;
    }
}

void UEvasionComponent::RestoreAnimRootMotionOverrideForEvasion()
{
    if (!bAnimRootMotionOverrideActive || !CachedCharacter.IsValid() || !CachedCharacter->GetMesh())
    {
        return;
    }

    if (UAnimInstance* AnimInstance = CachedCharacter->GetMesh()->GetAnimInstance())
    {
        AnimInstance->SetRootMotionMode(static_cast<ERootMotionMode::Type>(SavedAnimRootMotionMode));
    }

    bAnimRootMotionOverrideActive = false;
}

void UEvasionComponent::CancelActiveInvulnerability()
{
    InvulnerabilityRefCount = 0;

    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(InvulnerabilityStartTimerHandle);
        GetWorld()->GetTimerManager().ClearTimer(InvulnerabilityEndTimerHandle);
    }

    if (CachedStats.IsValid())
    {
        CachedStats->bIsInvulnerable = false;
        OnInvulnerabilityChanged.Broadcast(false);
    }
}
void UEvasionComponent::BeginInvulnerability()
{
    ++InvulnerabilityRefCount;
    if (!CachedStats.IsValid())
    {
        return;
    }

    CachedStats->bIsInvulnerable = InvulnerabilityRefCount > 0;
    OnInvulnerabilityChanged.Broadcast(CachedStats->bIsInvulnerable);
}

void UEvasionComponent::EndInvulnerability()
{
    InvulnerabilityRefCount = FMath::Max(0, InvulnerabilityRefCount - 1);
    if (!CachedStats.IsValid())
    {
        return;
    }

    CachedStats->bIsInvulnerable = InvulnerabilityRefCount > 0;
    OnInvulnerabilityChanged.Broadcast(CachedStats->bIsInvulnerable);
}

void UEvasionComponent::FinishActiveEvasion()
{
    if (!bIsEvading)
    {
        return;
    }

    bIsEvading = false;
    RestoreAnimRootMotionOverrideForEvasion();
    RestoreMovementRotationOverride();
    OnEvasionEnded.Broadcast(ActiveEvasionType);
}

void UEvasionComponent::ClearDodgeCooldown()
{
    bDodgeOnCooldown = false;
}

void UEvasionComponent::ClearRollCooldown()
{
    bRollOnCooldown = false;
}

void UEvasionComponent::BroadcastFail(ERPGEvasionType EvasionType, const FString& Reason)
{
    OnEvasionFailed.Broadcast(EvasionType, Reason);
}







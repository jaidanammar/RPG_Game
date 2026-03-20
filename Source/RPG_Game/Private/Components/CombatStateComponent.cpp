#include "Components/CombatStateComponent.h"

#include "Components/AttackSystemComponent.h"
#include "Components/PlayerStatsComponent.h"
#include "Components/EvasionComponent.h"
#include "Components/LocomotionComponent.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"

UCombatStateComponent::UCombatStateComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UCombatStateComponent::BeginPlay()
{
    Super::BeginPlay();

    CachedStats = GetOwner() ? GetOwner()->FindComponentByClass<UPlayerStatsComponent>() : nullptr;
    CachedLocomotion = GetOwner() ? GetOwner()->FindComponentByClass<ULocomotionComponent>() : nullptr;

    if (ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner()))
    {
        CachedMoveComp = OwnerCharacter->GetCharacterMovement();
    }

    if (CachedStats.IsValid())
    {
        CachedStats->OnDeath.AddDynamic(this, &UCombatStateComponent::HandleOwnerDeath);
        CachedStats->OnHitReceived.AddDynamic(this, &UCombatStateComponent::HandleOwnerHitReceived);

        if (CachedStats->IsDead())
        {
            CurrentState = ERPGCombatState::Dead;
        }
        else
        {
            CurrentState = ERPGCombatState::Idle;
        }
    }
    else
    {
        CurrentState = ERPGCombatState::Idle;
    }
}

void UCombatStateComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    EndParryWindow(false);

    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(ParryCooldownTimerHandle);
    }

    bParryOnCooldown = false;
    RestoreAnimRootMotionOverrideForHitstun();
    RestoreGuardMovementPolicy();
    Super::EndPlay(EndPlayReason);
}

void UCombatStateComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (bGuardInputHeld
        && CurrentState != ERPGCombatState::Guard
        && CurrentState != ERPGCombatState::Dead
        && CurrentState != ERPGCombatState::Hitstun)
    {
        StartGuard();
    }

    if (CurrentState == ERPGCombatState::Guard)
    {
        TickGuardStamina(DeltaTime);
    }

    if (CurrentState == ERPGCombatState::Hitstun && CachedMoveComp.IsValid())
    {
        const FVector CurrentVelocity = CachedMoveComp->Velocity;
        CachedMoveComp->Velocity = FVector(0.0f, 0.0f, CurrentVelocity.Z);

        if (bLockHorizontalPositionDuringHitstun && GetOwner() && CachedMoveComp->IsMovingOnGround())
        {
            const FVector OwnerLocation = GetOwner()->GetActorLocation();
            GetOwner()->SetActorLocation(FVector(HitstunAnchorLocation.X, HitstunAnchorLocation.Y, OwnerLocation.Z));
        }
    }
    if (!bGuardInputHeld && CurrentState == ERPGCombatState::Guard)
    {
        StopGuard();
    }

    if (CurrentState != ERPGCombatState::Guard && bGuardWalkSpeedOverrideActive)
    {
        RestoreGuardMovementPolicy();
    }
}

bool UCombatStateComponent::CanTransitionTo(ERPGCombatState NewState) const
{
    if (CurrentState == NewState)
    {
        return true;
    }

    if (!bEnforceTransitionRules)
    {
        return true;
    }

    if (bLockStateWhenDead && CurrentState == ERPGCombatState::Dead)
    {
        return false;
    }

    if (NewState == ERPGCombatState::Dead)
    {
        return true;
    }

    if (NewState == ERPGCombatState::Guard && !bAllowGuardState)
    {
        return false;
    }

    switch (CurrentState)
    {
    case ERPGCombatState::Idle:
        return NewState == ERPGCombatState::AttackStartup
            || NewState == ERPGCombatState::Guard
            || NewState == ERPGCombatState::Hitstun;

    case ERPGCombatState::AttackStartup:
        return NewState == ERPGCombatState::AttackActive
            || NewState == ERPGCombatState::AttackRecovery
            || NewState == ERPGCombatState::Hitstun
            || NewState == ERPGCombatState::Guard;

    case ERPGCombatState::AttackActive:
        return NewState == ERPGCombatState::AttackRecovery
            || NewState == ERPGCombatState::AttackStartup
            || NewState == ERPGCombatState::Hitstun
            || NewState == ERPGCombatState::Guard;

    case ERPGCombatState::AttackRecovery:
        return NewState == ERPGCombatState::Idle
            || NewState == ERPGCombatState::AttackStartup
            || NewState == ERPGCombatState::Hitstun
            || NewState == ERPGCombatState::Guard;

    case ERPGCombatState::Guard:
        return NewState == ERPGCombatState::Idle
            || NewState == ERPGCombatState::AttackStartup
            || NewState == ERPGCombatState::Hitstun;

    case ERPGCombatState::Hitstun:
        return NewState == ERPGCombatState::Idle
            || NewState == ERPGCombatState::Guard;

    case ERPGCombatState::Dead:
    default:
        return false;
    }
}

bool UCombatStateComponent::RequestState(ERPGCombatState NewState, bool bForce)
{
    if (CurrentState == NewState)
    {
        return true;
    }

    if (!bForce && !CanTransitionTo(NewState))
    {
        return false;
    }

    if (CurrentState == ERPGCombatState::Hitstun && GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(HitstunTimerHandle);
    }

    const ERPGCombatState OldState = CurrentState;
    CurrentState = NewState;

    if (CachedStats.IsValid() && bDisableStaminaRegenWhileGuarding)
    {
        CachedStats->bAllowStaminaRegen = (CurrentState != ERPGCombatState::Guard);
    }

    if (CurrentState == ERPGCombatState::Guard)
    {
        GuardDrainAccumulator = 0.0f;
        ApplyGuardMovementPolicy();
    }
    else if (OldState == ERPGCombatState::Guard)
    {
        GuardDrainAccumulator = 0.0f;
        RestoreGuardMovementPolicy();
    }

    OnCombatStateChanged.Broadcast(OldState, NewState);

    if (OldState != ERPGCombatState::Guard && NewState == ERPGCombatState::Guard)
    {
        OnGuardStateChanged.Broadcast(true);
    }
    else if (OldState == ERPGCombatState::Guard && NewState != ERPGCombatState::Guard)
    {
        OnGuardStateChanged.Broadcast(false);
    }

    return true;
}

bool UCombatStateComponent::StartGuard()
{
    const bool bWasGuardInputHeld = bGuardInputHeld;
    bGuardInputHeld = true;

    if (bBeginParryOnGuardPressed && !bWasGuardInputHeld)
    {
        BeginParryAttempt();
    }

    if (!bAllowGuardState)
    {
        return false;
    }

    if (!HasGuardStamina())
    {
        return false;
    }

    bool bDidEnterGuard = RequestState(ERPGCombatState::Guard);
    if (!bDidEnterGuard && CurrentState != ERPGCombatState::Dead)
    {
        bDidEnterGuard = RequestState(ERPGCombatState::Guard, true);
    }

    if (bDidEnterGuard)
    {
        GuardDrainAccumulator = 0.0f;
    }

    return bDidEnterGuard;
}

bool UCombatStateComponent::StopGuard()
{
    bGuardInputHeld = false;
    EndParryWindow(false);

    if (CurrentState != ERPGCombatState::Guard)
    {
        return true;
    }

    if (RequestState(ERPGCombatState::Idle))
    {
        return true;
    }

    return RequestState(ERPGCombatState::Idle, true);
}

void UCombatStateComponent::HandleGuardPressed()
{
    StartGuard();
}

void UCombatStateComponent::HandleGuardReleased()
{
    StopGuard();
}

bool UCombatStateComponent::BeginParryAttempt()
{
    if (!bAllowParry || bParryWindowActive || bParryOnCooldown || CurrentState == ERPGCombatState::Dead)
    {
        return false;
    }

    if (CachedLocomotion.IsValid() && !CachedLocomotion->IsCapabilityAllowed(ERPGMovementCapability::Parry))
    {
        return false;
    }

    if (CachedStats.IsValid() && ParryStaminaCost > 0.0f && CachedStats->CurrentStamina < ParryStaminaCost)
    {
        return false;
    }

    if (CachedStats.IsValid() && ParryStaminaCost > 0.0f)
    {
        CachedStats->DecreaseStamina(ParryStaminaCost);
    }

    bParryWindowActive = true;
    OnParryWindowChanged.Broadcast(true);

    const double WorldTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
    const double PerfectWindow = FMath::Clamp(static_cast<double>(PerfectParryWindowDuration), 0.0, static_cast<double>(ParryWindowDuration));
    PerfectParryWindowEndTime = WorldTime + PerfectWindow;

    if (GetWorld())
    {
        const float SafeDuration = FMath::Max(ParryWindowDuration, 0.01f);
        GetWorld()->GetTimerManager().ClearTimer(ParryWindowTimerHandle);
        GetWorld()->GetTimerManager().SetTimer(
            ParryWindowTimerHandle,
            FTimerDelegate::CreateWeakLambda(this, [this]()
            {
                EndParryWindow(true);
            }),
            SafeDuration,
            false);
    }

    return true;
}

bool UCombatStateComponent::TryNegateIncomingDamage(const FRPGDamageSpec& DamageSpec, bool& bOutPerfectParry)
{
    bOutPerfectParry = false;

    if (CurrentState == ERPGCombatState::Hitstun)
    {
        return false;
    }

    if (DamageSpec.bCanBeParried && bParryWindowActive)
    {
        const double WorldTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
        bOutPerfectParry = WorldTime <= PerfectParryWindowEndTime;

        EndParryWindow(false);

        bParryOnCooldown = true;
        if (GetWorld())
        {
            const float Cooldown = FMath::Max(ParryCooldown, 0.01f);
            GetWorld()->GetTimerManager().ClearTimer(ParryCooldownTimerHandle);
            GetWorld()->GetTimerManager().SetTimer(ParryCooldownTimerHandle, this, &UCombatStateComponent::EndParryCooldown, Cooldown, false);
        }

        const float AttackerHitstunDuration = bOutPerfectParry ? 0.4f : 0.25f;
        if (bDealParryCounterDamage && ParryCounterDamage > 0.0f)
        {
            if (UAttackSystemComponent* AttackSystem = GetOwner() ? GetOwner()->FindComponentByClass<UAttackSystemComponent>() : nullptr)
            {
                FRPGDamageSpec CounterDamageSpec;
                CounterDamageSpec.Damage = ParryCounterDamage * (bOutPerfectParry ? PerfectParryCounterDamageMultiplier : 1.0f);
                CounterDamageSpec.HitstunDuration = AttackerHitstunDuration;
                CounterDamageSpec.ReactionStrength = bOutPerfectParry ? ERPGHitReactionStrength::Heavy : ERPGHitReactionStrength::Light;
                CounterDamageSpec.bCanBeBlocked = false;
                CounterDamageSpec.bCanBeParried = false;
                CounterDamageSpec.DamageCauser = GetOwner();
                if (const ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner()))
                {
                    CounterDamageSpec.EventInstigator = OwnerCharacter->GetController();
                }
                AttackSystem->PrimeTraceDamageSpec(CounterDamageSpec);
            }
        }

        if (UCombatStateComponent* AttackerCombatState = DamageSpec.DamageCauser ? DamageSpec.DamageCauser->FindComponentByClass<UCombatStateComponent>() : nullptr)
        {
            AttackerCombatState->ApplyHitstun(AttackerHitstunDuration);
            AttackerCombatState->OnParried.Broadcast(bOutPerfectParry);
        }

        if (bEnterGuardStateOnParrySuccess && bGuardInputHeld)
        {
            RequestState(ERPGCombatState::Guard, true);
        }

        OnParrySuccess.Broadcast(DamageSpec.DamageCauser, bOutPerfectParry);
        return true;
    }

    if (CurrentState == ERPGCombatState::Guard && DamageSpec.bCanBeBlocked)
    {
        if (CachedStats.IsValid())
        {
            const float GuardStaminaDamage = FMath::Max(0.0f, DamageSpec.Damage * GuardStaminaDamageMultiplier);
            const bool bStaminaDepleted = CachedStats->DecreaseStamina(GuardStaminaDamage);
            if (bStaminaDepleted && bBreakGuardWhenStaminaDepleted)
            {
                RequestState(ERPGCombatState::Idle, true);
                LastHitReactionStrength = ERPGHitReactionStrength::GuardBreak;
                LastHitDirection = DamageSpec.HitDirection;
                OnHitReactionUpdated.Broadcast(LastHitReactionStrength, LastHitDirection);
                ApplyHitstun(GuardBreakHitstunDuration);
                return false;
            }
        }

        LastHitReactionStrength = ERPGHitReactionStrength::None;
        LastHitDirection = DamageSpec.HitDirection;
        OnHitReactionUpdated.Broadcast(LastHitReactionStrength, LastHitDirection);
        return true;
    }

    return false;
}

bool UCombatStateComponent::ApplyHitstun(float Duration)
{
    const float ResolvedDuration = Duration > 0.0f ? Duration : DefaultHitstunDuration;
    if (ResolvedDuration <= 0.0f)
    {
        return false;
    }

    if (CurrentState == ERPGCombatState::Dead)
    {
        return false;
    }

    if (CurrentState == ERPGCombatState::Guard && bGuardPreventsHitstun)
    {
        return false;
    }

    if (!RequestState(ERPGCombatState::Hitstun, true))
    {
        return false;
    }

    bGuardInputHeld = false;
    EndParryWindow(false);
    RestoreGuardMovementPolicy();

    HitstunAnchorLocation = GetOwner() ? GetOwner()->GetActorLocation() : FVector::ZeroVector;

    if (UEvasionComponent* EvasionComponent = GetOwner() ? GetOwner()->FindComponentByClass<UEvasionComponent>() : nullptr)
    {
        EvasionComponent->CancelActiveInvulnerability();
    }

    ApplyAnimRootMotionOverrideForHitstun();

    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(HitstunTimerHandle);
        GetWorld()->GetTimerManager().SetTimer(HitstunTimerHandle, this, &UCombatStateComponent::EndHitstun, ResolvedDuration, false);
    }

    return true;
}

void UCombatStateComponent::HandleOwnerDeath()
{
    EndParryWindow(false);
    RestoreAnimRootMotionOverrideForHitstun();
    RequestState(ERPGCombatState::Dead, true);
}

void UCombatStateComponent::HandleOwnerHitReceived(FRPGDamageSpec DamageSpec, float DamageApplied, float NewHealth, float MaxHealth)
{
    if (DamageApplied <= 0.0f || NewHealth <= 0.0f || !bEnterHitstunOnDamage)
    {
        return;
    }

    LastHitReactionStrength = DamageSpec.ReactionStrength;
    LastHitDirection = DamageSpec.HitDirection;
    OnHitReactionUpdated.Broadcast(LastHitReactionStrength, LastHitDirection);

    const float HitstunDuration = DamageSpec.HitstunDuration > 0.0f ? DamageSpec.HitstunDuration : DefaultHitstunDuration;
    ApplyHitstun(HitstunDuration);
}

void UCombatStateComponent::EndHitstun()
{
    if (CurrentState == ERPGCombatState::Hitstun)
    {
        RequestState(ERPGCombatState::Idle, true);
    }
    RestoreAnimRootMotionOverrideForHitstun();
}

void UCombatStateComponent::TickGuardStamina(float DeltaTime)
{
    if (!CachedStats.IsValid() || DeltaTime <= 0.0f)
    {
        return;
    }

    const float DrainPerSecond = bGuardDrainUsesPercentOfMaxStamina
        ? (CachedStats->MaxStamina * (GuardStaminaDrainPercentPerSecond / 100.0f))
        : GuardStaminaDrainPerSecond;

    if (DrainPerSecond <= 0.0f)
    {
        return;
    }

    const float DrainAmount = DrainPerSecond * DeltaTime;
    const float OldStamina = CachedStats->CurrentStamina;
    CachedStats->CurrentStamina = FMath::Clamp(CachedStats->CurrentStamina - DrainAmount, 0.0f, CachedStats->MaxStamina);

    if (!FMath::IsNearlyEqual(OldStamina, CachedStats->CurrentStamina))
    {
        CachedStats->OnStaminaChanged.Broadcast(CachedStats->CurrentStamina, CachedStats->MaxStamina);
    }

    const bool bStaminaDepleted = CachedStats->CurrentStamina <= KINDA_SMALL_NUMBER;
    if (bStaminaDepleted && bBreakGuardWhenStaminaDepleted)
    {
        bGuardInputHeld = false;
        RequestState(ERPGCombatState::Idle, true);
        ApplyHitstun(GuardBreakHitstunDuration);
    }
}

bool UCombatStateComponent::HasGuardStamina() const
{
    return !CachedStats.IsValid() || CachedStats->CurrentStamina > 0.0f;
}

void UCombatStateComponent::ApplyGuardMovementPolicy()
{
    if (!bLimitMovementWhileGuarding)
    {
        return;
    }

    if (!CachedLocomotion.IsValid())
    {
        CachedLocomotion = GetOwner() ? GetOwner()->FindComponentByClass<ULocomotionComponent>() : nullptr;
    }

    if (CachedLocomotion.IsValid())
    {
        const float SpeedMultiplier = FMath::Clamp(GuardWalkSpeedMultiplier, 0.0f, 1.0f);
        CachedLocomotion->SetSpeedMultiplier(TEXT("Guard"), SpeedMultiplier);
        bGuardWalkSpeedOverrideActive = true;
        return;
    }

    if (!CachedMoveComp.IsValid())
    {
        if (ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner()))
        {
            CachedMoveComp = OwnerCharacter->GetCharacterMovement();
        }
    }

    if (!CachedMoveComp.IsValid())
    {
        return;
    }

    if (!bGuardWalkSpeedOverrideActive)
    {
        SavedGuardWalkSpeed = CachedMoveComp->MaxWalkSpeed;
        bGuardWalkSpeedOverrideActive = true;
    }

    const float SpeedMultiplier = FMath::Clamp(GuardWalkSpeedMultiplier, 0.0f, 1.0f);
    CachedMoveComp->MaxWalkSpeed = SavedGuardWalkSpeed * SpeedMultiplier;
}

void UCombatStateComponent::RestoreGuardMovementPolicy()
{
    if (!bGuardWalkSpeedOverrideActive)
    {
        return;
    }

    if (CachedLocomotion.IsValid())
    {
        CachedLocomotion->ClearSpeedMultiplier(TEXT("Guard"));
        bGuardWalkSpeedOverrideActive = false;
        return;
    }

    if (CachedMoveComp.IsValid())
    {
        CachedMoveComp->MaxWalkSpeed = SavedGuardWalkSpeed;
    }

    bGuardWalkSpeedOverrideActive = false;
}

void UCombatStateComponent::EndParryWindow(bool bBroadcastFail)
{
    if (!bParryWindowActive)
    {
        return;
    }

    bParryWindowActive = false;
    PerfectParryWindowEndTime = 0.0;

    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(ParryWindowTimerHandle);
    }

    OnParryWindowChanged.Broadcast(false);

    if (bBroadcastFail)
    {
        if (CachedStats.IsValid() && ParryFailStaminaCost > 0.0f)
        {
            CachedStats->DecreaseStamina(ParryFailStaminaCost);
        }

        bParryOnCooldown = true;
        if (GetWorld())
        {
            const float Cooldown = FMath::Max(ParryCooldown, 0.01f);
            GetWorld()->GetTimerManager().ClearTimer(ParryCooldownTimerHandle);
            GetWorld()->GetTimerManager().SetTimer(ParryCooldownTimerHandle, this, &UCombatStateComponent::EndParryCooldown, Cooldown, false);
        }

        OnParryFailed.Broadcast();
    }
}

void UCombatStateComponent::EndParryCooldown()
{
    bParryOnCooldown = false;

    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(ParryCooldownTimerHandle);
    }
}







void UCombatStateComponent::ApplyAnimRootMotionOverrideForHitstun()
{
    if (!bDisableMontageRootMotionDuringHitstun || bHitstunRootMotionOverrideActive)
    {
        return;
    }

    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (!OwnerCharacter || !OwnerCharacter->GetMesh())
    {
        return;
    }

    if (UAnimInstance* AnimInstance = OwnerCharacter->GetMesh()->GetAnimInstance())
    {
        SavedHitstunRootMotionMode = static_cast<uint8>(AnimInstance->RootMotionMode);
        AnimInstance->SetRootMotionMode(ERootMotionMode::NoRootMotionExtraction);
        bHitstunRootMotionOverrideActive = true;
    }
}

void UCombatStateComponent::RestoreAnimRootMotionOverrideForHitstun()
{
    if (!bHitstunRootMotionOverrideActive)
    {
        return;
    }

    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (!OwnerCharacter || !OwnerCharacter->GetMesh())
    {
        bHitstunRootMotionOverrideActive = false;
        return;
    }

    if (UAnimInstance* AnimInstance = OwnerCharacter->GetMesh()->GetAnimInstance())
    {
        AnimInstance->SetRootMotionMode(static_cast<ERootMotionMode::Type>(SavedHitstunRootMotionMode));
    }

    bHitstunRootMotionOverrideActive = false;
}






#include "Components/CombatStateComponent.h"

#include "Components/PlayerStatsComponent.h"
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

    if (ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner()))
    {
        CachedMoveComp = OwnerCharacter->GetCharacterMovement();
    }

    if (CachedStats.IsValid())
    {
        CachedStats->OnDeath.AddDynamic(this, &UCombatStateComponent::HandleOwnerDeath);
        CachedStats->OnDamaged.AddDynamic(this, &UCombatStateComponent::HandleOwnerDamaged);

        if (CachedStats->IsDead())
        {
            CurrentState = ERPGCombatState::Dead;
        }
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
    RestoreGuardMovementPolicy();
    Super::EndPlay(EndPlayReason);
}

void UCombatStateComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (CurrentState == ERPGCombatState::Guard)
    {
        TickGuardStamina(DeltaTime);
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
    bGuardInputHeld = true;

    if (!bAllowGuardState)
    {
        return false;
    }

    if (!HasGuardStamina())
    {
        return false;
    }

    const bool bDidEnterGuard = RequestState(ERPGCombatState::Guard);
    if (bDidEnterGuard)
    {
        GuardDrainAccumulator = 0.0f;
    }

    return bDidEnterGuard;
}

bool UCombatStateComponent::StopGuard()
{
    bGuardInputHeld = false;

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
    bGuardInputHeld = true;
    StartGuard();
}

void UCombatStateComponent::HandleGuardReleased()
{
    bGuardInputHeld = false;
    StopGuard();
}

bool UCombatStateComponent::BeginParryAttempt()
{
    if (!bAllowParry || bParryWindowActive || bParryOnCooldown || CurrentState == ERPGCombatState::Dead)
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

bool UCombatStateComponent::TryNegateIncomingDamage(AActor* DamageCauser, bool& bOutPerfectParry)
{
    bOutPerfectParry = false;

    if (!bParryWindowActive)
    {
        return false;
    }

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

    if (bEnterGuardStateOnParrySuccess)
    {
        RequestState(ERPGCombatState::Guard, true);
    }

    OnParrySuccess.Broadcast(DamageCauser, bOutPerfectParry);
    return true;
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
    RequestState(ERPGCombatState::Dead, true);
}

void UCombatStateComponent::HandleOwnerDamaged(float Damage, float NewHealth, float MaxHealth)
{
    if (Damage <= 0.0f || NewHealth <= 0.0f || !bEnterHitstunOnDamage)
    {
        return;
    }

    ApplyHitstun(DefaultHitstunDuration);
}

void UCombatStateComponent::EndHitstun()
{
    if (CurrentState == ERPGCombatState::Hitstun)
    {
        RequestState(ERPGCombatState::Idle, true);
    }
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


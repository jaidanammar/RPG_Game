#include "Components/CombatStateComponent.h"

#include "Components/PlayerStatsComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"

UCombatStateComponent::UCombatStateComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UCombatStateComponent::BeginPlay()
{
    Super::BeginPlay();

    CachedStats = GetOwner() ? GetOwner()->FindComponentByClass<UPlayerStatsComponent>() : nullptr;
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

void UCombatStateComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (CurrentState == ERPGCombatState::Guard)
    {
        TickGuardStamina(DeltaTime);
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
            || NewState == ERPGCombatState::Hitstun;

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

    if (OldState == ERPGCombatState::Guard)
    {
        GuardDrainAccumulator = 0.0f;
    }

    OnCombatStateChanged.Broadcast(OldState, NewState);
    return true;
}

bool UCombatStateComponent::StartGuard()
{
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
    if (CurrentState != ERPGCombatState::Guard)
    {
        return true;
    }

    return RequestState(ERPGCombatState::Idle);
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
        RequestState(ERPGCombatState::Idle, true);
        ApplyHitstun(GuardBreakHitstunDuration);
    }
}

bool UCombatStateComponent::HasGuardStamina() const
{
    return !CachedStats.IsValid() || CachedStats->CurrentStamina > 0.0f;
}

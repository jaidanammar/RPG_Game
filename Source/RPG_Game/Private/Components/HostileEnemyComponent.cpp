#include "Components/HostileEnemyComponent.h"

#include "Components/AttackSystemComponent.h"
#include "Components/CombatStateComponent.h"
#include "Components/PlayerStatsComponent.h"
#include "Components/TargetLockComponent.h"
#include "GameFramework/Character.h"

UHostileEnemyComponent::UHostileEnemyComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UHostileEnemyComponent::BeginPlay()
{
    Super::BeginPlay();

    CachedCharacter = Cast<ACharacter>(GetOwner());
    CachedAttackSystem = GetOwner() ? GetOwner()->FindComponentByClass<UAttackSystemComponent>() : nullptr;
    CachedCombatState = GetOwner() ? GetOwner()->FindComponentByClass<UCombatStateComponent>() : nullptr;
    CachedStats = GetOwner() ? GetOwner()->FindComponentByClass<UPlayerStatsComponent>() : nullptr;
    CachedTargetLock = GetOwner() ? GetOwner()->FindComponentByClass<UTargetLockComponent>() : nullptr;

    if (CachedStats.IsValid())
    {
        CachedStats->OnHitReceived.AddDynamic(this, &UHostileEnemyComponent::HandleOwnerHitReceived);
    }
}

void UHostileEnemyComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    UpdateHostileBehavior(DeltaTime);
}

void UHostileEnemyComponent::SetHostileTarget(AActor* NewTarget)
{
    if (!IsTargetValid(NewTarget))
    {
        return;
    }

    CurrentTarget = NewTarget;
    bIsHostile = true;
    LastObservedTargetState = ERPGCombatState::Idle;
    NextDecisionTime = 0.0;

    if (CachedTargetLock.IsValid())
    {
        CachedTargetLock->SetLockTarget(NewTarget);
    }
}

void UHostileEnemyComponent::ClearHostileTarget()
{
    CurrentTarget = nullptr;
    bIsHostile = false;
    GuardReleaseTime = 0.0;
    RetreatEndTime = 0.0;

    if (CachedCombatState.IsValid())
    {
        CachedCombatState->StopGuard();
    }

    if (CachedTargetLock.IsValid())
    {
        CachedTargetLock->ClearLock();
    }
}

void UHostileEnemyComponent::HandleOwnerHitReceived(FRPGDamageSpec DamageSpec, float DamageApplied, float NewHealth, float MaxHealth)
{
    if (!bBecomeHostileOnDamage || DamageApplied <= 0.0f)
    {
        return;
    }

    SetHostileTarget(DamageSpec.DamageCauser);
}

void UHostileEnemyComponent::UpdateHostileBehavior(float DeltaTime)
{
    if (!bIsHostile || !CachedCharacter.IsValid())
    {
        return;
    }

    if (!IsTargetValid(CurrentTarget))
    {
        ClearHostileTarget();
        return;
    }

    if (CachedStats.IsValid() && CachedStats->IsDead())
    {
        return;
    }

    if (CachedCombatState.IsValid())
    {
        if (CachedCombatState->IsInState(ERPGCombatState::Dead) || CachedCombatState->IsInState(ERPGCombatState::Hitstun))
        {
            return;
        }
    }

    AActor* TargetActor = CurrentTarget;
    UCombatStateComponent* TargetCombatState = TargetActor ? TargetActor->FindComponentByClass<UCombatStateComponent>() : nullptr;
    const ERPGCombatState TargetState = TargetCombatState ? TargetCombatState->GetCombatState() : ERPGCombatState::Idle;

    const FVector OwnerLocation = CachedCharacter->GetActorLocation();
    const FVector TargetLocation = TargetActor->GetActorLocation();
    FVector ToTarget = TargetLocation - OwnerLocation;
    ToTarget.Z = 0.0f;

    const float Distance = ToTarget.Size();
    const FVector Direction = ToTarget.GetSafeNormal();
    const double WorldTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;

    if (bFaceTargetWhileHostile && !Direction.IsNearlyZero())
    {
        const FRotator DesiredRotation = Direction.Rotation();
        const FRotator NewRotation = FMath::RInterpTo(CachedCharacter->GetActorRotation(), DesiredRotation, DeltaTime, FaceTargetInterpSpeed);
        CachedCharacter->SetActorRotation(NewRotation);
    }

    const bool bThreatActive = TargetState == ERPGCombatState::AttackStartup || TargetState == ERPGCombatState::AttackActive;
    const bool bIsPunishWindow = ShouldPunishTarget(TargetState, Distance);

    if (TryStartGuardAgainstTargetState(TargetState, Distance, WorldTime))
    {
        LastObservedTargetState = TargetState;
        return;
    }

    StopGuardIfNeeded(bThreatActive, WorldTime);

    if (WorldTime < RetreatEndTime && !Direction.IsNearlyZero() && Distance < PreferredCombatDistance)
    {
        CachedCharacter->AddMovementInput(-Direction, 0.75f, true);
        LastObservedTargetState = TargetState;
        return;
    }

    if (Distance > ChaseStopDistance && !Direction.IsNearlyZero())
    {
        CachedCharacter->AddMovementInput(Direction, 1.0f, true);
    }
    else if (!bThreatActive && Distance < RetreatDistance && !Direction.IsNearlyZero())
    {
        CachedCharacter->AddMovementInput(-Direction, 0.35f, true);
    }

    if (!CanAttackTarget() || Distance > AttackRange || !IsFacingTarget(Direction))
    {
        LastObservedTargetState = TargetState;
        return;
    }

    if (WorldTime < NextAttackTime || WorldTime < NextDecisionTime)
    {
        LastObservedTargetState = TargetState;
        return;
    }

    const float AttackChance = bIsPunishWindow ? PunishAttackChance : BaseAttackChance;
    NextDecisionTime = WorldTime + GetNextDecisionDelay();
    if (FMath::FRand() > AttackChance)
    {
        LastObservedTargetState = TargetState;
        return;
    }

    if (!Direction.IsNearlyZero())
    {
        const FRotator AttackFacing(0.0f, Direction.Rotation().Yaw, 0.0f);
        CachedCharacter->SetActorRotation(AttackFacing);
        if (AController* Controller = CachedCharacter->GetController())
        {
            Controller->SetControlRotation(AttackFacing);
        }
    }

    CachedAttackSystem->HandleAttackInput();
    NextAttackTime = WorldTime + AttackInterval + FMath::FRandRange(0.05f, 0.2f);
    StartRetreat(WorldTime);
    LastObservedTargetState = TargetState;
}

bool UHostileEnemyComponent::CanAttackTarget() const
{
    if (!CachedAttackSystem.IsValid() || CachedAttackSystem->IsAttacking())
    {
        return false;
    }

    if (CachedCombatState.IsValid() && (CachedCombatState->IsInState(ERPGCombatState::Dead) || CachedCombatState->IsInState(ERPGCombatState::Guard)))
    {
        return false;
    }

    return true;
}

bool UHostileEnemyComponent::IsFacingTarget(const FVector& DirectionToTarget) const
{
    if (!CachedCharacter.IsValid() || DirectionToTarget.IsNearlyZero())
    {
        return true;
    }

    const FVector Forward = CachedCharacter->GetActorForwardVector().GetSafeNormal2D();
    const FVector TargetDirection = DirectionToTarget.GetSafeNormal2D();
    return FVector::DotProduct(Forward, TargetDirection) >= MinFacingDotToAttack;
}

bool UHostileEnemyComponent::IsTargetValid(AActor* TargetActor) const
{
    if (!IsValid(TargetActor) || TargetActor == GetOwner())
    {
        return false;
    }

    if (const UPlayerStatsComponent* TargetStats = TargetActor->FindComponentByClass<UPlayerStatsComponent>())
    {
        return !TargetStats->IsDead();
    }

    return true;
}

bool UHostileEnemyComponent::TryStartGuardAgainstTargetState(ERPGCombatState TargetState, float DistanceToTarget, double WorldTime)
{
    if (!CachedCombatState.IsValid())
    {
        return false;
    }

    const bool bThreatActive = TargetState == ERPGCombatState::AttackStartup || TargetState == ERPGCombatState::AttackActive;
    if (!bThreatActive || DistanceToTarget > ThreatGuardRange)
    {
        return false;
    }

    const bool bNewThreatWindow = TargetState != LastObservedTargetState;
    const bool bShouldEvaluate = bNewThreatWindow || WorldTime >= NextDecisionTime;
    if (!bShouldEvaluate)
    {
        return CachedCombatState->IsInState(ERPGCombatState::Guard);
    }

    NextDecisionTime = WorldTime + GetNextDecisionDelay();
    if (CachedCombatState->IsInState(ERPGCombatState::Guard))
    {
        GuardReleaseTime = WorldTime + FMath::FRandRange(MinGuardHoldDuration, MaxGuardHoldDuration);
        return true;
    }

    if (FMath::FRand() > GuardAgainstAttackChance)
    {
        return false;
    }

    if (!CachedCombatState->StartGuard())
    {
        return false;
    }

    GuardReleaseTime = WorldTime + FMath::FRandRange(MinGuardHoldDuration, MaxGuardHoldDuration);
    NextAttackTime = FMath::Max(NextAttackTime, WorldTime + PostGuardAttackDelay);
    return true;
}

void UHostileEnemyComponent::StopGuardIfNeeded(bool bThreatActive, double WorldTime)
{
    if (!CachedCombatState.IsValid() || !CachedCombatState->IsInState(ERPGCombatState::Guard))
    {
        return;
    }

    if (bThreatActive || WorldTime < GuardReleaseTime)
    {
        return;
    }

    CachedCombatState->StopGuard();
}

bool UHostileEnemyComponent::ShouldPunishTarget(ERPGCombatState TargetState, float DistanceToTarget) const
{
    return DistanceToTarget <= AttackRange && (TargetState == ERPGCombatState::AttackRecovery || TargetState == ERPGCombatState::Hitstun);
}

void UHostileEnemyComponent::StartRetreat(double WorldTime)
{
    RetreatEndTime = WorldTime + FMath::Max(0.0f, RetreatDuration);
}

float UHostileEnemyComponent::GetNextDecisionDelay() const
{
    const float MinDelay = FMath::Max(0.01f, MinDecisionInterval);
    const float MaxDelay = FMath::Max(MinDelay, MaxDecisionInterval);
    return FMath::FRandRange(MinDelay, MaxDelay);
}
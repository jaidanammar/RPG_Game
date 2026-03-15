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
    FeintEndTime = 0.0;
    RetreatEndTime = 0.0;
    AdvanceCommitEndTime = 0.0;
    NextStrafeSwapTime = 0.0;
    AttackPressure = 0.0f;
    StrafeDirectionSign = FMath::RandBool() ? 1 : -1;

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
    FeintEndTime = 0.0;
    HesitationEndTime = 0.0;
    AdvanceCommitEndTime = 0.0;
    NextStrafeSwapTime = 0.0;
    AttackPressure = 0.0f;

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
    if (DamageApplied <= 0.0f)
    {
        return;
    }

    const double WorldTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
    RecentDamageTime = WorldTime;
    HesitationEndTime = WorldTime + FMath::Max(0.0f, HesitationAfterTakingHit);
    AttackPressure = FMath::Max(0.0f, AttackPressure - 0.18f);
    FeintEndTime = 0.0;
    AdvanceCommitEndTime = 0.0;

    if (bBecomeHostileOnDamage)
    {
        SetHostileTarget(DamageSpec.DamageCauser);
    }
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
    const bool bHesitating = IsHesitating(WorldTime);
    const bool bCommittedAdvance = WorldTime < AdvanceCommitEndTime;

    const float PressureDelta = bIsPunishWindow ? PressureBuildPerSecond : -PressureDecayPerSecond;
    AttackPressure = FMath::Clamp(AttackPressure + (PressureDelta * DeltaTime), 0.0f, MaxPressureBonus);

    if (bIsPunishWindow || (Distance > AttackRange && !bThreatActive))
    {
        AdvanceCommitEndTime = FMath::Max(AdvanceCommitEndTime, WorldTime + AdvanceCommitDuration);
    }

    if (TryStartGuardAgainstTargetState(TargetState, Distance, WorldTime))
    {
        LastObservedTargetState = TargetState;
        return;
    }

    StopGuardIfNeeded(bThreatActive, WorldTime);

    const FVector StrafeDirection = GetStrafeDirection(Direction, WorldTime);
    const float DistanceError = Distance - PreferredCombatDistance;
    const bool bInsideCombatBand = FMath::Abs(DistanceError) <= CombatDistanceTolerance;
    const bool bFeinting = WorldTime < FeintEndTime;

    if (WorldTime < RetreatEndTime && !Direction.IsNearlyZero() && Distance < PreferredCombatDistance)
    {
        CachedCharacter->AddMovementInput(-Direction, 0.55f, true);
        CachedCharacter->AddMovementInput(StrafeDirection, StrafeWeight * 0.5f, true);
        LastObservedTargetState = TargetState;
        return;
    }

    if (!Direction.IsNearlyZero())
    {
        if (Distance > ChaseStopDistance)
        {
            CachedCharacter->AddMovementInput(Direction, 1.0f, true);
        }
        else if (bCommittedAdvance || bIsPunishWindow)
        {
            CachedCharacter->AddMovementInput(Direction, 0.9f, true);
            CachedCharacter->AddMovementInput(StrafeDirection, StrafeWeight * 0.35f, true);
        }
        else if (DistanceError > CombatDistanceTolerance)
        {
            CachedCharacter->AddMovementInput(Direction, 0.82f, true);
        }
        else if (DistanceError < -CombatDistanceTolerance)
        {
            CachedCharacter->AddMovementInput(-Direction, 0.28f, true);
        }
        else if (!bThreatActive)
        {
            CachedCharacter->AddMovementInput(Direction, 0.28f, true);
            CachedCharacter->AddMovementInput(StrafeDirection, StrafeWeight, true);
        }
        else
        {
            CachedCharacter->AddMovementInput(-Direction, 0.16f, true);
            CachedCharacter->AddMovementInput(StrafeDirection, StrafeWeight * 0.5f, true);
        }
    }

    if (!bThreatActive && !bFeinting && !bHesitating && bInsideCombatBand && !bCommittedAdvance && WorldTime >= NextDecisionTime)
    {
        NextDecisionTime = WorldTime + GetNextDecisionDelay();
        if (FMath::FRand() <= FeintChance)
        {
            FeintEndTime = WorldTime + FMath::FRandRange(MinFeintDuration, MaxFeintDuration);
            LastObservedTargetState = TargetState;
            return;
        }
    }

    if (bFeinting)
    {
        if (!Direction.IsNearlyZero())
        {
            CachedCharacter->AddMovementInput(Direction, 0.45f, true);
            CachedCharacter->AddMovementInput(StrafeDirection, StrafeWeight * 0.8f, true);
        }

        LastObservedTargetState = TargetState;
        return;
    }

    if (!CanAttackTarget() || Distance > AttackRange || !IsFacingTarget(Direction) || bHesitating)
    {
        LastObservedTargetState = TargetState;
        return;
    }

    if (WorldTime < NextAttackTime || WorldTime < NextDecisionTime)
    {
        LastObservedTargetState = TargetState;
        return;
    }

    const float AttackChance = GetAttackChance(bIsPunishWindow, Distance, WorldTime);
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
    AttackPressure = FMath::Clamp(AttackPressure + 0.1f, 0.0f, MaxPressureBonus);
    AdvanceCommitEndTime = 0.0;
    NextAttackTime = WorldTime + AttackInterval + FMath::FRandRange(0.04f, 0.16f);
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
    return DistanceToTarget <= AttackRange * 1.15f && (TargetState == ERPGCombatState::AttackRecovery || TargetState == ERPGCombatState::Hitstun);
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

float UHostileEnemyComponent::GetAttackChance(bool bIsPunishWindow, float DistanceToTarget, double WorldTime) const
{
    float AttackChance = bIsPunishWindow ? PunishAttackChance : BaseAttackChance;
    AttackChance += FMath::Min(AttackPressure, PressureAttackChanceBonus);

    if (DistanceToTarget <= AttackRange * 0.82f)
    {
        AttackChance += CloseRangeAttackChanceBonus;
    }

    const double TimeSinceDamage = WorldTime - RecentDamageTime;
    if (TimeSinceDamage >= 0.0 && TimeSinceDamage < RecentDamagePenaltyDuration)
    {
        const float PenaltyAlpha = 1.0f - FMath::Clamp(static_cast<float>(TimeSinceDamage / FMath::Max(RecentDamagePenaltyDuration, KINDA_SMALL_NUMBER)), 0.0f, 1.0f);
        AttackChance -= DamageNervesPenalty * PenaltyAlpha;
    }

    return FMath::Clamp(AttackChance, 0.12f, 0.99f);
}

FVector UHostileEnemyComponent::GetStrafeDirection(const FVector& DirectionToTarget, double WorldTime)
{
    if (DirectionToTarget.IsNearlyZero())
    {
        return FVector::ZeroVector;
    }

    if (WorldTime >= NextStrafeSwapTime)
    {
        StrafeDirectionSign = FMath::RandBool() ? 1 : -1;
        NextStrafeSwapTime = WorldTime + FMath::FRandRange(MinStrafeSwitchInterval, FMath::Max(MinStrafeSwitchInterval, MaxStrafeSwitchInterval));
    }

    const FVector Right = FVector::CrossProduct(FVector::UpVector, DirectionToTarget).GetSafeNormal();
    return Right * static_cast<float>(StrafeDirectionSign);
}

bool UHostileEnemyComponent::IsHesitating(double WorldTime) const
{
    return WorldTime < HesitationEndTime;
}

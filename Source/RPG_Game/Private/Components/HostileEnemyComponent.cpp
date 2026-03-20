#include "Components/HostileEnemyComponent.h"

#include "Components/CombatStateComponent.h"
#include "Components/EvasionComponent.h"
#include "Components/PlayerStatsComponent.h"
#include "Components/TargetLockComponent.h"
#include "Engine/Engine.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"

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
    CachedEvasion = GetOwner() ? GetOwner()->FindComponentByClass<UEvasionComponent>() : nullptr;
    CachedStats = GetOwner() ? GetOwner()->FindComponentByClass<UPlayerStatsComponent>() : nullptr;
    CachedTargetLock = GetOwner() ? GetOwner()->FindComponentByClass<UTargetLockComponent>() : nullptr;

    if (CachedStats.IsValid())
    {
        CachedStats->OnHitReceived.AddDynamic(this, &UHostileEnemyComponent::HandleOwnerHitReceived);
    }

    if (CachedEvasion.IsValid())
    {
        CachedEvasion->OnEvasionFailed.AddDynamic(this, &UHostileEnemyComponent::HandleOwnerEvasionFailed);
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

    const bool bIsSameTarget = bIsHostile && CurrentTarget == NewTarget;
    CurrentTarget = NewTarget;
    bIsHostile = true;

    if (!bIsSameTarget)
    {
        LastObservedTargetState = ERPGCombatState::Idle;
        LastAttackInputType = ERPGAttackInputType::Light;
        NextDecisionTime = 0.0;
        NextDefenseDecisionTime = 0.0;
        DefensiveUrgencyEndTime = 0.0;
        FeintEndTime = 0.0;
        RetreatEndTime = 0.0;
        AdvanceCommitEndTime = 0.0;
        NextStrafeSwapTime = 0.0;
        AttackPressure = 0.0f;
        bForceDefensiveEvasion = false;
        bWasAttackingLastTick = false;
        RepeatedAttackInputCount = 0;
        StrafeDirectionSign = FMath::RandBool() ? 1 : -1;
        ClearQueuedComboFollowUp();
    }

    RefreshFocusLockTarget(NewTarget);
}

void UHostileEnemyComponent::ClearHostileTarget()
{
    CurrentTarget = nullptr;
    bIsHostile = false;
    GuardReleaseTime = 0.0;
    NextDefenseDecisionTime = 0.0;
    DefensiveUrgencyEndTime = 0.0;
    RetreatEndTime = 0.0;
    FeintEndTime = 0.0;
    HesitationEndTime = 0.0;
    AdvanceCommitEndTime = 0.0;
    NextStrafeSwapTime = 0.0;
    AttackPressure = 0.0f;
    bForceDefensiveEvasion = false;
    bWasAttackingLastTick = false;
    RepeatedAttackInputCount = 0;
    ClearQueuedComboFollowUp();

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
    DefensiveUrgencyEndTime = WorldTime + 0.2;
    AttackPressure = FMath::Max(0.0f, AttackPressure - 0.18f);
    FeintEndTime = 0.0;
    AdvanceCommitEndTime = 0.0;
    NextDecisionTime = 0.0;
    NextDefenseDecisionTime = 0.0;
    bForceDefensiveEvasion = false;
    ClearQueuedComboFollowUp();

    if (bBecomeHostileOnDamage)
    {
        SetHostileTarget(DamageSpec.DamageCauser);
    }

    DebugDefenseEvent(TEXT("Hit received"));
}

void UHostileEnemyComponent::HandleOwnerEvasionFailed(ERPGEvasionType EvasionType, FString Reason)
{
    DebugDefenseEvent(FString::Printf(TEXT("Evasion failed: %s"), *Reason));
}

void UHostileEnemyComponent::UpdateHostileBehavior(float DeltaTime)
{
    if (!bIsHostile || !CachedCharacter.IsValid())
    {
        bWasAttackingLastTick = false;
        ClearQueuedComboFollowUp();
        return;
    }

    if (!IsTargetValid(CurrentTarget))
    {
        ClearHostileTarget();
        return;
    }

    if (CachedStats.IsValid() && CachedStats->IsDead())
    {
        bWasAttackingLastTick = false;
        ClearQueuedComboFollowUp();
        return;
    }

    if (CachedCombatState.IsValid())
    {
        if (CachedCombatState->IsInState(ERPGCombatState::Dead) || CachedCombatState->IsInState(ERPGCombatState::Hitstun))
        {
            bWasAttackingLastTick = false;
            ClearQueuedComboFollowUp();
            return;
        }
    }

    AActor* TargetActor = CurrentTarget;
    RefreshFocusLockTarget(TargetActor);
    UCombatStateComponent* TargetCombatState = TargetActor ? TargetActor->FindComponentByClass<UCombatStateComponent>() : nullptr;
    const ERPGCombatState TargetState = TargetCombatState ? TargetCombatState->GetCombatState() : ERPGCombatState::Idle;

    const FVector OwnerLocation = CachedCharacter->GetActorLocation();
    const FVector TargetLocation = TargetActor->GetActorLocation();
    FVector ToTarget = TargetLocation - OwnerLocation;
    ToTarget.Z = 0.0f;

    const float Distance = ToTarget.Size();
    const FVector Direction = ToTarget.GetSafeNormal();
    const double WorldTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
    const bool bThreatStartup = TargetState == ERPGCombatState::AttackStartup;
    const bool bThreatActive = bThreatStartup || TargetState == ERPGCombatState::AttackActive;
    const bool bIsPunishWindow = ShouldPunishTarget(TargetState, Distance);
    const bool bHesitating = IsHesitating(WorldTime);
    const bool bCommittedAdvance = WorldTime < AdvanceCommitEndTime;
    const bool bFeinting = WorldTime < FeintEndTime;
    const FVector StrafeDirection = GetStrafeDirection(Direction, WorldTime);
    const bool bIsCurrentlyAttacking = CachedAttackSystem.IsValid() && CachedAttackSystem->IsAttacking();

    auto FinishTick = [this, TargetState, bIsCurrentlyAttacking]()
    {
        LastObservedTargetState = TargetState;
        bWasAttackingLastTick = bIsCurrentlyAttacking;
    };

    if (bThreatStartup && LastObservedTargetState != ERPGCombatState::AttackStartup)
    {
        DefensiveUrgencyEndTime = WorldTime + 0.22;
        bForceDefensiveEvasion = ShouldPreferEvasionDefense() && Distance <= ThreatEvadeRange * 0.96f;
    }
    else if (!bThreatActive && WorldTime >= DefensiveUrgencyEndTime)
    {
        bForceDefensiveEvasion = false;
    }

    if (bFaceTargetWhileHostile
        && !Direction.IsNearlyZero()
        && !(CachedEvasion.IsValid() && CachedEvasion->bIsEvading))
    {
        const FRotator DesiredRotation = Direction.Rotation();
        const FRotator NewRotation = FMath::RInterpTo(CachedCharacter->GetActorRotation(), DesiredRotation, DeltaTime, FaceTargetInterpSpeed);
        CachedCharacter->SetActorRotation(NewRotation);

        if (AController* Controller = CachedCharacter->GetController())
        {
            Controller->SetControlRotation(NewRotation);
        }
    }

    const float PressureDelta = bIsPunishWindow ? PressureBuildPerSecond : -PressureDecayPerSecond;
    AttackPressure = FMath::Clamp(AttackPressure + (PressureDelta * DeltaTime), 0.0f, MaxPressureBonus);

    if (bIsPunishWindow || (Distance > AttackRange && !bThreatActive))
    {
        AdvanceCommitEndTime = FMath::Max(AdvanceCommitEndTime, WorldTime + AdvanceCommitDuration);
    }

    if (bIsCurrentlyAttacking && !bWasAttackingLastTick)
    {
        QueueComboFollowUp(bIsPunishWindow, Distance, WorldTime);
    }
    else if (!bIsCurrentlyAttacking && bWasAttackingLastTick)
    {
        ClearQueuedComboFollowUp();
    }

    if (bComboFollowUpQueued
        && bIsCurrentlyAttacking
        && CachedAttackSystem.IsValid()
        && CachedAttackSystem->bComboWindowOpen
        && !CachedAttackSystem->bSaveAttack
        && WorldTime >= QueuedComboBufferTime)
    {
        CachedAttackSystem->BufferComboInputByType(QueuedComboInputType);

        if (QueuedComboInputType == LastAttackInputType)
        {
            ++RepeatedAttackInputCount;
        }
        else
        {
            LastAttackInputType = QueuedComboInputType;
            RepeatedAttackInputCount = 0;
        }

        bComboFollowUpQueued = false;
    }

    if (bIsCurrentlyAttacking)
    {
        FinishTick();
        return;
    }

    if (TryStartParryAgainstTargetState(TargetState, Distance, WorldTime))
    {
        FinishTick();
        return;
    }

    const bool bPreferGuard = ShouldPreferGuardDefense();
    const bool bPreferEvasion = ShouldPreferEvasionDefense();

    if (bPreferGuard)
    {
        if (TryStartGuardAgainstTargetState(TargetState, Distance, WorldTime))
        {
            FinishTick();
            return;
        }

        if (TryStartDefensiveEvasion(TargetState, Direction, StrafeDirection, Distance, WorldTime))
        {
            FinishTick();
            return;
        }
    }
    else
    {
        if (TryStartDefensiveEvasion(TargetState, Direction, StrafeDirection, Distance, WorldTime))
        {
            FinishTick();
            return;
        }

        if (TryStartGuardAgainstTargetState(TargetState, Distance, WorldTime))
        {
            FinishTick();
            return;
        }
    }

    StopGuardIfNeeded(bThreatActive, WorldTime);

    const float MinDesiredCombatDistance = FMath::Max(PersonalSpaceDistance + 12.0f, RetreatDistance + 6.0f);
    const float MaxDesiredCombatDistance = FMath::Max(MinDesiredCombatDistance + 1.0f, ChaseStopDistance - 10.0f);
    const float DesiredCombatDistance = FMath::Clamp(
        PreferredCombatDistance - (bIsPunishWindow ? 10.0f : 0.0f) - (AttackPressure * 18.0f),
        MinDesiredCombatDistance,
        MaxDesiredCombatDistance);
    const float DistanceError = Distance - DesiredCombatDistance;
    const bool bInsideCombatBand = FMath::Abs(DistanceError) <= CombatDistanceTolerance;
    const float OrbitWeight = StrafeWeight * (bHesitating ? 0.75f : 1.0f + (AttackPressure * 0.8f));

    if (WorldTime < RetreatEndTime && !Direction.IsNearlyZero() && Distance < DesiredCombatDistance + CombatDistanceTolerance)
    {
        CachedCharacter->AddMovementInput(-Direction, 0.72f, true);
        CachedCharacter->AddMovementInput(StrafeDirection, OrbitWeight * 0.8f, true);
        FinishTick();
        return;
    }

    if (!Direction.IsNearlyZero())
    {
        float ForwardInput = 0.0f;
        float LateralInput = 0.0f;

        if (Distance < PersonalSpaceDistance)
        {
            ForwardInput = -0.92f;
            LateralInput = OrbitWeight * PersonalSpaceStrafeMultiplier;
            NextAttackTime = FMath::Max(NextAttackTime, WorldTime + 0.18);
        }
        else if (Distance < RetreatDistance)
        {
            ForwardInput = -0.44f;
            LateralInput = OrbitWeight * 1.1f;
        }
        else if (Distance > ChaseStopDistance)
        {
            ForwardInput = 0.92f;
            LateralInput = OrbitWeight * 0.15f;
        }
        else if (bCommittedAdvance || bIsPunishWindow)
        {
            ForwardInput = Distance > PersonalSpaceDistance + 12.0f ? 0.74f : -0.16f;
            LateralInput = OrbitWeight * 0.55f;
        }
        else if (DistanceError > CombatDistanceTolerance)
        {
            ForwardInput = 0.58f;
            LateralInput = OrbitWeight * 0.45f;
        }
        else if (DistanceError < -CombatDistanceTolerance)
        {
            ForwardInput = -0.34f;
            LateralInput = OrbitWeight * 1.05f;
        }
        else if (!bThreatActive)
        {
            const float PositionCorrection = FMath::Clamp(-DistanceError / FMath::Max(CombatDistanceTolerance, 1.0f), -0.16f, 0.18f);
            ForwardInput = PositionCorrection;
            LateralInput = OrbitWeight * 1.1f;
        }
        else
        {
            ForwardInput = Distance < DesiredCombatDistance ? -0.18f : 0.0f;
            LateralInput = OrbitWeight * 0.8f;
        }

        if (FMath::Abs(ForwardInput) > KINDA_SMALL_NUMBER)
        {
            CachedCharacter->AddMovementInput(Direction, ForwardInput, true);
        }

        if (FMath::Abs(LateralInput) > KINDA_SMALL_NUMBER)
        {
            CachedCharacter->AddMovementInput(StrafeDirection, LateralInput, true);
        }
    }

    if (!bThreatActive && !bFeinting && !bHesitating && bInsideCombatBand && !bCommittedAdvance && WorldTime >= NextDecisionTime)
    {
        NextDecisionTime = WorldTime + GetNextDecisionDelay();
        if (FMath::FRand() <= FeintChance)
        {
            FeintEndTime = WorldTime + FMath::FRandRange(MinFeintDuration, MaxFeintDuration);
            FinishTick();
            return;
        }
    }

    if (bFeinting)
    {
        if (!Direction.IsNearlyZero())
        {
            CachedCharacter->AddMovementInput(Direction, 0.18f, true);
            CachedCharacter->AddMovementInput(StrafeDirection, OrbitWeight * 1.2f, true);
        }

        FinishTick();
        return;
    }

    if (!CanAttackTarget() || Distance > AttackRange || Distance < PersonalSpaceDistance || !IsFacingTarget(Direction) || bHesitating)
    {
        FinishTick();
        return;
    }

    if (WorldTime < NextAttackTime || WorldTime < NextDecisionTime)
    {
        FinishTick();
        return;
    }

    const float AttackChance = GetAttackChance(bIsPunishWindow, Distance, WorldTime);
    NextDecisionTime = WorldTime + GetNextDecisionDelay();
    if (FMath::FRand() > AttackChance)
    {
        FinishTick();
        return;
    }

    const ERPGAttackInputType AttackInputType = ChooseAttackInputType(bIsPunishWindow, Distance);

    if (!Direction.IsNearlyZero())
    {
        const FRotator AttackFacing(0.0f, Direction.Rotation().Yaw, 0.0f);
        CachedCharacter->SetActorRotation(AttackFacing);
        if (AController* Controller = CachedCharacter->GetController())
        {
            Controller->SetControlRotation(AttackFacing);
        }
    }

    const bool bWasAttacking = CachedAttackSystem.IsValid() && CachedAttackSystem->IsAttacking();
    if (CachedAttackSystem.IsValid())
    {
        CachedAttackSystem->HandleAttackInputByType(AttackInputType);
    }

    if (CachedAttackSystem.IsValid() && !bWasAttacking && CachedAttackSystem->IsAttacking())
    {
        if (AttackInputType == LastAttackInputType)
        {
            ++RepeatedAttackInputCount;
        }
        else
        {
            LastAttackInputType = AttackInputType;
            RepeatedAttackInputCount = 0;
        }

        AttackPressure = FMath::Clamp(AttackPressure + 0.05f, 0.0f, MaxPressureBonus);
        AdvanceCommitEndTime = 0.0;
        NextAttackTime = WorldTime + AttackInterval + FMath::FRandRange(0.12f, 0.28f);
        NextDecisionTime = FMath::Max(NextDecisionTime, WorldTime + PostAttackDecisionDelay + FMath::FRandRange(0.06f, 0.14f));
        StartRetreat(WorldTime);
    }

    FinishTick();
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

void UHostileEnemyComponent::RefreshFocusLockTarget(AActor* TargetActor)
{
    if (!bMaintainFocusLockWhileHostile || !CachedTargetLock.IsValid())
    {
        return;
    }

    if (!IsValid(TargetActor))
    {
        CachedTargetLock->ClearLock();
        return;
    }

    if (CachedTargetLock->GetLockTarget() != TargetActor)
    {
        CachedTargetLock->SetLockTarget(TargetActor);
    }
}

bool UHostileEnemyComponent::TryStartParryAgainstTargetState(ERPGCombatState TargetState, float DistanceToTarget, double WorldTime)
{
    if (!CachedCombatState.IsValid())
    {
        return false;
    }

    const bool bThreatStartup = TargetState == ERPGCombatState::AttackStartup;
    const bool bNewThreatWindow = TargetState != LastObservedTargetState;
    if (!bThreatStartup || !bNewThreatWindow)
    {
        return false;
    }

    if (DistanceToTarget > FMath::Min(ParryThreatRange, ThreatGuardRange))
    {
        return false;
    }

    if (CachedCombatState->IsParryOnCooldown() || CachedCombatState->IsInState(ERPGCombatState::Hitstun) || CachedCombatState->IsInState(ERPGCombatState::Dead))
    {
        return false;
    }

    float ParryChance = ParryAgainstAttackChance;
    if (CachedCombatState->IsInState(ERPGCombatState::Guard))
    {
        ParryChance += GuardParryChance;
    }

    if (ShouldPreferGuardDefense())
    {
        ParryChance += 0.08f;
    }

    if (DistanceToTarget <= AttackRange * 0.9f)
    {
        ParryChance += 0.06f;
    }

    ParryChance += FMath::Min(AttackPressure * 0.25f, 0.1f);
    ParryChance = FMath::Clamp(ParryChance, 0.0f, 0.92f);
    if (FMath::FRand() > ParryChance)
    {
        return false;
    }

    bool bParryStarted = false;
    if (CachedCombatState->IsInState(ERPGCombatState::Guard))
    {
        bParryStarted = CachedCombatState->BeginParryAttempt();
    }
    else
    {
        bParryStarted = CachedCombatState->StartGuard();
    }

    if (!bParryStarted)
    {
        return false;
    }

    GuardReleaseTime = WorldTime + FMath::FRandRange(MinGuardHoldDuration * 0.8f, FMath::Max(MinGuardHoldDuration * 0.8f, MaxGuardHoldDuration * 0.9f));
    NextDefenseDecisionTime = WorldTime + GetNextDefenseDecisionDelay();
    NextAttackTime = FMath::Max(NextAttackTime, WorldTime + PostGuardAttackDelay + 0.08f);
    DebugDefenseEvent(TEXT("Parry attempt"));
    return true;
}

bool UHostileEnemyComponent::TryStartGuardAgainstTargetState(ERPGCombatState TargetState, float DistanceToTarget, double WorldTime)
{
    if (!CachedCombatState.IsValid())
    {
        return false;
    }

    const bool bThreatStartup = TargetState == ERPGCombatState::AttackStartup;
    const bool bThreatActive = bThreatStartup || TargetState == ERPGCombatState::AttackActive;
    const bool bUrgentDefense = WorldTime < DefensiveUrgencyEndTime;
    const bool bPreferGuard = ShouldPreferGuardDefense();
    const bool bFallbackFromUnavailableEvasion = !bPreferGuard && CachedEvasion.IsValid() && (CachedEvasion->bIsEvading || CachedEvasion->bDodgeOnCooldown);
    const float GuardRangeMultiplier = bUrgentDefense ? 1.12f : (bPreferGuard ? 1.08f : 1.0f);
    if ((!bThreatActive && !bUrgentDefense) || DistanceToTarget > (ThreatGuardRange * GuardRangeMultiplier))
    {
        return false;
    }

    const bool bNewThreatWindow = TargetState != LastObservedTargetState;
    const bool bMustGuardNow = bPreferGuard && bThreatStartup && bNewThreatWindow;
    const bool bShouldEvaluate = bMustGuardNow || bFallbackFromUnavailableEvasion || bUrgentDefense || bNewThreatWindow || WorldTime >= NextDefenseDecisionTime;
    if (!bShouldEvaluate)
    {
        return CachedCombatState->IsInState(ERPGCombatState::Guard);
    }

    NextDefenseDecisionTime = WorldTime + GetNextDefenseDecisionDelay();
    if (CachedCombatState->IsInState(ERPGCombatState::Guard))
    {
        GuardReleaseTime = WorldTime + FMath::FRandRange(MinGuardHoldDuration, MaxGuardHoldDuration);
        return true;
    }

    const float GuardChance = FMath::Clamp(GuardAgainstAttackChance + (bUrgentDefense ? 0.28f : 0.0f) + (bPreferGuard ? 0.18f : 0.0f) + (bFallbackFromUnavailableEvasion && bThreatStartup ? 0.22f : 0.0f), 0.0f, 0.995f);
    if (!bMustGuardNow && FMath::FRand() > GuardChance)
    {
        return false;
    }

    if (!CachedCombatState->StartGuard())
    {
        DebugDefenseEvent(TEXT("Guard failed"));
        return false;
    }

    DebugDefenseEvent(bMustGuardNow ? TEXT("Startup guard") : TEXT("Guard success"));
    GuardReleaseTime = WorldTime + FMath::FRandRange(MinGuardHoldDuration, MaxGuardHoldDuration);
    NextAttackTime = FMath::Max(NextAttackTime, WorldTime + PostGuardAttackDelay);
    return true;
}

bool UHostileEnemyComponent::TryStartDefensiveEvasion(ERPGCombatState TargetState, const FVector& DirectionToTarget, const FVector& StrafeDirection, float DistanceToTarget, double WorldTime)
{
    if (!CachedEvasion.IsValid() || DirectionToTarget.IsNearlyZero())
    {
        return false;
    }

    const bool bThreatStartup = TargetState == ERPGCombatState::AttackStartup;
    const bool bThreatActive = bThreatStartup || TargetState == ERPGCombatState::AttackActive;
    const bool bUrgentDefense = WorldTime < DefensiveUrgencyEndTime;
    const bool bPreferEvasion = ShouldPreferEvasionDefense();
    const float EvadeRangeMultiplier = bUrgentDefense ? 1.18f : (bPreferEvasion ? 1.12f : 1.0f);
    if ((!bThreatActive && !bUrgentDefense) || DistanceToTarget > (ThreatEvadeRange * EvadeRangeMultiplier))
    {
        return false;
    }

    if (CachedCombatState.IsValid() && CachedCombatState->IsInState(ERPGCombatState::Guard))
    {
        return false;
    }

    if (CachedEvasion->bIsEvading || CachedEvasion->bDodgeOnCooldown)
    {
        return false;
    }

    const bool bNewThreatWindow = TargetState != LastObservedTargetState;
    const bool bMustEvadeNow = (bForceDefensiveEvasion && bUrgentDefense) || (bPreferEvasion && bThreatStartup && bNewThreatWindow);
    if (!bMustEvadeNow && !bUrgentDefense && !bNewThreatWindow && WorldTime < NextDefenseDecisionTime)
    {
        return false;
    }

    NextDefenseDecisionTime = WorldTime + GetNextDefenseDecisionDelay();
    const float EvadeChance = FMath::Clamp(EvadeAgainstAttackChance + (bUrgentDefense ? 0.35f : 0.0f) + (bThreatStartup ? 0.18f : 0.0f) + (bPreferEvasion ? 0.18f : 0.0f), 0.0f, 0.99f);
    if (!bMustEvadeNow && FMath::FRand() > EvadeChance)
    {
        return false;
    }

    const bool bUseLateralDodge = !bMustEvadeNow && StrafeWeight >= 0.11f && !StrafeDirection.IsNearlyZero();
    if (bUseLateralDodge)
    {
        CachedCharacter->AddMovementInput(StrafeDirection, 1.0f, true);
        CachedCharacter->AddMovementInput(-DirectionToTarget, 0.25f, true);
    }
    else
    {
        CachedCharacter->AddMovementInput(-DirectionToTarget, 1.0f, true);
    }

    if (!CachedEvasion->StartDodge())
    {
        NextDefenseDecisionTime = WorldTime + FMath::Max(GetNextDefenseDecisionDelay(), 0.18f);
        DebugDefenseEvent(TEXT("Dodge failed"));
        return false;
    }

    DebugDefenseEvent(bMustEvadeNow ? TEXT("Startup dodge") : TEXT("Dodge success"));
    GuardReleaseTime = 0.0;
    FeintEndTime = 0.0;
    RetreatEndTime = WorldTime + 0.12;
    AdvanceCommitEndTime = 0.0;
    NextAttackTime = FMath::Max(NextAttackTime, WorldTime + PostGuardAttackDelay + 0.16f);
    ClearQueuedComboFollowUp();
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

float UHostileEnemyComponent::GetNextDefenseDecisionDelay() const
{
    const float MinDelay = FMath::Min(MinDecisionInterval, 0.08f);
    const float MaxDelay = FMath::Min(MaxDecisionInterval, 0.16f);
    return FMath::FRandRange(FMath::Max(0.02f, MinDelay), FMath::Max(FMath::Max(0.02f, MinDelay), MaxDelay));
}

float UHostileEnemyComponent::GetAttackChance(bool bIsPunishWindow, float DistanceToTarget, double WorldTime) const
{
    float AttackChance = bIsPunishWindow ? PunishAttackChance : BaseAttackChance;
    AttackChance += FMath::Min(AttackPressure, PressureAttackChanceBonus);

    if (DistanceToTarget <= AttackRange * 0.82f)
    {
        AttackChance += CloseRangeAttackChanceBonus;
    }

    if (DistanceToTarget < PersonalSpaceDistance)
    {
        AttackChance -= 0.22f;
    }

    if (RepeatedAttackInputCount > 0)
    {
        AttackChance -= FMath::Min(0.08f * static_cast<float>(RepeatedAttackInputCount), 0.18f);
    }

    const double TimeSinceDamage = WorldTime - RecentDamageTime;
    if (TimeSinceDamage >= 0.0 && TimeSinceDamage < RecentDamagePenaltyDuration)
    {
        const float PenaltyAlpha = 1.0f - FMath::Clamp(static_cast<float>(TimeSinceDamage / FMath::Max(RecentDamagePenaltyDuration, KINDA_SMALL_NUMBER)), 0.0f, 1.0f);
        AttackChance -= DamageNervesPenalty * PenaltyAlpha;
    }

    return FMath::Clamp(AttackChance, 0.08f, 0.99f);
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

bool UHostileEnemyComponent::HasUsableAttackInputType(ERPGAttackInputType InputType) const
{
    if (!CachedAttackSystem.IsValid())
    {
        return false;
    }

    if (const FRPGAttackStartRandomPool* Pool = CachedAttackSystem->RandomizedStartStagesByType.Find(InputType))
    {
        for (const int32 StageIndex : Pool->StageIndices)
        {
            if (CachedAttackSystem->AttackStages.IsValidIndex(StageIndex))
            {
                return true;
            }
        }
    }

    if (const int32* StartStage = CachedAttackSystem->AttackStartStageByType.Find(InputType))
    {
        return CachedAttackSystem->AttackStages.IsValidIndex(*StartStage);
    }

    return false;
}

ERPGAttackInputType UHostileEnemyComponent::ChooseAttackInputType(bool bIsPunishWindow, float DistanceToTarget)
{
    TArray<ERPGAttackInputType> Candidates;
    TArray<float> CandidateWeights;

    const float SafeAttackRange = FMath::Max(AttackRange, 1.0f);
    const float DistanceRatio = DistanceToTarget / SafeAttackRange;
    const bool bCloseRange = DistanceRatio <= 0.82f;
    const bool bFarEdge = DistanceRatio >= 0.9f;

    auto AddCandidate = [this, &Candidates, &CandidateWeights](ERPGAttackInputType InputType, float Weight)
    {
        if (Weight <= KINDA_SMALL_NUMBER || !HasUsableAttackInputType(InputType))
        {
            return;
        }

        float AdjustedWeight = Weight;
        if (InputType == LastAttackInputType)
        {
            const float RepeatPenalty = RepeatedAttackInputCount >= 2 ? 0.12f : 0.38f;
            AdjustedWeight *= RepeatPenalty;
        }

        Candidates.Add(InputType);
        CandidateWeights.Add(AdjustedWeight);
    };

    AddCandidate(ERPGAttackInputType::Light, 1.0f);
    AddCandidate(ERPGAttackInputType::LightSlash, bCloseRange ? 1.2f : 0.72f);
    AddCandidate(ERPGAttackInputType::LightStab, bFarEdge ? 1.18f : 0.76f);

    float HeavyWeight = bIsPunishWindow ? PunishHeavyAttackChance : HeavyAttackChance;
    HeavyWeight += AttackPressure * 0.9f;
    HeavyWeight += bFarEdge ? 0.1f : 0.0f;
    HeavyWeight -= bCloseRange ? 0.05f : 0.0f;
    AddCandidate(ERPGAttackInputType::Heavy, HeavyWeight);

    if (Candidates.Num() == 0)
    {
        return ERPGAttackInputType::Light;
    }

    float TotalWeight = 0.0f;
    for (const float Weight : CandidateWeights)
    {
        TotalWeight += FMath::Max(Weight, 0.0f);
    }

    if (TotalWeight <= KINDA_SMALL_NUMBER)
    {
        return Candidates[0];
    }

    float Selection = FMath::FRandRange(0.0f, TotalWeight);
    for (int32 Index = 0; Index < Candidates.Num(); ++Index)
    {
        Selection -= FMath::Max(CandidateWeights[Index], 0.0f);
        if (Selection <= 0.0f)
        {
            return Candidates[Index];
        }
    }

    return Candidates.Last();
}

void UHostileEnemyComponent::QueueComboFollowUp(bool bIsPunishWindow, float DistanceToTarget, double WorldTime)
{
    bComboFollowUpQueued = false;

    if (!CachedAttackSystem.IsValid())
    {
        return;
    }

    float FollowUpChance = bIsPunishWindow ? PunishComboFollowUpChance : ComboFollowUpChance;
    FollowUpChance += FMath::Min(AttackPressure * 0.35f, 0.16f);

    if (DistanceToTarget <= AttackRange * 0.8f)
    {
        FollowUpChance += 0.08f;
    }
    else if (DistanceToTarget > AttackRange * 1.05f)
    {
        FollowUpChance -= 0.12f;
    }

    FollowUpChance = FMath::Clamp(FollowUpChance, 0.0f, 0.92f);
    if (FMath::FRand() > FollowUpChance)
    {
        return;
    }

    QueuedComboInputType = ChooseAttackInputType(bIsPunishWindow, DistanceToTarget);
    QueuedComboBufferTime = WorldTime + FMath::FRandRange(0.06f, 0.14f);
    bComboFollowUpQueued = true;
}

void UHostileEnemyComponent::ClearQueuedComboFollowUp()
{
    bComboFollowUpQueued = false;
    QueuedComboBufferTime = 0.0;
    QueuedComboInputType = ERPGAttackInputType::Light;
}

bool UHostileEnemyComponent::IsHesitating(double WorldTime) const
{
    return WorldTime < HesitationEndTime;
}

bool UHostileEnemyComponent::ShouldPreferGuardDefense() const
{
    return GuardAgainstAttackChance >= 0.72f && EvadeAgainstAttackChance < 0.4f;
}

bool UHostileEnemyComponent::ShouldPreferEvasionDefense() const
{
    return EvadeAgainstAttackChance >= 0.38f && GuardAgainstAttackChance < 0.82f;
}

void UHostileEnemyComponent::DebugDefenseEvent(const FString& Message) const
{
#if !UE_BUILD_SHIPPING
    if (!GEngine || !GetOwner())
    {
        return;
    }

    const FString Prefix = FString::Printf(TEXT("%s: %s"), *GetNameSafe(GetOwner()), *Message);
    GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Yellow, Prefix);
#endif
}

#include "Components/TargetLockComponent.h"

#include "Components/PlayerStatsComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "Kismet/GameplayStatics.h"
#include "WorldCollision.h"

UTargetLockComponent::UTargetLockComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UTargetLockComponent::BeginPlay()
{
    Super::BeginPlay();
}

void UTargetLockComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!CurrentTarget.IsValid())
    {
        return;
    }

    const bool bTargetNoLongerValid = !IsValidLockTarget(CurrentTarget.Get()) || IsTargetDead(CurrentTarget.Get());
    if (bTargetNoLongerValid)
    {
        if (!bAutoRelockOnTargetLost || !LockBestTarget())
        {
            ClearLock();
        }

        return;
    }

    if (bAutoRotateToTarget)
    {
        RotateOwnerTowardTarget(DeltaTime);
    }

    if (bDriveControllerRotationWhenLocked)
    {
        UpdateControllerFacing(DeltaTime);
    }
}

bool UTargetLockComponent::ToggleLockOn()
{
    if (IsLockedOn())
    {
        ClearLock();
        return false;
    }

    return LockBestTarget();
}

bool UTargetLockComponent::LockBestTarget()
{
    return SetLockTarget(FindBestTarget());
}

void UTargetLockComponent::ClearLock()
{
    if (!CurrentTarget.IsValid())
    {
        return;
    }

    CurrentTarget = nullptr;
    OnLockTargetChanged.Broadcast(nullptr);
}

bool UTargetLockComponent::CycleTarget(bool bCycleRight)
{
    if (!GetOwner())
    {
        return false;
    }

    TArray<AActor*> Candidates;
    GatherCandidateTargets(Candidates);

    if (Candidates.Num() == 0)
    {
        ClearLock();
        return false;
    }

    const FVector OwnerLocation = GetOwner()->GetActorLocation();
    const FVector Forward = GetOwner()->GetActorForwardVector().GetSafeNormal2D();
    const FVector Right = GetOwner()->GetActorRightVector().GetSafeNormal2D();

    AActor* BestSideTarget = nullptr;
    float BestSideScore = -FLT_MAX;

    for (AActor* Candidate : Candidates)
    {
        if (!IsValid(Candidate) || Candidate == CurrentTarget.Get())
        {
            continue;
        }

        FVector ToCandidate = Candidate->GetActorLocation() - OwnerLocation;
        ToCandidate.Z = 0.0f;
        ToCandidate = ToCandidate.GetSafeNormal();
        if (ToCandidate.IsNearlyZero())
        {
            continue;
        }

        const float Side = FVector::DotProduct(Right, ToCandidate);
        if (bCycleRight && Side <= 0.0f)
        {
            continue;
        }

        if (!bCycleRight && Side >= 0.0f)
        {
            continue;
        }

        const float ForwardScore = FVector::DotProduct(Forward, ToCandidate);
        const float SideScore = FMath::Abs(Side);
        const float Score = ForwardScore + (SideScore * 0.25f);
        if (Score > BestSideScore)
        {
            BestSideScore = Score;
            BestSideTarget = Candidate;
        }
    }

    if (BestSideTarget)
    {
        return SetLockTarget(BestSideTarget);
    }

    return LockBestTarget();
}

bool UTargetLockComponent::SetLockTarget(AActor* NewTarget)
{
    if (!IsValid(NewTarget) || !IsValidLockTarget(NewTarget) || IsTargetDead(NewTarget))
    {
        return false;
    }

    CurrentTarget = NewTarget;
    OnLockTargetChanged.Broadcast(NewTarget);
    return true;
}

bool UTargetLockComponent::IsLockedOn() const
{
    return CurrentTarget.IsValid();
}

AActor* UTargetLockComponent::GetLockTarget() const
{
    return CurrentTarget.Get();
}

FVector UTargetLockComponent::GetLockTargetLocation() const
{
    return CurrentTarget.IsValid() ? CurrentTarget->GetActorLocation() : FVector::ZeroVector;
}

void UTargetLockComponent::GatherCandidateTargets(TArray<AActor*>& OutTargets) const
{
    OutTargets.Reset();

    if (!GetWorld() || !GetOwner())
    {
        return;
    }

    // Prefer tag-driven discovery so non-pawn enemies (e.g. actor-based dummies) can be locked.
    if (!TargetableTag.IsNone())
    {
        TArray<AActor*> TaggedTargets;
        UGameplayStatics::GetAllActorsWithTag(GetWorld(), TargetableTag, TaggedTargets);

        for (AActor* Candidate : TaggedTargets)
        {
            if (!IsValid(Candidate) || Candidate == GetOwner())
            {
                continue;
            }

            const float Distance = FVector::Dist2D(GetOwner()->GetActorLocation(), Candidate->GetActorLocation());
            if (Distance > SearchRadius)
            {
                continue;
            }

            if (IsValidLockTarget(Candidate) && !IsTargetDead(Candidate))
            {
                OutTargets.AddUnique(Candidate);
            }
        }

        if (OutTargets.Num() > 0)
        {
            return;
        }
    }

    TArray<FOverlapResult> Overlaps;
    FCollisionObjectQueryParams ObjectQueryParams;
    ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);
    ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);

    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(TargetLockQuery), false, GetOwner());

    const bool bAnyOverlap = GetWorld()->OverlapMultiByObjectType(
        Overlaps,
        GetOwner()->GetActorLocation(),
        FQuat::Identity,
        ObjectQueryParams,
        FCollisionShape::MakeSphere(SearchRadius),
        QueryParams);

    if (!bAnyOverlap)
    {
        return;
    }

    for (const FOverlapResult& Overlap : Overlaps)
    {
        AActor* Candidate = Overlap.GetActor();
        if (IsValidLockTarget(Candidate) && !IsTargetDead(Candidate))
        {
            OutTargets.AddUnique(Candidate);
        }
    }
}

AActor* UTargetLockComponent::FindBestTarget() const
{
    if (!GetOwner())
    {
        return nullptr;
    }

    TArray<AActor*> Candidates;
    GatherCandidateTargets(Candidates);

    if (Candidates.Num() == 0)
    {
        return nullptr;
    }

    const FVector OwnerLocation = GetOwner()->GetActorLocation();

    FVector Forward = GetOwner()->GetActorForwardVector();
    if (const ACharacter* CharacterOwner = Cast<ACharacter>(GetOwner()))
    {
        if (const AController* Controller = CharacterOwner->GetController())
        {
            Forward = Controller->GetControlRotation().Vector();
        }
    }

    Forward.Z = 0.0f;
    Forward = Forward.GetSafeNormal();

    const float MinDot = FMath::Cos(FMath::DegreesToRadians(MaxLockAngleDegrees));

    AActor* BestTarget = nullptr;
    float BestScore = -FLT_MAX;

    AActor* NearestTarget = nullptr;
    float NearestDistance = FLT_MAX;

    for (AActor* Candidate : Candidates)
    {
        if (!IsValid(Candidate))
        {
            continue;
        }

        FVector ToCandidate = Candidate->GetActorLocation() - OwnerLocation;
        const float Distance = ToCandidate.Size2D();
        if (Distance > MaxLockDistance || Distance <= KINDA_SMALL_NUMBER)
        {
            continue;
        }

        if (Distance < NearestDistance)
        {
            NearestDistance = Distance;
            NearestTarget = Candidate;
        }

        ToCandidate.Z = 0.0f;
        ToCandidate = ToCandidate.GetSafeNormal();

        const float Dot = FVector::DotProduct(Forward, ToCandidate);
        if (Dot < MinDot)
        {
            continue;
        }

        const float DistanceScore = 1.0f - (Distance / MaxLockDistance);
        const float Score = (Dot * 0.75f) + (DistanceScore * 0.25f);

        if (Score > BestScore)
        {
            BestScore = Score;
            BestTarget = Candidate;
        }
    }

    return BestTarget ? BestTarget : NearestTarget;
}

bool UTargetLockComponent::IsValidLockTarget(AActor* Candidate) const
{
    if (!IsValid(Candidate) || Candidate == GetOwner())
    {
        return false;
    }

    if (!TargetableTag.IsNone() && !Candidate->ActorHasTag(TargetableTag))
    {
        return false;
    }

    if (!GetOwner())
    {
        return false;
    }

    if (bBreakLockIfOutOfRange)
    {
        const float Distance = FVector::Dist2D(GetOwner()->GetActorLocation(), Candidate->GetActorLocation());
        if (Distance > BreakLockDistance)
        {
            return false;
        }
    }

    const float Distance = FVector::Dist2D(GetOwner()->GetActorLocation(), Candidate->GetActorLocation());
    if (Distance > MaxLockDistance)
    {
        return false;
    }

    if (bRequireLineOfSight && !HasLineOfSightTo(Candidate))
    {
        return false;
    }

    return true;
}

bool UTargetLockComponent::HasLineOfSightTo(AActor* Candidate) const
{
    if (!GetWorld() || !GetOwner() || !IsValid(Candidate))
    {
        return false;
    }

    FHitResult HitResult;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(TargetLockLOS), false, GetOwner());
    Params.AddIgnoredActor(GetOwner());

    const FVector Start = GetOwner()->GetActorLocation() + FVector(0.0f, 0.0f, 50.0f);
    const FVector End = Candidate->GetActorLocation() + FVector(0.0f, 0.0f, 50.0f);

    const bool bBlocked = GetWorld()->LineTraceSingleByChannel(
        HitResult,
        Start,
        End,
        LineOfSightChannel,
        Params);

    if (!bBlocked)
    {
        return true;
    }

    return HitResult.GetActor() == Candidate;
}

bool UTargetLockComponent::IsTargetDead(AActor* Candidate) const
{
    if (!IsValid(Candidate))
    {
        return true;
    }

    if (const UPlayerStatsComponent* Stats = Candidate->FindComponentByClass<UPlayerStatsComponent>())
    {
        return Stats->IsDead();
    }

    return false;
}

void UTargetLockComponent::RotateOwnerTowardTarget(float DeltaTime)
{
    if (!GetOwner() || !CurrentTarget.IsValid())
    {
        return;
    }

    FVector ToTarget = CurrentTarget->GetActorLocation() - GetOwner()->GetActorLocation();
    ToTarget.Z = 0.0f;

    if (ToTarget.IsNearlyZero())
    {
        return;
    }

    const FRotator CurrentRotation = GetOwner()->GetActorRotation();
    const FRotator DesiredRotation = ToTarget.Rotation();
    const FRotator NewRotation = FMath::RInterpTo(CurrentRotation, DesiredRotation, DeltaTime, RotationInterpSpeed);

    GetOwner()->SetActorRotation(NewRotation);
}

void UTargetLockComponent::UpdateControllerFacing(float DeltaTime)
{
    if (!CurrentTarget.IsValid())
    {
        return;
    }

    ACharacter* CharacterOwner = Cast<ACharacter>(GetOwner());
    if (!CharacterOwner)
    {
        return;
    }

    AController* Controller = CharacterOwner->GetController();
    if (!Controller)
    {
        return;
    }

    FVector ToTarget = CurrentTarget->GetActorLocation() - GetOwner()->GetActorLocation();
    ToTarget.Z = 0.0f;

    if (ToTarget.IsNearlyZero())
    {
        return;
    }

    const FRotator CurrentControlRotation = Controller->GetControlRotation();

    // Preserve player camera pitch/roll; lock-on only steers yaw toward the target.
    const FRotator DesiredControlRotation(
        CurrentControlRotation.Pitch,
        ToTarget.Rotation().Yaw,
        CurrentControlRotation.Roll);

    const FRotator NewControlRotation = FMath::RInterpTo(CurrentControlRotation, DesiredControlRotation, DeltaTime, ControllerRotationInterpSpeed);

    Controller->SetControlRotation(NewControlRotation);
}


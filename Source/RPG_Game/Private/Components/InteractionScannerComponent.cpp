#include "Components/InteractionScannerComponent.h"

#include "Engine/HitResult.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Interaction/RPGInteractable.h"

namespace
{
AActor* FindBestInteractableFromHits(const TArray<FHitResult>& Hits, AActor* Owner, const FVector& ScoreOrigin)
{
    AActor* BestInteractable = nullptr;
    float BestDistanceSq = TNumericLimits<float>::Max();

    for (const FHitResult& Hit : Hits)
    {
        AActor* Candidate = Hit.GetActor();
        if (!Candidate || Candidate == Owner || !Candidate->GetClass()->ImplementsInterface(URPGInteractable::StaticClass()))
        {
            continue;
        }

        if (!IRPGInteractable::Execute_CanInteract(Candidate, Owner))
        {
            continue;
        }

        const FVector HitLocation = Hit.ImpactPoint.IsNearlyZero() ? Candidate->GetActorLocation() : FVector(Hit.ImpactPoint);
        const float DistanceSq = FVector::DistSquared(ScoreOrigin, HitLocation);
        if (DistanceSq < BestDistanceSq)
        {
            BestDistanceSq = DistanceSq;
            BestInteractable = Candidate;
        }
    }

    return BestInteractable;
}

AActor* FindBestInteractableFromOverlaps(const TArray<FOverlapResult>& Overlaps, AActor* Owner, const FVector& ScoreOrigin)
{
    AActor* BestInteractable = nullptr;
    float BestDistanceSq = TNumericLimits<float>::Max();

    for (const FOverlapResult& Overlap : Overlaps)
    {
        AActor* Candidate = Overlap.GetActor();
        if (!Candidate || Candidate == Owner || !Candidate->GetClass()->ImplementsInterface(URPGInteractable::StaticClass()))
        {
            continue;
        }

        if (!IRPGInteractable::Execute_CanInteract(Candidate, Owner))
        {
            continue;
        }

        const float DistanceSq = FVector::DistSquared(ScoreOrigin, Candidate->GetActorLocation());
        if (DistanceSq < BestDistanceSq)
        {
            BestDistanceSq = DistanceSq;
            BestInteractable = Candidate;
        }
    }

    return BestInteractable;
}
}

UInteractionScannerComponent::UInteractionScannerComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UInteractionScannerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (bRefreshContinuously)
    {
        RefreshCurrentInteractable();
    }
}

bool UInteractionScannerComponent::TryInteract()
{
    AActor* Target = FindBestInteractable();
    UpdateCurrentInteractable(Target);

    if (!Target)
    {
        return false;
    }

    IRPGInteractable::Execute_Interact(Target, GetOwner());
    RefreshCurrentInteractable();
    return true;
}

void UInteractionScannerComponent::RefreshCurrentInteractable()
{
    UpdateCurrentInteractable(FindBestInteractable());
}

AActor* UInteractionScannerComponent::FindBestInteractable() const
{
    AActor* Owner = GetOwner();
    UWorld* World = GetWorld();
    if (!Owner || !World)
    {
        return nullptr;
    }

    FVector TraceStart = FVector::ZeroVector;
    FRotator TraceRotation = FRotator::ZeroRotator;

    if (const APawn* OwnerPawn = Cast<APawn>(Owner))
    {
        if (AController* Controller = OwnerPawn->GetController())
        {
            Controller->GetPlayerViewPoint(TraceStart, TraceRotation);
        }
    }

    if (TraceStart.IsNearlyZero())
    {
        Owner->GetActorEyesViewPoint(TraceStart, TraceRotation);
    }

    const FVector TraceEnd = TraceStart + (TraceRotation.Vector() * TraceDistance);
    const FCollisionShape TraceShape = FCollisionShape::MakeSphere(TraceRadius);

    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(RPGInteractionTrace), false, Owner);

    TArray<FHitResult> Hits;
    if (World->SweepMultiByChannel(Hits, TraceStart, TraceEnd, FQuat::Identity, TraceChannel, TraceShape, QueryParams))
    {
        if (AActor* BestFromTrace = FindBestInteractableFromHits(Hits, Owner, TraceStart))
        {
            return BestFromTrace;
        }
    }

    const float NearbyRadius = FMath::Max(TraceDistance, TraceRadius * 2.0f);
    const FCollisionShape NearbyShape = FCollisionShape::MakeSphere(NearbyRadius);
    const FVector NearbyOrigin = Owner->GetActorLocation();

    TArray<FOverlapResult> Overlaps;
    if (!World->OverlapMultiByChannel(Overlaps, NearbyOrigin, FQuat::Identity, TraceChannel, NearbyShape, QueryParams))
    {
        return nullptr;
    }

    return FindBestInteractableFromOverlaps(Overlaps, Owner, NearbyOrigin);
}

FText UInteractionScannerComponent::GetCurrentInteractionLabel() const
{
    if (AActor* Target = CurrentInteractable.Get())
    {
        if (Target->GetClass()->ImplementsInterface(URPGInteractable::StaticClass()))
        {
            return IRPGInteractable::Execute_GetInteractionLabel(Target);
        }
    }

    return FText::GetEmpty();
}

void UInteractionScannerComponent::UpdateCurrentInteractable(AActor* NewInteractable)
{
    if (CurrentInteractable.Get() == NewInteractable)
    {
        return;
    }

    CurrentInteractable = NewInteractable;
    OnCurrentInteractableChanged.Broadcast(NewInteractable);
}

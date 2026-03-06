#include "Components/AttackSystemComponent.h"

#include "Animation/AnimInstance.h"
#include "Components/CombatStateComponent.h"
#include "Components/PlayerStatsComponent.h"
#include "Components/SceneComponent.h"
#include "Components/TargetLockComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "TimerManager.h"

UAttackSystemComponent::UAttackSystemComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UAttackSystemComponent::BeginPlay()
{
    Super::BeginPlay();

    CachedCharacter = Cast<ACharacter>(GetOwner());
    CachedStats = GetOwner() ? GetOwner()->FindComponentByClass<UPlayerStatsComponent>() : nullptr;
    CachedTargetLock = GetOwner() ? GetOwner()->FindComponentByClass<UTargetLockComponent>() : nullptr;
    CachedCombatState = GetOwner() ? GetOwner()->FindComponentByClass<UCombatStateComponent>() : nullptr;

    if (AttackStages.Num() == 0)
    {
        AttackStages.Add(FRPGAttackStage());
    }
}

void UAttackSystemComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!bEnableAttackFacingAssist || !bIsAttacking)
    {
        return;
    }

    UpdateAttackFacing(DeltaTime);
}

void UAttackSystemComponent::HandleAttackInput()
{
    // During an active swing, input should buffer the next combo stage.
    if (bIsAttacking)
    {
        BufferComboInput();
        return;
    }

    if (!CanStartAttack())
    {
        return;
    }

    StartAttackStage(AttackIndex);
}

void UAttackSystemComponent::BufferComboInput()
{
    if (!bIsAttacking)
    {
        return;
    }

    if (bUseComboWindowLock && !bComboWindowOpen)
    {
        return;
    }

    bSaveAttack = true;

    if (GetWorld())
    {
        const float BufferDuration = FMath::Max(ComboInputBufferDuration, 0.01f);
        GetWorld()->GetTimerManager().ClearTimer(ComboBufferTimerHandle);
        GetWorld()->GetTimerManager().SetTimer(ComboBufferTimerHandle, this, &UAttackSystemComponent::ClearBufferedComboInput, BufferDuration, false);
    }
}

void UAttackSystemComponent::ContinueComboOrStop()
{
    if (bSaveAttack && AttackStages.IsValidIndex(AttackIndex + 1))
    {
        ++AttackIndex;
        bSaveAttack = false;

        if (GetWorld())
        {
            GetWorld()->GetTimerManager().ClearTimer(ComboBufferTimerHandle);
        }

        // Allow the chained stage to start from notify timing.
        bCanAttack = true;
        StartAttackStage(AttackIndex);
        return;
    }

    StopCombo();
}

void UAttackSystemComponent::StopCombo()
{
    bIsAttacking = false;
    bCanAttack = true;
    bSaveAttack = false;
    bComboWindowOpen = false;
    AttackIndex = 0;

    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(ComboResetTimerHandle);
        GetWorld()->GetTimerManager().ClearTimer(ComboBufferTimerHandle);
    }

    if (CachedCombatState.IsValid() && !CachedCombatState->IsInState(ERPGCombatState::Dead))
    {
        CachedCombatState->RequestState(ERPGCombatState::Idle);
    }

    OnAttackEnded.Broadcast(AttackIndex);
}

void UAttackSystemComponent::OpenComboWindow()
{
    if (!bIsAttacking)
    {
        return;
    }

    bComboWindowOpen = true;
}

void UAttackSystemComponent::CloseComboWindow()
{
    bComboWindowOpen = false;
}

void UAttackSystemComponent::StartTrace(USceneComponent* InTraceStart, USceneComponent* InTraceEnd)
{
    // Ignore duplicate begin-notify calls during one active trace window.
    if (bTraceActive)
    {
        return;
    }

    if (!InTraceStart || !InTraceEnd)
    {
        return;
    }

    TraceStartComponent = InTraceStart;
    TraceEndComponent = InTraceEnd;
    ResetHitActors();

    if (!GetWorld())
    {
        return;
    }

    bTraceActive = true;

    if (CachedCombatState.IsValid())
    {
        CachedCombatState->RequestState(ERPGCombatState::AttackActive);
    }

    GetWorld()->GetTimerManager().ClearTimer(TraceTimerHandle);

    // Fire one trace immediately so short notify windows do not miss the first contact frame.
    TickTrace();

    const float SafeInterval = FMath::Max(TraceInterval, 0.001f);
    GetWorld()->GetTimerManager().SetTimer(TraceTimerHandle, this, &UAttackSystemComponent::TickTrace, SafeInterval, true);
}

void UAttackSystemComponent::StopTrace()
{
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(TraceTimerHandle);
    }

    bTraceActive = false;

    if (bIsAttacking && CachedCombatState.IsValid() && !CachedCombatState->IsInState(ERPGCombatState::Dead))
    {
        CachedCombatState->RequestState(ERPGCombatState::AttackRecovery);
    }
}

void UAttackSystemComponent::ResetHitActors()
{
    HitActorsThisSwing.Reset();
}

bool UAttackSystemComponent::CanStartAttack() const
{
    if (!bCanAttack || !AttackStages.IsValidIndex(AttackIndex))
    {
        return false;
    }

    if (CachedCombatState.IsValid())
    {
        return CachedCombatState->CanTransitionTo(ERPGCombatState::AttackStartup);
    }

    return true;
}

bool UAttackSystemComponent::ConsumeStaminaForCurrentStage()
{
    if (!CachedStats.IsValid() || !AttackStages.IsValidIndex(AttackIndex))
    {
        return true;
    }

    const float Cost = AttackStages[AttackIndex].StaminaCost;
    if (Cost <= 0.0f)
    {
        return true;
    }

    const bool bOutOfStamina = CachedStats->DecreaseStamina(Cost);
    return !bOutOfStamina;
}

void UAttackSystemComponent::ClearBufferedComboInput()
{
    bSaveAttack = false;
}

void UAttackSystemComponent::StartAttackStage(int32 StageIndex)
{
    if (!AttackStages.IsValidIndex(StageIndex))
    {
        return;
    }

    // First stage requires bCanAttack; chained stages are enabled in ContinueComboOrStop.
    if (!CanStartAttack())
    {
        return;
    }

    if (CachedCombatState.IsValid() && !CachedCombatState->RequestState(ERPGCombatState::AttackStartup))
    {
        return;
    }

    AttackIndex = StageIndex;
    if (!ConsumeStaminaForCurrentStage())
    {
        StopCombo();
        return;
    }

    bCanAttack = false;
    bIsAttacking = true;
    bSaveAttack = false;
    bComboWindowOpen = !bUseComboWindowLock;

    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(ComboBufferTimerHandle);
    }

    ResetHitActors();
    OnAttackStarted.Broadcast(AttackIndex);

    if (CachedCharacter.IsValid())
    {
        if (UAnimInstance* AnimInstance = CachedCharacter->GetMesh() ? CachedCharacter->GetMesh()->GetAnimInstance() : nullptr)
        {
            if (AttackStages[AttackIndex].Montage)
            {
                AnimInstance->Montage_Play(AttackStages[AttackIndex].Montage);
            }
        }
    }

    if (GetWorld())
    {
        const float ResetDelay = FMath::Max(AttackStages[AttackIndex].ComboResetDelay, 0.01f);
        GetWorld()->GetTimerManager().ClearTimer(ComboResetTimerHandle);
        GetWorld()->GetTimerManager().SetTimer(ComboResetTimerHandle, this, &UAttackSystemComponent::ResetComboState, ResetDelay, false);
    }
}

void UAttackSystemComponent::ResetComboState()
{
    // Notify-based combo chains should call ContinueComboOrStop before this fires.
    StopCombo();
}

void UAttackSystemComponent::UpdateAttackFacing(float DeltaTime)
{
    if (!CachedCharacter.IsValid() || !GetOwner())
    {
        return;
    }

    const FVector OwnerLocation = GetOwner()->GetActorLocation();
    const FRotator CurrentRotation = GetOwner()->GetActorRotation();

    if (bFaceLockTargetDuringAttack && CachedTargetLock.IsValid() && CachedTargetLock->IsLockedOn())
    {
        const FVector TargetLocation = CachedTargetLock->GetLockTargetLocation();
        FVector ToTarget = TargetLocation - OwnerLocation;
        ToTarget.Z = 0.0f;

        if (!ToTarget.IsNearlyZero())
        {
            const FRotator DesiredRotation = ToTarget.Rotation();
            const FRotator NewRotation = FMath::RInterpTo(CurrentRotation, DesiredRotation, DeltaTime, AttackFacingInterpSpeed);
            GetOwner()->SetActorRotation(NewRotation);
        }

        return;
    }

    if (bFaceControllerYawWhenNoLockTarget)
    {
        if (AController* Controller = CachedCharacter->GetController())
        {
            const FRotator ControlRotation = Controller->GetControlRotation();
            const FRotator DesiredRotation(0.0f, ControlRotation.Yaw, 0.0f);
            const FRotator NewRotation = FMath::RInterpTo(CurrentRotation, DesiredRotation, DeltaTime, AttackFacingInterpSpeed);
            GetOwner()->SetActorRotation(NewRotation);
        }
    }
}

void UAttackSystemComponent::TickTrace()
{
    if (!TraceStartComponent.IsValid() || !TraceEndComponent.IsValid())
    {
        return;
    }

    const FVector Start = TraceStartComponent->GetComponentLocation();
    const FVector End = TraceEndComponent->GetComponentLocation();

    TArray<FHitResult> HitResults;
    TArray<AActor*> ActorsToIgnore;
    if (GetOwner())
    {
        ActorsToIgnore.Add(GetOwner());
    }

    const bool bAnyHit = UKismetSystemLibrary::SphereTraceMulti(
        this,
        Start,
        End,
        TraceRadius,
        UEngineTypes::ConvertToTraceType(TraceChannel),
        false,
        ActorsToIgnore,
        EDrawDebugTrace::None,
        HitResults,
        true);

    if (!bAnyHit)
    {
        return;
    }

    for (const FHitResult& Hit : HitResults)
    {
        AActor* HitActor = Hit.GetActor();
        if (!IsValid(HitActor))
        {
            continue;
        }

        const TWeakObjectPtr<AActor> HitActorPtr(HitActor);
        if (HitActorsThisSwing.Contains(HitActorPtr))
        {
            continue;
        }

        if (!DamageableTag.IsNone() && !HitActor->ActorHasTag(DamageableTag))
        {
            continue;
        }

        HitActorsThisSwing.Add(HitActorPtr);

        const float Damage = AttackStages.IsValidIndex(AttackIndex) ? AttackStages[AttackIndex].Damage : 10.0f;
        UGameplayStatics::ApplyDamage(
            HitActor,
            Damage,
            CachedCharacter.IsValid() ? CachedCharacter->GetController() : nullptr,
            GetOwner(),
            nullptr);

        OnAttackHit.Broadcast(HitActor);
    }
}

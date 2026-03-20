#include "Components/AttackSystemComponent.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/RPGAnimNotifyState_AttackTraceWindow.h"
#include "Components/CombatStateComponent.h"
#include "Components/PlayerStatsComponent.h"
#include "Components/SceneComponent.h"
#include "Components/TargetLockComponent.h"
#include "Components/LocomotionComponent.h"
#include "Data/RPGCombatMovesetDataAsset.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "TimerManager.h"

namespace
{
bool ShouldUseFallbackTrace(
    const TWeakObjectPtr<USceneComponent>& TraceStartComponent,
    const TWeakObjectPtr<USceneComponent>& TraceEndComponent,
    const AActor* OwnerActor)
{
    if (!TraceStartComponent.IsValid() || !TraceEndComponent.IsValid())
    {
        return true;
    }

    const FVector StartLocation = TraceStartComponent->GetComponentLocation();
    const FVector EndLocation = TraceEndComponent->GetComponentLocation();
    if (FVector::DistSquared(StartLocation, EndLocation) > FMath::Square(5.0f))
    {
        return false;
    }

    if (!IsValid(OwnerActor))
    {
        return true;
    }

    const FVector OwnerLocation = OwnerActor->GetActorLocation();
    const float StartToOwnerSq = FVector::DistSquared(StartLocation, OwnerLocation);
    const float EndToOwnerSq = FVector::DistSquared(EndLocation, OwnerLocation);
    return StartToOwnerSq <= FMath::Square(30.0f) && EndToOwnerSq <= FMath::Square(30.0f);
}
}
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
    CachedMoveComp = CachedCharacter.IsValid() ? CachedCharacter->GetCharacterMovement() : nullptr;
    CachedLocomotion = GetOwner() ? GetOwner()->FindComponentByClass<ULocomotionComponent>() : nullptr;
    SavedWalkSpeed = CachedMoveComp.IsValid() ? CachedMoveComp->MaxWalkSpeed : 0.0f;
    ResolveDefaultTraceComponents();

    if (bLoadMovesetOnBeginPlay && AttackMoveset)
    {
        ApplyAttackMovesetInternal(AttackMoveset);
    }

    if (AttackStages.Num() == 0)
    {
        AttackStages.Add(FRPGAttackStage());
    }

    if (CachedCombatState.IsValid())
    {
        CachedCombatState->OnCombatStateChanged.RemoveDynamic(this, &UAttackSystemComponent::HandleCombatStateChanged);
        CachedCombatState->OnCombatStateChanged.AddDynamic(this, &UAttackSystemComponent::HandleCombatStateChanged);
    }
}


void UAttackSystemComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (bPostAttackSpeedRecoveryActive)
    {
        if (!CachedMoveComp.IsValid() && CachedCharacter.IsValid())
        {
            CachedMoveComp = CachedCharacter->GetCharacterMovement();
        }

        if (!CachedMoveComp.IsValid())
        {
            bPostAttackSpeedRecoveryActive = false;
        }
        else
        {
            const float SafeRecoveryDuration = FMath::Max(PostAttackSpeedRecoveryDuration, 0.01f);
            PostAttackSpeedRecoveryElapsed = FMath::Min(PostAttackSpeedRecoveryElapsed + DeltaTime, SafeRecoveryDuration);
            const float Alpha = FMath::Clamp(PostAttackSpeedRecoveryElapsed / SafeRecoveryDuration, 0.0f, 1.0f);
            const float EasedAlpha = FMath::InterpEaseOut(0.0f, 1.0f, Alpha, 2.0f);
            CachedMoveComp->MaxWalkSpeed = FMath::Lerp(PostAttackSpeedRecoveryStartSpeed, PostAttackSpeedRecoveryTargetSpeed, EasedAlpha);

            if (Alpha >= 1.0f - KINDA_SMALL_NUMBER)
            {
                bPostAttackSpeedRecoveryActive = false;
            }
        }
    }

    if (bPrimaryAttackHeld)
    {
        PrimaryAttackHeldDuration += DeltaTime;

        const bool bShouldTriggerHeldHeavy = !bPrimaryAttackConsumedByHold
            && PrimaryAttackHeldDuration >= PrimaryHeavyHoldThreshold
            && HasAttackStartForInputType(ERPGAttackInputType::Heavy);

        if (bShouldTriggerHeldHeavy)
        {
            bPrimaryAttackConsumedByHold = true;
            HandleAttackInputByType(ERPGAttackInputType::Heavy);
        }
    }

    if (!bEnableAttackFacingAssist || !bIsAttacking)
    {
        return;
    }

    UpdateAttackFacing(DeltaTime);
}

void UAttackSystemComponent::ApplyAttackMoveset(URPGCombatMovesetDataAsset* InMoveset, bool bResetComboState)
{
    if (!InMoveset)
    {
        return;
    }

    AttackMoveset = InMoveset;
    ApplyAttackMovesetInternal(InMoveset);

    if (bResetComboState)
    {
        StopCombo();
    }
    else
    {
        AttackIndex = FMath::Clamp(AttackIndex, 0, FMath::Max(0, AttackStages.Num() - 1));
    }
}

void UAttackSystemComponent::SetWeaponAttackTuning(float InDamageMultiplier, float InStaminaMultiplier)
{
    WeaponDamageMultiplier = FMath::Max(0.01f, InDamageMultiplier);
    WeaponStaminaCostMultiplier = FMath::Max(0.01f, InStaminaMultiplier);
}

void UAttackSystemComponent::ApplyAttackMovesetInternal(const URPGCombatMovesetDataAsset* InMoveset)
{
    if (!InMoveset)
    {
        return;
    }

    AttackStages = InMoveset->AttackStages;
    AttackStartStageByType = InMoveset->AttackStartStageByType;
    RandomizedStartStagesByType = InMoveset->RandomizedStartStagesByType;
    LastRandomStartStageByType.Reset();
    bUseComboWindowLock = InMoveset->bUseComboWindowLock;
    bAllowSequentialComboFallback = InMoveset->bAllowSequentialComboFallback;
    AttackContinuationMode = InMoveset->AttackContinuationMode;
    ComboInputBufferDuration = FMath::Max(InMoveset->ComboInputBufferDuration, 0.01f);

    if (AttackStages.Num() == 0)
    {
        AttackStages.Add(FRPGAttackStage());
    }
}

void UAttackSystemComponent::HandleAttackInput()
{
    HandleAttackInputByType(ResolvePrimaryAttackInputType());
}

void UAttackSystemComponent::HandleAttackInputByType(ERPGAttackInputType InputType)
{
    if (bIsAttacking)
    {
        BufferComboInputByType(InputType);
        return;
    }

    if (!CanStartAttack())
    {
        return;
    }

    const int32 StartStage = ResolveComboStartStage(InputType);
    if (!AttackStages.IsValidIndex(StartStage))
    {
        return;
    }

    StartAttackStage(StartStage, InputType);
}

void UAttackSystemComponent::HandlePrimaryAttackPressed()
{
    bPrimaryAttackHeld = true;
    bPrimaryAttackConsumedByHold = false;
    PrimaryAttackHeldDuration = 0.0f;
}

void UAttackSystemComponent::HandlePrimaryAttackReleased()
{
    const bool bShouldUseHeldLight = !bPrimaryAttackConsumedByHold;

    bPrimaryAttackHeld = false;
    bPrimaryAttackConsumedByHold = false;
    PrimaryAttackHeldDuration = 0.0f;

    if (bShouldUseHeldLight)
    {
        HandleAttackInputByType(ResolvePrimaryAttackInputType());
    }
}

bool UAttackSystemComponent::BeginChargeAttack()
{
    return false;
}

bool UAttackSystemComponent::ReleaseChargeAttack()
{
    return false;
}

void UAttackSystemComponent::CancelChargeAttack()
{
}

float UAttackSystemComponent::GetCurrentChargeTime() const
{
    return 0.0f;
}

float UAttackSystemComponent::GetCurrentChargeRatio() const
{
    return 0.0f;
}

void UAttackSystemComponent::BufferComboInput()
{
    BufferComboInputByType(ERPGAttackInputType::Light);
}

void UAttackSystemComponent::BufferComboInputByType(ERPGAttackInputType InputType)
{
    if (!bIsAttacking)
    {
        return;
    }

    if (bUseComboWindowLock && !bComboWindowOpen)
    {
        return;
    }

    BufferedInputType = InputType;
    bSaveAttack = true;

    if (!bKeepBufferedInputUntilConsumed && GetWorld())
    {
        const float BufferDuration = FMath::Max(ComboInputBufferDuration, 0.01f);
        GetWorld()->GetTimerManager().ClearTimer(ComboBufferTimerHandle);
        GetWorld()->GetTimerManager().SetTimer(ComboBufferTimerHandle, this, &UAttackSystemComponent::ClearBufferedComboInput, BufferDuration, false);
    }
}

void UAttackSystemComponent::ContinueComboOrStop()
{
    if (!bSaveAttack)
    {
        StopCombo();
        return;
    }

    int32 NextStage = INDEX_NONE;
    if (AttackContinuationMode == ERPGAttackContinuationMode::ComboLinks)
    {
        NextStage = ResolveNextComboStage(AttackIndex, BufferedInputType);
    }
    else
    {
        NextStage = ResolveComboStartStage(BufferedInputType);
    }
    if (AttackStages.IsValidIndex(NextStage))
    {
        AttackIndex = NextStage;
        bSaveAttack = false;

        if (GetWorld())
        {
            GetWorld()->GetTimerManager().ClearTimer(ComboBufferTimerHandle);
        }

        bCanAttack = true;
        StartAttackStage(AttackIndex, BufferedInputType);
        return;
    }

    StopCombo();
}

void UAttackSystemComponent::StopCombo()
{
    const int32 PreviousAttackIndex = AttackIndex;

    bIsAttacking = false;
    bCanAttack = true;
    bSaveAttack = false;
    bComboWindowOpen = false;
    BufferedInputType = ERPGAttackInputType::Light;
    AttackIndex = 0;
    ActiveAttackInputType = ERPGAttackInputType::Light;
    ActiveAttackDamageMultiplier = 1.0f;
    ActiveAttackStaminaMultiplier = 1.0f;

    if (bStopAttackMontageOnComboEnd)
    {
    if (UAnimInstance* AnimInstance = GetOwnerAnimInstance())
        {
            if (AttackStages.IsValidIndex(PreviousAttackIndex))
            {
                if (UAnimMontage* PreviousStageMontage = AttackStages[PreviousAttackIndex].Montage)
                {
                    if (AnimInstance->Montage_IsPlaying(PreviousStageMontage))
                    {
                        AnimInstance->Montage_Stop(FMath::Max(0.0f, StopAttackMontageBlendOutTime), PreviousStageMontage);
                    }
                }
            }
        }
    }

    StopTrace();
    RestoreStageMovementPolicy();

    RestorePostAttackMovementLock();

    const bool bHasMovementIntent = HasMovementIntent();

    if (bClearGroundMomentumOnAttackEnd
        && CachedMoveComp.IsValid()
        && CachedMoveComp->IsMovingOnGround()
        && (!bPreserveMomentumWhenMovementInputHeld || !bHasMovementIntent))
    {
        const FVector CurrentVelocity = CachedMoveComp->Velocity;
        CachedMoveComp->Velocity = FVector(0.0f, 0.0f, CurrentVelocity.Z);
    }

    ApplyPostAttackMovementLock();

    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(ComboResetTimerHandle);
        GetWorld()->GetTimerManager().ClearTimer(ComboBufferTimerHandle);
    }

    if (CachedCombatState.IsValid()
        && !CachedCombatState->IsInState(ERPGCombatState::Dead)
        && !CachedCombatState->IsInState(ERPGCombatState::Hitstun))
    {
        CachedCombatState->RequestState(ERPGCombatState::Idle);
    }

    OnAttackEnded.Broadcast(PreviousAttackIndex);
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
    TickTrace();

    const float SafeInterval = FMath::Max(TraceInterval, 0.001f);
    GetWorld()->GetTimerManager().SetTimer(TraceTimerHandle, this, &UAttackSystemComponent::TickTrace, SafeInterval, true);
}

void UAttackSystemComponent::StartConfiguredTrace()
{
    ResolveDefaultTraceComponents();

    if (!ShouldUseFallbackTrace(TraceStartComponent, TraceEndComponent, GetOwner()))
    {
        StartTrace(TraceStartComponent.Get(), TraceEndComponent.Get());
        return;
    }

    BeginAutoManagedTrace();
}

void UAttackSystemComponent::StopTrace()
{
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(TraceTimerHandle);
    }

    bTraceActive = false;

    ClearPrimedTraceDamageSpec();

    if (bIsAttacking && CachedCombatState.IsValid() && !CachedCombatState->IsInState(ERPGCombatState::Dead))
    {
        CachedCombatState->RequestState(ERPGCombatState::AttackRecovery);
    }
}

void UAttackSystemComponent::ResetHitActors()
{
    HitActorsThisSwing.Reset();
}

void UAttackSystemComponent::PrimeTraceDamageSpec(const FRPGDamageSpec& DamageSpec, bool bResetHits)
{
    PrimedTraceDamageSpec = DamageSpec;
    bPrimedTraceDamageSpecActive = true;

    if (bResetHits)
    {
        ResetHitActors();
    }
}

void UAttackSystemComponent::ClearPrimedTraceDamageSpec()
{
    bPrimedTraceDamageSpecActive = false;
    PrimedTraceDamageSpec = FRPGDamageSpec();
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

bool UAttackSystemComponent::HasAttackStartForInputType(ERPGAttackInputType InputType) const
{
    return AttackStages.IsValidIndex(ResolveRandomizedStartStage(InputType))
        || (AttackStartStageByType.Contains(InputType)
            && AttackStages.IsValidIndex(AttackStartStageByType[InputType]));
}

ERPGAttackInputType UAttackSystemComponent::ResolvePrimaryAttackInputType() const
{
    TArray<ERPGAttackInputType> VariantCandidates;

    if (HasAttackStartForInputType(ERPGAttackInputType::LightSlash))
    {
        VariantCandidates.Add(ERPGAttackInputType::LightSlash);
    }

    if (HasAttackStartForInputType(ERPGAttackInputType::LightStab))
    {
        VariantCandidates.Add(ERPGAttackInputType::LightStab);
    }

    if (VariantCandidates.Num() > 0)
    {
        return VariantCandidates[FMath::RandRange(0, VariantCandidates.Num() - 1)];
    }

    return ERPGAttackInputType::Light;
}

int32 UAttackSystemComponent::ResolveRandomizedStartStage(ERPGAttackInputType InputType) const
{
    const FRPGAttackStartRandomPool* Pool = RandomizedStartStagesByType.Find(InputType);
    if (!Pool || Pool->StageIndices.Num() == 0)
    {
        return INDEX_NONE;
    }

    TArray<int32> ValidIndices;
    ValidIndices.Reserve(Pool->StageIndices.Num());
    for (int32 StageIndex : Pool->StageIndices)
    {
        if (AttackStages.IsValidIndex(StageIndex))
        {
            ValidIndices.AddUnique(StageIndex);
        }
    }

    if (ValidIndices.Num() == 0)
    {
        return INDEX_NONE;
    }

    int32 SelectedIndex = ValidIndices[FMath::RandRange(0, ValidIndices.Num() - 1)];

    if (Pool->bAvoidImmediateRepeat && ValidIndices.Num() > 1)
    {
        if (const int32* LastIndex = LastRandomStartStageByType.Find(InputType))
        {
            if (*LastIndex == SelectedIndex)
            {
                TArray<int32> Alternatives = ValidIndices;
                Alternatives.Remove(SelectedIndex);
                SelectedIndex = Alternatives[FMath::RandRange(0, Alternatives.Num() - 1)];
            }
        }
    }

    LastRandomStartStageByType.Add(InputType, SelectedIndex);
    return SelectedIndex;
}

int32 UAttackSystemComponent::ResolveComboStartStage(ERPGAttackInputType InputType) const
{
    const int32 RandomizedStage = ResolveRandomizedStartStage(InputType);
    if (AttackStages.IsValidIndex(RandomizedStage))
    {
        return RandomizedStage;
    }

    if (const int32* FoundStage = AttackStartStageByType.Find(InputType))
    {
        if (AttackStages.IsValidIndex(*FoundStage))
        {
            return *FoundStage;
        }
    }

    if (InputType == ERPGAttackInputType::Light)
    {
        const TArray<ERPGAttackInputType> CompatibleInputs = {
            ERPGAttackInputType::LightSlash,
            ERPGAttackInputType::LightStab
        };

        for (const ERPGAttackInputType CompatibleInput : CompatibleInputs)
        {
            const int32 CompatibleRandomizedStage = ResolveRandomizedStartStage(CompatibleInput);
            if (AttackStages.IsValidIndex(CompatibleRandomizedStage))
            {
                return CompatibleRandomizedStage;
            }

            if (const int32* CompatibleStage = AttackStartStageByType.Find(CompatibleInput))
            {
                if (AttackStages.IsValidIndex(*CompatibleStage))
                {
                    return *CompatibleStage;
                }
            }
        }
    }
    else if (InputType == ERPGAttackInputType::LightSlash || InputType == ERPGAttackInputType::LightStab)
    {
        const int32 GenericLightRandomizedStage = ResolveRandomizedStartStage(ERPGAttackInputType::Light);
        if (AttackStages.IsValidIndex(GenericLightRandomizedStage))
        {
            return GenericLightRandomizedStage;
        }

        if (const int32* GenericLightStage = AttackStartStageByType.Find(ERPGAttackInputType::Light))
        {
            if (AttackStages.IsValidIndex(*GenericLightStage))
            {
                return *GenericLightStage;
            }
        }
    }

    if (AttackStages.IsValidIndex(AttackIndex))
    {
        return AttackIndex;
    }

    return AttackStages.IsValidIndex(0) ? 0 : INDEX_NONE;
}

int32 UAttackSystemComponent::ResolveNextComboStage(int32 FromStageIndex, ERPGAttackInputType InputType) const
{
    if (!AttackStages.IsValidIndex(FromStageIndex))
    {
        return INDEX_NONE;
    }

    TArray<ERPGAttackInputType> CompatibleInputs;
    CompatibleInputs.Add(InputType);

    if (InputType == ERPGAttackInputType::Light)
    {
        CompatibleInputs.Add(ERPGAttackInputType::LightSlash);
        CompatibleInputs.Add(ERPGAttackInputType::LightStab);
    }
    else if (InputType == ERPGAttackInputType::LightSlash || InputType == ERPGAttackInputType::LightStab)
    {
        CompatibleInputs.Add(ERPGAttackInputType::Light);
    }

    const FRPGAttackStage& Stage = AttackStages[FromStageIndex];
    for (const ERPGAttackInputType CompatibleInput : CompatibleInputs)
    {
        for (const FRPGComboLink& Link : Stage.ComboLinks)
        {
            if (Link.InputType == CompatibleInput && AttackStages.IsValidIndex(Link.NextStageIndex))
            {
                return Link.NextStageIndex;
            }
        }
    }

    if (bAllowSequentialComboFallback && AttackStages.IsValidIndex(FromStageIndex + 1))
    {
        return FromStageIndex + 1;
    }

    return INDEX_NONE;
}


bool UAttackSystemComponent::ConsumeStaminaForCurrentStage() const
{
    if (!CachedStats.IsValid() || !AttackStages.IsValidIndex(AttackIndex))
    {
        return true;
    }

    const float Cost = AttackStages[AttackIndex].StaminaCost * WeaponStaminaCostMultiplier * ActiveAttackStaminaMultiplier;
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
    BufferedInputType = ERPGAttackInputType::Light;
}

void UAttackSystemComponent::StartAttackStage(int32 StageIndex, ERPGAttackInputType InputType, float InDamageMultiplier, float InStaminaMultiplier)
{
    if (!AttackStages.IsValidIndex(StageIndex))
    {
        return;
    }

    if (!CanStartAttack())
    {
        return;
    }

    if (CachedCombatState.IsValid() && !CachedCombatState->RequestState(ERPGCombatState::AttackStartup))
    {
        return;
    }

    AttackIndex = StageIndex;
    ActiveAttackInputType = InputType;
    ActiveAttackDamageMultiplier = FMath::Max(0.01f, InDamageMultiplier);
    ActiveAttackStaminaMultiplier = FMath::Max(0.01f, InStaminaMultiplier);

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

    RestorePostAttackMovementLock();
    bPostAttackSpeedRecoveryActive = false;
    StopTrace();
    RestoreStageMovementPolicy();
    ApplyStageMovementPolicy(AttackStages[AttackIndex]);
    ResetHitActors();
    ResolveDefaultTraceComponents();

    bool bUsingMontageEndComboFlow = false;
    float ForcedResetDelay = FMath::Max(AttackStages[AttackIndex].ComboResetDelay, 0.01f);
    bool bUseNotifyDrivenTraceWindow = false;
    if (UAnimInstance* AnimInstance = GetOwnerAnimInstance())
    {
        if (UAnimMontage* StageMontage = AttackStages[AttackIndex].Montage)
        {
            bUseNotifyDrivenTraceWindow = MontageUsesNotifyDrivenTraceWindow(StageMontage);
            if (bForceMontageRestartPerStage && AnimInstance->Montage_IsPlaying(StageMontage))
            {
                AnimInstance->Montage_Stop(0.02f, StageMontage);
            }

            const float PlayedMontageDuration = AnimInstance->Montage_Play(
                StageMontage,
                1.0f,
                EMontagePlayReturnType::MontageLength,
                0.0f,
                true);

            if (PlayedMontageDuration > 0.0f)
            {
                ForcedResetDelay = FMath::Max(ForcedResetDelay, PlayedMontageDuration + FMath::Max(StopAttackMontageBlendOutTime, 0.05f));
            }

            if (bUseMontageBlendOutForComboFlow)
            {
                FOnMontageBlendingOutStarted BlendOutDelegate;
                BlendOutDelegate.BindUObject(this, &UAttackSystemComponent::HandleStageMontageBlendingOut);
                AnimInstance->Montage_SetBlendingOutDelegate(BlendOutDelegate, StageMontage);
                bUsingMontageEndComboFlow = true;
            }
        }
    }

    if (bAutoManageWeaponTrace && !bUseNotifyDrivenTraceWindow)
    {
        BeginAutoManagedTrace();
    }

    OnAttackStarted.Broadcast(AttackIndex);

    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(ComboResetTimerHandle);

        if (!bUsingMontageEndComboFlow)
        {
            ForcedResetDelay = FMath::Max(AttackStages[AttackIndex].ComboResetDelay, 0.01f);
        }

        GetWorld()->GetTimerManager().SetTimer(ComboResetTimerHandle, this, &UAttackSystemComponent::ResetComboState, ForcedResetDelay, false);
    }
}

void UAttackSystemComponent::HandleStageMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted)
{
    if (!bUseMontageBlendOutForComboFlow || !bIsAttacking || !AttackStages.IsValidIndex(AttackIndex))
    {
        return;
    }

    if (AttackStages[AttackIndex].Montage != Montage)
    {
        return;
    }

    if (bInterrupted)
    {
        StopCombo();
        return;
    }

    ContinueComboOrStop();
}

void UAttackSystemComponent::ResetComboState()
{
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

void UAttackSystemComponent::BeginAutoManagedTrace()
{
    if (!GetWorld() || bTraceActive)
    {
        return;
    }

    if (!ShouldUseFallbackTrace(TraceStartComponent, TraceEndComponent, GetOwner()))
    {
        StartTrace(TraceStartComponent.Get(), TraceEndComponent.Get());
        return;
    }

    bTraceActive = true;

    if (CachedCombatState.IsValid())
    {
        CachedCombatState->RequestState(ERPGCombatState::AttackActive);
    }

    GetWorld()->GetTimerManager().ClearTimer(TraceTimerHandle);
    TickTrace();

    const float SafeInterval = FMath::Max(TraceInterval, 0.001f);
    GetWorld()->GetTimerManager().SetTimer(TraceTimerHandle, this, &UAttackSystemComponent::TickTrace, SafeInterval, true);
}
void UAttackSystemComponent::TickTrace()
{
    FVector Start = FVector::ZeroVector;
    FVector End = FVector::ZeroVector;

    if (!ShouldUseFallbackTrace(TraceStartComponent, TraceEndComponent, GetOwner()))
    {
        Start = TraceStartComponent->GetComponentLocation();
        End = TraceEndComponent->GetComponentLocation();
    }
    else if (GetOwner())
    {
        const FTransform OwnerTransform = GetOwner()->GetActorTransform();
        Start = OwnerTransform.TransformPosition(TraceFallbackLocalStartOffset);
        End = Start + (GetOwner()->GetActorForwardVector().GetSafeNormal2D() * TraceFallbackForwardDistance);
    }
    else
    {
        return;
    }

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

        UPlayerStatsComponent* TargetStats = HitActor->FindComponentByClass<UPlayerStatsComponent>();
        const bool bHasDamageableTag = DamageableTag.IsNone() || HitActor->ActorHasTag(DamageableTag);
        if (!TargetStats && !bHasDamageableTag)
        {
            continue;
        }

        HitActorsThisSwing.Add(HitActorPtr);

        const FRPGDamageSpec DamageSpec = BuildDamageSpec(Hit);
        UE_LOG(LogTemp, Log, TEXT("Attack hit: Owner=%s Target=%s Damage=%.2f"), *GetNameSafe(GetOwner()), *GetNameSafe(HitActor), DamageSpec.Damage);

        if (TargetStats)
        {
            TargetStats->ApplyIncomingHit(DamageSpec);
        }
        else
        {
            UGameplayStatics::ApplyDamage(
                HitActor,
                DamageSpec.Damage,
                DamageSpec.EventInstigator,
                DamageSpec.DamageCauser,
                nullptr);
        }

        OnAttackHit.Broadcast(HitActor);
    }
}

FRPGDamageSpec UAttackSystemComponent::BuildDamageSpec(const FHitResult& Hit) const
{
    FRPGDamageSpec DamageSpec = bPrimedTraceDamageSpecActive ? PrimedTraceDamageSpec : FRPGDamageSpec();
    if (!DamageSpec.DamageCauser)
    {
        DamageSpec.DamageCauser = GetOwner();
    }
    if (!DamageSpec.EventInstigator)
    {
        DamageSpec.EventInstigator = CachedCharacter.IsValid() ? CachedCharacter->GetController() : nullptr;
    }
    DamageSpec.HitLocation = Hit.ImpactPoint;
    DamageSpec.HitDirection = ResolveHitDirectionAgainstActor(Hit.GetActor());

    if (bPrimedTraceDamageSpecActive)
    {
        return DamageSpec;
    }

    if (!AttackStages.IsValidIndex(AttackIndex))
    {
        DamageSpec.Damage *= WeaponDamageMultiplier * ActiveAttackDamageMultiplier;
        return DamageSpec;
    }

    const FRPGAttackStage& Stage = AttackStages[AttackIndex];
    DamageSpec.Damage = Stage.Damage * WeaponDamageMultiplier * ActiveAttackDamageMultiplier;
    DamageSpec.HitstunDuration = Stage.HitstunDuration;
    DamageSpec.ReactionStrength = Stage.ReactionStrength;
    DamageSpec.StaggerDamage = Stage.StaggerDamage;
    DamageSpec.bCanBeBlocked = Stage.bCanBeBlocked;
    DamageSpec.bCanBeParried = Stage.bCanBeParried;
    return DamageSpec;
}

ERPGHitDirection UAttackSystemComponent::ResolveHitDirectionAgainstActor(const AActor* TargetActor) const
{
    if (!IsValid(TargetActor) || !GetOwner())
    {
        return ERPGHitDirection::Front;
    }

    FVector ToAttacker = GetOwner()->GetActorLocation() - TargetActor->GetActorLocation();
    ToAttacker.Z = 0.0f;
    ToAttacker = ToAttacker.GetSafeNormal();

    if (ToAttacker.IsNearlyZero())
    {
        return ERPGHitDirection::Front;
    }

    const FVector TargetForward = TargetActor->GetActorForwardVector().GetSafeNormal2D();
    const FVector TargetRight = TargetActor->GetActorRightVector().GetSafeNormal2D();
    const float ForwardDot = FVector::DotProduct(TargetForward, ToAttacker);
    const float RightDot = FVector::DotProduct(TargetRight, ToAttacker);

    if (FMath::Abs(ForwardDot) >= FMath::Abs(RightDot))
    {
        return ForwardDot >= 0.0f ? ERPGHitDirection::Front : ERPGHitDirection::Back;
    }

    return RightDot >= 0.0f ? ERPGHitDirection::Right : ERPGHitDirection::Left;
}
UAnimInstance* UAttackSystemComponent::GetOwnerAnimInstance() const
{
    if (!CachedCharacter.IsValid() || !CachedCharacter->GetMesh())
    {
        return nullptr;
    }

    return CachedCharacter->GetMesh()->GetAnimInstance();
}

void UAttackSystemComponent::ResolveDefaultTraceComponents()
{
    if (TraceStartComponent.IsValid() && TraceEndComponent.IsValid())
    {
        return;
    }

    if (!GetOwner())
    {
        return;
    }

    TArray<USceneComponent*> SceneComponents;
    GetOwner()->GetComponents<USceneComponent>(SceneComponents);

    for (USceneComponent* SceneComponent : SceneComponents)
    {
        if (!SceneComponent)
        {
            continue;
        }

        if (!TraceStartComponent.IsValid() && SceneComponent->GetFName() == DefaultTraceStartComponentName)
        {
            TraceStartComponent = SceneComponent;
        }
        else if (!TraceEndComponent.IsValid() && SceneComponent->GetFName() == DefaultTraceEndComponentName)
        {
            TraceEndComponent = SceneComponent;
        }
    }
}
bool UAttackSystemComponent::MontageUsesNotifyDrivenTraceWindow(const UAnimMontage* Montage) const
{
    if (!Montage)
    {
        return false;
    }

    for (const FAnimNotifyEvent& NotifyEvent : Montage->Notifies)
    {
        if (NotifyEvent.NotifyStateClass && NotifyEvent.NotifyStateClass->IsA<URPGAnimNotifyState_AttackTraceWindow>())
        {
            return true;
        }
    }

    return false;
}

void UAttackSystemComponent::ApplyStageMovementPolicy(const FRPGAttackStage& Stage)
{
    if (!CachedMoveComp.IsValid() && CachedCharacter.IsValid())
    {
        CachedMoveComp = CachedCharacter->GetCharacterMovement();
    }

    if (CachedMoveComp.IsValid())
    {
        if (!bStageWalkSpeedOverrideActive)
        {
            SavedWalkSpeed = CachedMoveComp->MaxWalkSpeed;
            bStageWalkSpeedOverrideActive = true;
        }

        float SpeedMultiplier = Stage.MaxWalkSpeedMultiplierDuringStage;
        if (bClampAttackWalkSpeedMultiplier)
        {
            SpeedMultiplier = FMath::Clamp(SpeedMultiplier, MinAttackWalkSpeedMultiplier, 1.0f);
        }

        CachedMoveComp->MaxWalkSpeed = SavedWalkSpeed * FMath::Max(0.0f, SpeedMultiplier);
    }

    if (Stage.bUseRootMotionForStage)
    {
        if (UAnimInstance* AnimInstance = GetOwnerAnimInstance())
        {
            if (!bStageRootMotionOverrideActive)
            {
                SavedRootMotionMode = static_cast<uint8>(AnimInstance->RootMotionMode);
                bStageRootMotionOverrideActive = true;
            }

            AnimInstance->SetRootMotionMode(ERootMotionMode::RootMotionFromMontagesOnly);
        }
    }
}

void UAttackSystemComponent::RestoreStageMovementPolicy()
{
    if (bStageWalkSpeedOverrideActive && CachedMoveComp.IsValid())
    {
        CachedMoveComp->MaxWalkSpeed = SavedWalkSpeed;
    }
    bStageWalkSpeedOverrideActive = false;

    if (bStageRootMotionOverrideActive)
    {
        if (UAnimInstance* AnimInstance = GetOwnerAnimInstance())
        {
            AnimInstance->SetRootMotionMode(static_cast<ERootMotionMode::Type>(SavedRootMotionMode));
        }
        bStageRootMotionOverrideActive = false;
    }
}

void UAttackSystemComponent::ApplyPostAttackMovementLock()
{
    if (!bApplyPostAttackMovementLock)
    {
        return;
    }

    if (bSkipPostAttackMovementLockWhenMovementInputHeld && HasMovementIntent())
    {
        return;
    }

    if (!CachedMoveComp.IsValid() && CachedCharacter.IsValid())
    {
        CachedMoveComp = CachedCharacter->GetCharacterMovement();
    }

    if (!CachedMoveComp.IsValid())
    {
        return;
    }

    SavedPostAttackWalkSpeed = CachedMoveComp->MaxWalkSpeed;
    CachedMoveComp->MaxWalkSpeed = 0.0f;
    bPostAttackMovementLockActive = true;

    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(PostAttackMovementLockTimerHandle);
        if (PostAttackMovementLockDuration > 0.0f)
        {
            GetWorld()->GetTimerManager().SetTimer(PostAttackMovementLockTimerHandle, this, &UAttackSystemComponent::RestorePostAttackMovementLock, PostAttackMovementLockDuration, false);
        }
    }
}

void UAttackSystemComponent::RestorePostAttackMovementLock()
{
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(PostAttackMovementLockTimerHandle);
    }

    if (!bPostAttackMovementLockActive)
    {
        return;
    }

    bPostAttackMovementLockActive = false;

    if (!CachedMoveComp.IsValid() && CachedCharacter.IsValid())
    {
        CachedMoveComp = CachedCharacter->GetCharacterMovement();
    }

    if (!CachedMoveComp.IsValid())
    {
        return;
    }

    if (bUsePostAttackSpeedRecoveryRamp && PostAttackSpeedRecoveryDuration > 0.0f)
    {
        bPostAttackSpeedRecoveryActive = true;
        PostAttackSpeedRecoveryElapsed = 0.0f;
        PostAttackSpeedRecoveryStartSpeed = CachedMoveComp->MaxWalkSpeed;
        PostAttackSpeedRecoveryTargetSpeed = SavedPostAttackWalkSpeed;
    }
    else
    {
        CachedMoveComp->MaxWalkSpeed = SavedPostAttackWalkSpeed;
        bPostAttackSpeedRecoveryActive = false;
    }
}

bool UAttackSystemComponent::HasMovementIntent() const
{
    if (!CachedCharacter.IsValid())
    {
        return false;
    }

    const FVector PendingInput = CachedCharacter->GetPendingMovementInputVector();
    if (!PendingInput.IsNearlyZero())
    {
        return true;
    }

    if (CachedMoveComp.IsValid())
    {
        return !CachedMoveComp->GetCurrentAcceleration().IsNearlyZero();
    }

    return false;
}






















void UAttackSystemComponent::HandleCombatStateChanged(ERPGCombatState OldState, ERPGCombatState NewState)
{
    if (!bIsAttacking)
    {
        return;
    }

    if (NewState == ERPGCombatState::Hitstun || NewState == ERPGCombatState::Dead)
    {
        StopCombo();
    }
}

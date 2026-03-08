#include "Components/AttackSystemComponent.h"

#include "Animation/AnimInstance.h"
#include "Components/CombatStateComponent.h"
#include "Components/PlayerStatsComponent.h"
#include "Components/SceneComponent.h"
#include "Components/TargetLockComponent.h"
#include "Data/RPGCombatMovesetDataAsset.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "GameFramework/CharacterMovementComponent.h"
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
    CachedMoveComp = CachedCharacter.IsValid() ? CachedCharacter->GetCharacterMovement() : nullptr;
    SavedWalkSpeed = CachedMoveComp.IsValid() ? CachedMoveComp->MaxWalkSpeed : 0.0f;

    if (bLoadMovesetOnBeginPlay && AttackMoveset)
    {
        ApplyAttackMovesetInternal(AttackMoveset);
    }

    if (AttackStages.Num() == 0)
    {
        AttackStages.Add(FRPGAttackStage());
    }
}

void UAttackSystemComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    EvaluateHoldHeavyInput();

    if (bIsChargingAttack && !bChargeFullySignaled && GetCurrentChargeTime() >= MaxChargeTime)
    {
        bChargeFullySignaled = true;
        OnChargeFullyCharged.Broadcast();
    }
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
    ComboInputBufferDuration = FMath::Max(InMoveset->ComboInputBufferDuration, 0.01f);
    bEnableHoldHeavyFromPrimaryInput = InMoveset->bEnableHoldHeavyFromPrimaryInput;
    HoldHeavyTriggerTime = FMath::Max(InMoveset->HoldHeavyTriggerTime, 0.01f);
    HoldHeavyInputType = InMoveset->HoldHeavyInputType;
    bEnableDistanceBasedLightVariants = InMoveset->bEnableDistanceBasedLightVariants;
    LightSlashInputType = InMoveset->LightSlashInputType;
    LightStabInputType = InMoveset->LightStabInputType;
    LightStabMinDistance = FMath::Max(0.0f, InMoveset->LightStabMinDistance);
    LightStabMaxDistance = FMath::Max(LightStabMinDistance, InMoveset->LightStabMaxDistance);
    bEnableChargedAttack = InMoveset->bEnableChargedAttack;
    MinChargeTime = FMath::Max(InMoveset->MinChargeTime, 0.01f);
    MaxChargeTime = FMath::Max(InMoveset->MaxChargeTime, 0.05f);
    bRequireFullChargeForChargedInput = InMoveset->bRequireFullChargeForChargedInput;
    PartialChargeInputType = InMoveset->PartialChargeInputType;
    FullChargeInputType = InMoveset->FullChargeInputType;
    bScaleChargedDamageByHoldTime = InMoveset->bScaleChargedDamageByHoldTime;
    MinChargedDamageMultiplier = FMath::Max(InMoveset->MinChargedDamageMultiplier, 0.01f);
    MaxChargedDamageMultiplier = FMath::Max(InMoveset->MaxChargedDamageMultiplier, 0.01f);
    MinChargedStaminaMultiplier = FMath::Max(InMoveset->MinChargedStaminaMultiplier, 0.01f);
    MaxChargedStaminaMultiplier = FMath::Max(InMoveset->MaxChargedStaminaMultiplier, 0.01f);
    bAutoPlayChargePresentation = InMoveset->bAutoPlayChargePresentation;
    ChargeStartMontage = InMoveset->ChargeStartMontage;
    ChargeLoopMontage = InMoveset->ChargeLoopMontage;
    ChargeStartMontagePlayRate = FMath::Max(InMoveset->ChargeStartMontagePlayRate, 0.01f);
    ChargeLoopMontagePlayRate = FMath::Max(InMoveset->ChargeLoopMontagePlayRate, 0.01f);
    ChargeReleaseBlendOutTime = FMath::Max(InMoveset->ChargeReleaseBlendOutTime, 0.0f);
    bStopChargeMontagesOnRelease = InMoveset->bStopChargeMontagesOnRelease;
    bEnableFinishers = InMoveset->bEnableFinishers;
    FinisherChanceOnLethalHit = FMath::Clamp(InMoveset->FinisherChanceOnLethalHit, 0.0f, 1.0f);
    FinisherMontages = InMoveset->FinisherMontages;
    FinisherMontagePlayRate = FMath::Max(InMoveset->FinisherMontagePlayRate, 0.01f);
    bStopCurrentMontageForFinisher = InMoveset->bStopCurrentMontageForFinisher;

    if (AttackStages.Num() == 0)
    {
        AttackStages.Add(FRPGAttackStage());
    }
}

void UAttackSystemComponent::HandleAttackInput()
{
    HandleAttackInputByType(ResolvePrimaryLightInputType());
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
    bPrimaryInputHeld = true;
    bHoldHeavyTriggeredThisPress = false;
    PrimaryInputPressStartTimeSeconds = GetWorld() ? static_cast<float>(GetWorld()->GetTimeSeconds()) : 0.0f;
    UE_LOG(LogTemp, Verbose, TEXT("PrimaryAttack: pressed"));
}

void UAttackSystemComponent::HandlePrimaryAttackReleased()
{
    const float ReleaseTimeSeconds = GetWorld() ? static_cast<float>(GetWorld()->GetTimeSeconds()) : 0.0f;
    const float HeldDuration = ReleaseTimeSeconds - PrimaryInputPressStartTimeSeconds;
    const bool bDidTriggerHeavy = bHoldHeavyTriggeredThisPress;
    const bool bExceededHoldThreshold = bEnableHoldHeavyFromPrimaryInput && (HeldDuration >= HoldHeavyTriggerTime);

    bPrimaryInputHeld = false;
    bHoldHeavyTriggeredThisPress = false;
    PrimaryInputPressStartTimeSeconds = 0.0f;

    if (bDidTriggerHeavy)
    {
        UE_LOG(LogTemp, Verbose, TEXT("PrimaryAttack: released after heavy trigger (held %.3fs)"), HeldDuration);
        return;
    }

    if (bExceededHoldThreshold)
    {
        UE_LOG(LogTemp, Warning, TEXT("PrimaryAttack: hold threshold reached (%.3fs) but heavy did not trigger. Skipping light fallback."), HeldDuration);
        return;
    }

    const ERPGAttackInputType ResolvedLightInput = ResolvePrimaryLightInputType();
    UE_LOG(LogTemp, Verbose, TEXT("PrimaryAttack: tap release (held %.3fs), triggering %s"), HeldDuration, *UEnum::GetValueAsString(ResolvedLightInput));
    HandleAttackInputByType(ResolvedLightInput);
}
bool UAttackSystemComponent::BeginChargeAttack()
{
    if (!bEnableChargedAttack || bIsChargingAttack || bIsAttacking)
    {
        return false;
    }

    if (!CanStartAttack())
    {
        return false;
    }

    bIsChargingAttack = true;
    bChargeFullySignaled = false;
    ChargeStartTimeSeconds = GetWorld() ? static_cast<float>(GetWorld()->GetTimeSeconds()) : 0.0f;

    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(ChargeLoopStartTimerHandle);
    }

    PlayChargePresentationStart();
    ApplyChargeMovementPolicy();
    OnChargeStateChanged.Broadcast(true);
    return true;
}

bool UAttackSystemComponent::ReleaseChargeAttack()
{
    if (!bIsChargingAttack)
    {
        return false;
    }

    const float HeldTime = GetCurrentChargeTime();
    const ERPGAttackInputType ChargedInputType = ResolveChargedInputType(HeldTime);

    float ChargedDamageMultiplier = 1.0f;
    float ChargedStaminaMultiplier = 1.0f;
    ResolveChargedMultipliers(HeldTime, ChargedDamageMultiplier, ChargedStaminaMultiplier);

    bIsChargingAttack = false;
    ChargeStartTimeSeconds = 0.0f;
    bChargeFullySignaled = false;

    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(ChargeLoopStartTimerHandle);
    }

    if (bStopChargeMontagesOnRelease)
    {
        StopChargePresentation(ChargeReleaseBlendOutTime);
    }

    RestoreChargeMovementPolicy();
    OnChargeStateChanged.Broadcast(false);

    if (bIsAttacking)
    {
        BufferedInputType = ChargedInputType;
        bSaveAttack = true;

        if (!bKeepBufferedInputUntilConsumed && GetWorld())
        {
            const float BufferDuration = FMath::Max(ComboInputBufferDuration, 0.01f);
            GetWorld()->GetTimerManager().ClearTimer(ComboBufferTimerHandle);
            GetWorld()->GetTimerManager().SetTimer(ComboBufferTimerHandle, this, &UAttackSystemComponent::ClearBufferedComboInput, BufferDuration, false);
        }

        return true;
    }

    if (!CanStartAttack())
    {
        return false;
    }

    const int32 StartStage = ResolveComboStartStage(ChargedInputType);
    if (!AttackStages.IsValidIndex(StartStage))
    {
        return false;
    }

    StartAttackStage(StartStage, ChargedInputType, ChargedDamageMultiplier, ChargedStaminaMultiplier);
    return true;
}

void UAttackSystemComponent::CancelChargeAttack()
{
    if (!bIsChargingAttack)
    {
        return;
    }

    bIsChargingAttack = false;
    ChargeStartTimeSeconds = 0.0f;
    bChargeFullySignaled = false;

    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(ChargeLoopStartTimerHandle);
    }

    StopChargePresentation(ChargeReleaseBlendOutTime);
    RestoreChargeMovementPolicy();
    OnChargeStateChanged.Broadcast(false);
}

float UAttackSystemComponent::GetCurrentChargeTime() const
{
    if (!bIsChargingAttack || !GetWorld())
    {
        return 0.0f;
    }

    return FMath::Max(0.0f, static_cast<float>(GetWorld()->GetTimeSeconds()) - ChargeStartTimeSeconds);
}

float UAttackSystemComponent::GetCurrentChargeRatio() const
{
    if (!bIsChargingAttack)
    {
        return 0.0f;
    }

    const float SafeMaxCharge = FMath::Max(MaxChargeTime, 0.05f);
    return FMath::Clamp(GetCurrentChargeTime() / SafeMaxCharge, 0.0f, 1.0f);
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

    const int32 NextStage = ResolveNextComboStage(AttackIndex, BufferedInputType);
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
    bIsAttacking = false;
    bCanAttack = true;
    bSaveAttack = false;
    bComboWindowOpen = false;
    BufferedInputType = ERPGAttackInputType::Light;
    AttackIndex = 0;
    ActiveAttackInputType = ERPGAttackInputType::Light;
    ActiveAttackDamageMultiplier = 1.0f;
    ActiveAttackStaminaMultiplier = 1.0f;
    RestoreStageMovementPolicy();

    RestorePostAttackMovementLock();

    if (bClearGroundMomentumOnAttackEnd && CachedMoveComp.IsValid() && CachedMoveComp->IsMovingOnGround())
    {
        const FVector CurrentVelocity = CachedMoveComp->Velocity;
        CachedMoveComp->Velocity = FVector(0.0f, 0.0f, CurrentVelocity.Z);
    }

    CancelChargeAttack();
    ApplyPostAttackMovementLock();

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

    // Backward compatibility: if slash/stab is not explicitly mapped, fall back to generic Light.
    if (InputType == ERPGAttackInputType::LightSlash || InputType == ERPGAttackInputType::LightStab)
    {
        const int32 LightRandomizedStage = ResolveRandomizedStartStage(ERPGAttackInputType::Light);
        if (AttackStages.IsValidIndex(LightRandomizedStage))
        {
            return LightRandomizedStage;
        }

        if (const int32* LightStage = AttackStartStageByType.Find(ERPGAttackInputType::Light))
        {
            if (AttackStages.IsValidIndex(*LightStage))
            {
                return *LightStage;
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

    const FRPGAttackStage& Stage = AttackStages[FromStageIndex];
    for (const FRPGComboLink& Link : Stage.ComboLinks)
    {
        if (Link.InputType == InputType && AttackStages.IsValidIndex(Link.NextStageIndex))
        {
            return Link.NextStageIndex;
        }
    }

    // Backward compatibility: allow light slash/stab inputs to consume generic Light combo links.
    if (InputType == ERPGAttackInputType::LightSlash || InputType == ERPGAttackInputType::LightStab)
    {
        for (const FRPGComboLink& Link : Stage.ComboLinks)
        {
            if (Link.InputType == ERPGAttackInputType::Light && AttackStages.IsValidIndex(Link.NextStageIndex))
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
ERPGAttackInputType UAttackSystemComponent::ResolvePrimaryLightInputType() const
{
    if (!bEnableDistanceBasedLightVariants)
    {
        return ERPGAttackInputType::Light;
    }

    if (!GetOwner() || !CachedTargetLock.IsValid() || !CachedTargetLock->IsLockedOn())
    {
        return LightSlashInputType;
    }

    const FVector OwnerLocation = GetOwner()->GetActorLocation();
    const FVector TargetLocation = CachedTargetLock->GetLockTargetLocation();
    const float DistanceToTarget = FVector::Dist2D(OwnerLocation, TargetLocation);

    const float MinDistance = FMath::Max(0.0f, LightStabMinDistance);
    const float MaxDistance = FMath::Max(MinDistance, LightStabMaxDistance);
    return (DistanceToTarget >= MinDistance && DistanceToTarget <= MaxDistance) ? LightStabInputType : LightSlashInputType;
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

    StopChargePresentation(ChargeReleaseBlendOutTime);
    RestorePostAttackMovementLock();
    bPostAttackSpeedRecoveryActive = false;
    RestoreStageMovementPolicy();
    ApplyStageMovementPolicy(AttackStages[AttackIndex]);
    ResetHitActors();
    OnAttackStarted.Broadcast(AttackIndex);

    if (UAnimInstance* AnimInstance = GetOwnerAnimInstance())
    {
        if (UAnimMontage* StageMontage = AttackStages[AttackIndex].Montage)
        {
            if (bForceMontageRestartPerStage && AnimInstance->Montage_IsPlaying(StageMontage))
            {
                AnimInstance->Montage_Stop(0.02f, StageMontage);
            }

            AnimInstance->Montage_Play(
                StageMontage,
                1.0f,
                EMontagePlayReturnType::MontageLength,
                0.0f,
                true);
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

        const float BaseDamage = AttackStages.IsValidIndex(AttackIndex) ? AttackStages[AttackIndex].Damage : 10.0f;
        const float Damage = BaseDamage * WeaponDamageMultiplier * ActiveAttackDamageMultiplier;

        UPlayerStatsComponent* TargetStats = HitActor->FindComponentByClass<UPlayerStatsComponent>();
        const bool bPredictedLethal = TargetStats && !TargetStats->IsDead() && (Damage >= TargetStats->CurrentHealth);
        const bool bRollFinisher = bEnableFinishers
            && bPredictedLethal
            && FinisherChanceOnLethalHit > 0.0f
            && FinisherMontages.Num() > 0
            && (FMath::FRand() <= FinisherChanceOnLethalHit);

        UGameplayStatics::ApplyDamage(
            HitActor,
            Damage,
            CachedCharacter.IsValid() ? CachedCharacter->GetController() : nullptr,
            GetOwner(),
            nullptr);

        if (bRollFinisher && TargetStats && TargetStats->IsDead())
        {
            TryPlayFinisherMontage(HitActor);
        }

        OnAttackHit.Broadcast(HitActor);
    }
}

void UAttackSystemComponent::EvaluateHoldHeavyInput()
{
    if (!bEnableHoldHeavyFromPrimaryInput || !bPrimaryInputHeld || bHoldHeavyTriggeredThisPress || !GetWorld())
    {
        return;
    }

    const float HeldTime = static_cast<float>(GetWorld()->GetTimeSeconds()) - PrimaryInputPressStartTimeSeconds;
    if (HeldTime < HoldHeavyTriggerTime)
    {
        return;
    }

    if (TryTriggerHoldHeavyAttack())
    {
        bHoldHeavyTriggeredThisPress = true;
    }
}

bool UAttackSystemComponent::TryTriggerHoldHeavyAttack()
{
    if (bIsAttacking)
    {
        BufferComboInputByType(HoldHeavyInputType);
        return true;
    }

    if (!CanStartAttack())
    {
        return false;
    }

    const int32 StartStage = ResolveComboStartStage(HoldHeavyInputType);
    if (!AttackStages.IsValidIndex(StartStage))
    {
        return false;
    }

    UE_LOG(LogTemp, Log, TEXT("HoldHeavy: triggering heavy attack at stage index %d"), StartStage);
    StartAttackStage(StartStage, HoldHeavyInputType);
    return true;
}

ERPGAttackInputType UAttackSystemComponent::ResolveChargedInputType(float HeldTime) const
{
    if (HeldTime < MinChargeTime)
    {
        return PartialChargeInputType;
    }

    if (!bRequireFullChargeForChargedInput)
    {
        return FullChargeInputType;
    }

    const float SafeMaxCharge = FMath::Max(MaxChargeTime, 0.05f);
    return HeldTime >= SafeMaxCharge ? FullChargeInputType : PartialChargeInputType;
}

void UAttackSystemComponent::ResolveChargedMultipliers(float HeldTime, float& OutDamageMultiplier, float& OutStaminaMultiplier) const
{
    OutDamageMultiplier = 1.0f;
    OutStaminaMultiplier = 1.0f;

    if (!bScaleChargedDamageByHoldTime)
    {
        return;
    }

    const float SafeMaxCharge = FMath::Max(MaxChargeTime, 0.05f);
    const float ChargeAlpha = FMath::Clamp(HeldTime / SafeMaxCharge, 0.0f, 1.0f);

    OutDamageMultiplier = FMath::Lerp(MinChargedDamageMultiplier, MaxChargedDamageMultiplier, ChargeAlpha);
    OutStaminaMultiplier = FMath::Lerp(MinChargedStaminaMultiplier, MaxChargedStaminaMultiplier, ChargeAlpha);
}

void UAttackSystemComponent::TryPlayFinisherMontage(AActor* TargetActor)
{
    UAnimInstance* AnimInstance = GetOwnerAnimInstance();
    if (!AnimInstance)
    {
        return;
    }

    TArray<UAnimMontage*> ValidMontages;
    for (UAnimMontage* Montage : FinisherMontages)
    {
        if (Montage)
        {
            ValidMontages.Add(Montage);
        }
    }

    if (ValidMontages.Num() == 0)
    {
        return;
    }

    UAnimMontage* SelectedMontage = ValidMontages[FMath::RandRange(0, ValidMontages.Num() - 1)];
    if (!SelectedMontage)
    {
        return;
    }

    if (bStopCurrentMontageForFinisher)
    {
        AnimInstance->Montage_Stop(0.08f);
    }

    AnimInstance->Montage_Play(SelectedMontage, FMath::Max(0.01f, FinisherMontagePlayRate));
    OnFinisherTriggered.Broadcast(TargetActor);
}

UAnimInstance* UAttackSystemComponent::GetOwnerAnimInstance() const
{
    if (!CachedCharacter.IsValid() || !CachedCharacter->GetMesh())
    {
        return nullptr;
    }

    return CachedCharacter->GetMesh()->GetAnimInstance();
}

void UAttackSystemComponent::PlayChargePresentationStart()
{
    if (!bAutoPlayChargePresentation)
    {
        return;
    }

    UAnimInstance* AnimInstance = GetOwnerAnimInstance();
    if (!AnimInstance)
    {
        return;
    }

    if (ChargeStartMontage)
    {
        AnimInstance->Montage_Play(ChargeStartMontage, FMath::Max(0.01f, ChargeStartMontagePlayRate));

        if (ChargeLoopMontage && GetWorld())
        {
            const float Duration = FMath::Max(0.01f, ChargeStartMontage->GetPlayLength() / FMath::Max(0.01f, ChargeStartMontagePlayRate));
            GetWorld()->GetTimerManager().ClearTimer(ChargeLoopStartTimerHandle);
            GetWorld()->GetTimerManager().SetTimer(ChargeLoopStartTimerHandle, this, &UAttackSystemComponent::PlayChargePresentationLoop, Duration, false);
        }

        return;
    }

    PlayChargePresentationLoop();
}

void UAttackSystemComponent::PlayChargePresentationLoop()
{
    if (!bIsChargingAttack || !bAutoPlayChargePresentation || !ChargeLoopMontage)
    {
        return;
    }

    UAnimInstance* AnimInstance = GetOwnerAnimInstance();
    if (!AnimInstance)
    {
        return;
    }

    AnimInstance->Montage_Play(ChargeLoopMontage, FMath::Max(0.01f, ChargeLoopMontagePlayRate));
}

void UAttackSystemComponent::StopChargePresentation(float BlendOutTime)
{
    UAnimInstance* AnimInstance = GetOwnerAnimInstance();
    if (!AnimInstance)
    {
        return;
    }

    const float SafeBlend = FMath::Max(0.0f, BlendOutTime);

    if (ChargeStartMontage && AnimInstance->Montage_IsPlaying(ChargeStartMontage))
    {
        AnimInstance->Montage_Stop(SafeBlend, ChargeStartMontage);
    }

    if (ChargeLoopMontage && AnimInstance->Montage_IsPlaying(ChargeLoopMontage))
    {
        AnimInstance->Montage_Stop(SafeBlend, ChargeLoopMontage);
    }
}









void UAttackSystemComponent::ApplyChargeMovementPolicy()
{
    if (!bLimitMovementWhileCharging)
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

    if (!bChargeWalkSpeedOverrideActive)
    {
        SavedChargeWalkSpeed = CachedMoveComp->MaxWalkSpeed;
        bChargeWalkSpeedOverrideActive = true;
    }

    const float SpeedMultiplier = FMath::Clamp(ChargeWalkSpeedMultiplier, 0.0f, 1.0f);
    CachedMoveComp->MaxWalkSpeed = SavedChargeWalkSpeed * SpeedMultiplier;
}

void UAttackSystemComponent::RestoreChargeMovementPolicy()
{
    if (!bChargeWalkSpeedOverrideActive)
    {
        return;
    }

    if (CachedMoveComp.IsValid())
    {
        CachedMoveComp->MaxWalkSpeed = SavedChargeWalkSpeed;
    }

    bChargeWalkSpeedOverrideActive = false;
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

        float SpeedMultiplier = FMath::Clamp(Stage.MaxWalkSpeedMultiplierDuringStage, 0.0f, 1.0f);
        if (bClampAttackWalkSpeedMultiplier)
        {
            SpeedMultiplier = FMath::Max(SpeedMultiplier, FMath::Clamp(MinAttackWalkSpeedMultiplier, 0.0f, 1.0f));
        }
        CachedMoveComp->MaxWalkSpeed = SavedWalkSpeed * SpeedMultiplier;
    }

    if (UAnimInstance* AnimInstance = GetOwnerAnimInstance())
    {
        if (!bStageRootMotionOverrideActive)
        {
            SavedRootMotionMode = static_cast<uint8>(AnimInstance->RootMotionMode);
            bStageRootMotionOverrideActive = true;
        }

        if (Stage.bUseRootMotionForStage)
        {
            AnimInstance->SetRootMotionMode(ERootMotionMode::RootMotionFromMontagesOnly);
        }
        else
        {
            AnimInstance->SetRootMotionMode(ERootMotionMode::NoRootMotionExtraction);
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
    }
    bStageRootMotionOverrideActive = false;
}





































void UAttackSystemComponent::ApplyPostAttackMovementLock()
{
    if (!bApplyPostAttackMovementLock || PostAttackMovementLockDuration <= 0.0f)
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

    if (!bPostAttackMovementLockActive)
    {
        SavedPostAttackWalkSpeed = CachedMoveComp->MaxWalkSpeed;
        bPostAttackMovementLockActive = true;
    }

    bPostAttackSpeedRecoveryActive = false;
    PostAttackSpeedRecoveryElapsed = 0.0f;
    PostAttackSpeedRecoveryStartSpeed = 0.0f;
    PostAttackSpeedRecoveryTargetSpeed = 0.0f;
    CachedMoveComp->MaxWalkSpeed = 0.0f;

    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(PostAttackMovementLockTimerHandle);
        GetWorld()->GetTimerManager().SetTimer(
            PostAttackMovementLockTimerHandle,
            this,
            &UAttackSystemComponent::RestorePostAttackMovementLock,
            PostAttackMovementLockDuration,
            false);
    }
}

void UAttackSystemComponent::RestorePostAttackMovementLock()
{
    if (!bPostAttackMovementLockActive)
    {
        return;
    }

    if (CachedMoveComp.IsValid())
    {
        const float TargetWalkSpeed = SavedPostAttackWalkSpeed;

        if (bUsePostAttackSpeedRecoveryRamp && PostAttackSpeedRecoveryDuration > 0.0f)
        {
            PostAttackSpeedRecoveryStartSpeed = CachedMoveComp->MaxWalkSpeed;
            PostAttackSpeedRecoveryTargetSpeed = TargetWalkSpeed;
            PostAttackSpeedRecoveryElapsed = 0.0f;
            bPostAttackSpeedRecoveryActive = !FMath::IsNearlyEqual(PostAttackSpeedRecoveryStartSpeed, PostAttackSpeedRecoveryTargetSpeed);

            if (!bPostAttackSpeedRecoveryActive)
            {
                CachedMoveComp->MaxWalkSpeed = TargetWalkSpeed;
            }
        }
        else
        {
            bPostAttackSpeedRecoveryActive = false;
            CachedMoveComp->MaxWalkSpeed = TargetWalkSpeed;
        }
    }

    bPostAttackMovementLockActive = false;

    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(PostAttackMovementLockTimerHandle);
    }
}



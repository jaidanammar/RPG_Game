#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AttackSystemComponent.generated.h"

class AActor;
class ACharacter;
class UAnimInstance;
class UAnimMontage;
class URPGCombatMovesetDataAsset;
class USceneComponent;
class UPlayerStatsComponent;
class UTargetLockComponent;
class UCombatStateComponent;
class UCharacterMovementComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAttackStateChanged, int32, AttackIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAttackHit, AActor*, HitActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnChargeStateChanged, bool, bIsCharging);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnChargeFullyCharged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFinisherTriggered, AActor*, TargetActor);

UENUM(BlueprintType)
enum class ERPGAttackInputType : uint8
{
    Light UMETA(DisplayName = "Light"),
    Heavy UMETA(DisplayName = "Heavy"),
    Special UMETA(DisplayName = "Special"),
    Charged UMETA(DisplayName = "Charged"),
    Aerial UMETA(DisplayName = "Aerial"),
    LightSlash UMETA(DisplayName = "Light Slash"),
    LightStab UMETA(DisplayName = "Light Stab")
};

USTRUCT(BlueprintType)
struct FRPGComboLink
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo")
    ERPGAttackInputType InputType = ERPGAttackInputType::Light;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo", meta = (ClampMin = "0"))
    int32 NextStageIndex = INDEX_NONE;
};

USTRUCT(BlueprintType)
struct FRPGAttackStage
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
    UAnimMontage* Montage = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack", meta = (ClampMin = "0.0"))
    float StaminaCost = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack", meta = (ClampMin = "0.0"))
    float Damage = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack", meta = (ClampMin = "0.01"))
    float ComboResetDelay = 0.6f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Combo")
    TArray<FRPGComboLink> ComboLinks;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Movement")
    bool bUseRootMotionForStage = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Movement", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float MaxWalkSpeedMultiplierDuringStage = 1.0f;
};

USTRUCT(BlueprintType)
struct FRPGAttackStartRandomPool
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Combo")
    TArray<int32> StageIndices;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Combo")
    bool bAvoidImmediateRepeat = true;
};

UCLASS(ClassGroup=(RPG), BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class RPG_GAME_API UAttackSystemComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UAttackSystemComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Setup")
    TObjectPtr<URPGCombatMovesetDataAsset> AttackMoveset = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Setup")
    bool bLoadMovesetOnBeginPlay = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|State")
    bool bCanAttack = true;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attack|State")
    bool bIsAttacking = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attack|State")
    bool bSaveAttack = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attack|State")
    int32 AttackIndex = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attack|State")
    bool bComboWindowOpen = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Setup")
    TArray<FRPGAttackStage> AttackStages;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Combo")
    bool bUseComboWindowLock = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Combo")
    bool bAllowSequentialComboFallback = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Combo", meta = (ClampMin = "0.01"))
    float ComboInputBufferDuration = 0.25f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Combo")
    bool bKeepBufferedInputUntilConsumed = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Combo")
    TMap<ERPGAttackInputType, int32> AttackStartStageByType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Combo")
    TMap<ERPGAttackInputType, FRPGAttackStartRandomPool> RandomizedStartStagesByType;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attack|State")
    ERPGAttackInputType BufferedInputType = ERPGAttackInputType::Light;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Input")
    bool bEnableHoldHeavyFromPrimaryInput = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Input", meta=(ClampMin="0.01"))
    float HoldHeavyTriggerTime = 0.2f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Input")
    ERPGAttackInputType HoldHeavyInputType = ERPGAttackInputType::Heavy;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Input|Light Variants")
    bool bEnableDistanceBasedLightVariants = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Input|Light Variants")
    ERPGAttackInputType LightSlashInputType = ERPGAttackInputType::LightSlash;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Input|Light Variants")
    ERPGAttackInputType LightStabInputType = ERPGAttackInputType::LightStab;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Input|Light Variants", meta=(ClampMin="0.0"))
    float LightStabMinDistance = 220.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Input|Light Variants", meta=(ClampMin="0.0"))
    float LightStabMaxDistance = 500.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Charged")
    bool bEnableChargedAttack = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Charged", meta=(ClampMin="0.01"))
    float MinChargeTime = 0.35f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Charged", meta=(ClampMin="0.05"))
    float MaxChargeTime = 1.1f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Charged")
    bool bRequireFullChargeForChargedInput = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Charged")
    ERPGAttackInputType PartialChargeInputType = ERPGAttackInputType::Heavy;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Charged")
    ERPGAttackInputType FullChargeInputType = ERPGAttackInputType::Charged;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Charged")
    bool bScaleChargedDamageByHoldTime = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Charged", meta=(ClampMin="0.01"))
    float MinChargedDamageMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Charged", meta=(ClampMin="0.01"))
    float MaxChargedDamageMultiplier = 1.8f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Charged", meta=(ClampMin="0.01"))
    float MinChargedStaminaMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Charged", meta=(ClampMin="0.01"))
    float MaxChargedStaminaMultiplier = 1.4f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attack|Charged")
    bool bIsChargingAttack = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Charged|Presentation")
    bool bAutoPlayChargePresentation = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Charged|Presentation")
    TObjectPtr<UAnimMontage> ChargeStartMontage = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Charged|Presentation")
    TObjectPtr<UAnimMontage> ChargeLoopMontage = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Charged|Presentation", meta=(ClampMin="0.01"))
    float ChargeStartMontagePlayRate = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Charged|Presentation", meta=(ClampMin="0.01"))
    float ChargeLoopMontagePlayRate = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Charged|Presentation", meta=(ClampMin="0.0"))
    float ChargeReleaseBlendOutTime = 0.08f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Charged|Presentation")
    bool bStopChargeMontagesOnRelease = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Charged|Movement")
    bool bLimitMovementWhileCharging = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Charged|Movement", meta=(ClampMin="0.0", ClampMax="1.0"))
    float ChargeWalkSpeedMultiplier = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Movement")
    bool bClampAttackWalkSpeedMultiplier = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Movement", meta=(ClampMin="0.0", ClampMax="1.0"))
    float MinAttackWalkSpeedMultiplier = 0.2f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Movement")
    bool bClearGroundMomentumOnAttackEnd = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Movement")
    bool bApplyPostAttackMovementLock = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Movement", meta=(ClampMin="0.0", ClampMax="0.35"))
    float PostAttackMovementLockDuration = 0.08f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Movement")
    bool bUsePostAttackSpeedRecoveryRamp = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Movement", meta=(ClampMin="0.0", ClampMax="0.6"))
    float PostAttackSpeedRecoveryDuration = 0.16f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Finisher")
    bool bEnableFinishers = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Finisher", meta=(ClampMin="0.0", ClampMax="1.0"))
    float FinisherChanceOnLethalHit = 0.35f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Finisher")
    TArray<TObjectPtr<UAnimMontage>> FinisherMontages;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Finisher", meta=(ClampMin="0.01"))
    float FinisherMontagePlayRate = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Finisher")
    bool bStopCurrentMontageForFinisher = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Animation")
    bool bForceMontageRestartPerStage = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Tuning", meta=(ClampMin="0.01"))
    float WeaponDamageMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Tuning", meta=(ClampMin="0.01"))
    float WeaponStaminaCostMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Aim")
    bool bEnableAttackFacingAssist = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Aim")
    bool bFaceLockTargetDuringAttack = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Aim")
    bool bFaceControllerYawWhenNoLockTarget = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Aim", meta = (ClampMin = "0.1"))
    float AttackFacingInterpSpeed = 12.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Trace", meta=(ClampMin = "0.0"))
    float TraceRadius = 15.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Trace", meta=(ClampMin = "0.001"))
    float TraceInterval = 0.01f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Trace")
    TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Pawn;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Trace")
    FName DamageableTag = TEXT("Damageable");

    UPROPERTY(BlueprintAssignable, Category = "Attack|Events")
    FOnAttackStateChanged OnAttackStarted;

    UPROPERTY(BlueprintAssignable, Category = "Attack|Events")
    FOnAttackStateChanged OnAttackEnded;

    UPROPERTY(BlueprintAssignable, Category = "Attack|Events")
    FOnAttackHit OnAttackHit;

    UPROPERTY(BlueprintAssignable, Category = "Attack|Events")
    FOnChargeStateChanged OnChargeStateChanged;

    UPROPERTY(BlueprintAssignable, Category = "Attack|Events")
    FOnChargeFullyCharged OnChargeFullyCharged;

    UPROPERTY(BlueprintAssignable, Category = "Attack|Events")
    FOnFinisherTriggered OnFinisherTriggered;

    UFUNCTION(BlueprintCallable, Category = "Attack|Setup")
    void ApplyAttackMoveset(URPGCombatMovesetDataAsset* InMoveset, bool bResetComboState = true);

    UFUNCTION(BlueprintCallable, Category = "Attack|Tuning")
    void SetWeaponAttackTuning(float InDamageMultiplier = 1.0f, float InStaminaMultiplier = 1.0f);

    UFUNCTION(BlueprintCallable, Category = "Attack")
    void HandleAttackInput();

    UFUNCTION(BlueprintCallable, Category = "Attack")
    void HandleAttackInputByType(ERPGAttackInputType InputType);

    UFUNCTION(BlueprintCallable, Category = "Attack|Input")
    void HandlePrimaryAttackPressed();

    UFUNCTION(BlueprintCallable, Category = "Attack|Input")
    void HandlePrimaryAttackReleased();

    UFUNCTION(BlueprintCallable, Category = "Attack|Charged")
    bool BeginChargeAttack();

    UFUNCTION(BlueprintCallable, Category = "Attack|Charged")
    bool ReleaseChargeAttack();

    UFUNCTION(BlueprintCallable, Category = "Attack|Charged")
    void CancelChargeAttack();

    UFUNCTION(BlueprintPure, Category = "Attack|Charged")
    float GetCurrentChargeTime() const;

    UFUNCTION(BlueprintPure, Category = "Attack|Charged")
    float GetCurrentChargeRatio() const;

    UFUNCTION(BlueprintCallable, Category = "Attack")
    void BufferComboInput();

    UFUNCTION(BlueprintCallable, Category = "Attack")
    void BufferComboInputByType(ERPGAttackInputType InputType);

    UFUNCTION(BlueprintCallable, Category = "Attack")
    void ContinueComboOrStop();

    UFUNCTION(BlueprintCallable, Category = "Attack")
    void StopCombo();

    UFUNCTION(BlueprintCallable, Category = "Attack|Combo")
    void OpenComboWindow();

    UFUNCTION(BlueprintCallable, Category = "Attack|Combo")
    void CloseComboWindow();

    UFUNCTION(BlueprintCallable, Category = "Attack|Trace")
    void StartTrace(USceneComponent* InTraceStart, USceneComponent* InTraceEnd);

    UFUNCTION(BlueprintCallable, Category = "Attack|Trace")
    void StopTrace();

    UFUNCTION(BlueprintCallable, Category = "Attack|Trace")
    void ResetHitActors();

    UFUNCTION(BlueprintPure, Category = "Attack")
    bool IsAttacking() const { return bIsAttacking; }

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    TWeakObjectPtr<ACharacter> CachedCharacter;
    TWeakObjectPtr<UPlayerStatsComponent> CachedStats;
    TWeakObjectPtr<UTargetLockComponent> CachedTargetLock;
    TWeakObjectPtr<UCombatStateComponent> CachedCombatState;
    TWeakObjectPtr<UCharacterMovementComponent> CachedMoveComp;
    TWeakObjectPtr<USceneComponent> TraceStartComponent;
    TWeakObjectPtr<USceneComponent> TraceEndComponent;
    TSet<TWeakObjectPtr<AActor>> HitActorsThisSwing;
    FTimerHandle ComboResetTimerHandle;
    FTimerHandle ComboBufferTimerHandle;
    FTimerHandle TraceTimerHandle;
    FTimerHandle ChargeLoopStartTimerHandle;
    FTimerHandle PostAttackMovementLockTimerHandle;
    bool bTraceActive = false;

    float ChargeStartTimeSeconds = 0.0f;
    bool bChargeFullySignaled = false;
    bool bPrimaryInputHeld = false;
    bool bHoldHeavyTriggeredThisPress = false;
    float PrimaryInputPressStartTimeSeconds = 0.0f;
    ERPGAttackInputType ActiveAttackInputType = ERPGAttackInputType::Light;
    float ActiveAttackDamageMultiplier = 1.0f;
    float ActiveAttackStaminaMultiplier = 1.0f;
    float SavedWalkSpeed = 0.0f;
    bool bStageWalkSpeedOverrideActive = false;
    uint8 SavedRootMotionMode = 0;
    bool bStageRootMotionOverrideActive = false;
    float SavedChargeWalkSpeed = 0.0f;
    bool bChargeWalkSpeedOverrideActive = false;
    float SavedPostAttackWalkSpeed = 0.0f;
    bool bPostAttackMovementLockActive = false;
    bool bPostAttackSpeedRecoveryActive = false;
    float PostAttackSpeedRecoveryElapsed = 0.0f;
    float PostAttackSpeedRecoveryStartSpeed = 0.0f;
    float PostAttackSpeedRecoveryTargetSpeed = 0.0f;
    mutable TMap<ERPGAttackInputType, int32> LastRandomStartStageByType;

    void ApplyAttackMovesetInternal(const URPGCombatMovesetDataAsset* InMoveset);
    bool CanStartAttack() const;
    int32 ResolveRandomizedStartStage(ERPGAttackInputType InputType) const;
    int32 ResolveComboStartStage(ERPGAttackInputType InputType) const;
    int32 ResolveNextComboStage(int32 FromStageIndex, ERPGAttackInputType InputType) const;
    ERPGAttackInputType ResolvePrimaryLightInputType() const;
    bool ConsumeStaminaForCurrentStage() const;
    void ClearBufferedComboInput();
    void StartAttackStage(int32 StageIndex, ERPGAttackInputType InputType, float InDamageMultiplier = 1.0f, float InStaminaMultiplier = 1.0f);
    void ResetComboState();
    void TickTrace();
    void UpdateAttackFacing(float DeltaTime);
    ERPGAttackInputType ResolveChargedInputType(float HeldTime) const;
    void ResolveChargedMultipliers(float HeldTime, float& OutDamageMultiplier, float& OutStaminaMultiplier) const;
    void EvaluateHoldHeavyInput();
    bool TryTriggerHoldHeavyAttack();
    void TryPlayFinisherMontage(AActor* TargetActor);
    UAnimInstance* GetOwnerAnimInstance() const;
    void PlayChargePresentationStart();
    void PlayChargePresentationLoop();
    void StopChargePresentation(float BlendOutTime);
    void ApplyChargeMovementPolicy();
    void RestoreChargeMovementPolicy();
    void ApplyStageMovementPolicy(const FRPGAttackStage& Stage);
    void RestoreStageMovementPolicy();
    void ApplyPostAttackMovementLock();
    void RestorePostAttackMovementLock();
};













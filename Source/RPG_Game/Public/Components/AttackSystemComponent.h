#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AttackSystemComponent.generated.h"

class AActor;
class ACharacter;
class UAnimMontage;
class URPGCombatMovesetDataAsset;
class USceneComponent;
class UPlayerStatsComponent;
class UTargetLockComponent;
class UCombatStateComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAttackStateChanged, int32, AttackIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAttackHit, AActor*, HitActor);

UENUM(BlueprintType)
enum class ERPGAttackInputType : uint8
{
    Light UMETA(DisplayName = "Light"),
    Heavy UMETA(DisplayName = "Heavy"),
    Special UMETA(DisplayName = "Special"),
    Charged UMETA(DisplayName = "Charged"),
    Aerial UMETA(DisplayName = "Aerial")
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
    TMap<ERPGAttackInputType, int32> AttackStartStageByType;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attack|State")
    ERPGAttackInputType BufferedInputType = ERPGAttackInputType::Light;

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

    UFUNCTION(BlueprintCallable, Category = "Attack|Setup")
    void ApplyAttackMoveset(URPGCombatMovesetDataAsset* InMoveset, bool bResetComboState = true);

    UFUNCTION(BlueprintCallable, Category = "Attack|Tuning")
    void SetWeaponAttackTuning(float InDamageMultiplier = 1.0f, float InStaminaMultiplier = 1.0f);

    UFUNCTION(BlueprintCallable, Category = "Attack")
    void HandleAttackInput();

    UFUNCTION(BlueprintCallable, Category = "Attack")
    void HandleAttackInputByType(ERPGAttackInputType InputType);

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
    TWeakObjectPtr<USceneComponent> TraceStartComponent;
    TWeakObjectPtr<USceneComponent> TraceEndComponent;
    TSet<TWeakObjectPtr<AActor>> HitActorsThisSwing;
    FTimerHandle ComboResetTimerHandle;
    FTimerHandle ComboBufferTimerHandle;
    FTimerHandle TraceTimerHandle;
    bool bTraceActive = false;

    void ApplyAttackMovesetInternal(const URPGCombatMovesetDataAsset* InMoveset);
    bool CanStartAttack() const;
    int32 ResolveComboStartStage(ERPGAttackInputType InputType) const;
    int32 ResolveNextComboStage(int32 FromStageIndex, ERPGAttackInputType InputType) const;
    bool ConsumeStaminaForCurrentStage();
    void ClearBufferedComboInput();
    void StartAttackStage(int32 StageIndex);
    void ResetComboState();
    void TickTrace();
    void UpdateAttackFacing(float DeltaTime);
};

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EvasionComponent.generated.h"

class ACharacter;
class UAnimMontage;
class UCombatStateComponent;
class UPlayerStatsComponent;
class USpringArmComponent;
class UTargetLockComponent;

UENUM(BlueprintType)
enum class ERPGEvasionType : uint8
{
    Dodge UMETA(DisplayName = "Dodge"),
    Roll UMETA(DisplayName = "Combat Roll")
};

UENUM(BlueprintType)
enum class ERPGEvasionDirection : uint8
{
    Forward UMETA(DisplayName = "Forward"),
    ForwardRight UMETA(DisplayName = "Forward Right"),
    Right UMETA(DisplayName = "Right"),
    BackwardRight UMETA(DisplayName = "Backward Right"),
    Backward UMETA(DisplayName = "Backward"),
    BackwardLeft UMETA(DisplayName = "Backward Left"),
    Left UMETA(DisplayName = "Left"),
    ForwardLeft UMETA(DisplayName = "Forward Left")
};

USTRUCT(BlueprintType)
struct FRPGEvasionDirectionalMontages
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evasion")
    UAnimMontage* Forward = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evasion")
    UAnimMontage* ForwardRight = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evasion")
    UAnimMontage* Right = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evasion")
    UAnimMontage* BackwardRight = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evasion")
    UAnimMontage* Backward = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evasion")
    UAnimMontage* BackwardLeft = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evasion")
    UAnimMontage* Left = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evasion")
    UAnimMontage* ForwardLeft = nullptr;

    UAnimMontage* SelectMontage(ERPGEvasionDirection Direction) const;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEvasionTriggered, ERPGEvasionType, EvasionType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEvasionFailed, ERPGEvasionType, EvasionType, FString, Reason);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInvulnerabilityChanged, bool, bIsInvulnerable);

UCLASS(ClassGroup=(RPG), BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class RPG_GAME_API UEvasionComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UEvasionComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evasion|General")
    bool bCanDodge = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evasion|General")
    bool bCanRoll = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evasion|General")
    bool bAllowEvasionDuringAttack = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evasion|General")
    bool bCancelGuardOnEvasion = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evasion|General")
    bool bRotateActorTowardEvasionDirection = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evasion|General")
    bool bForceOrientRotationToMovementDuringEvasion = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evasion|General")
    bool bDisableMontageRootMotionDuringEvasion = true;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Evasion|State")
    bool bIsEvading = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Evasion|State")
    bool bDodgeOnCooldown = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Evasion|State")
    bool bRollOnCooldown = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Evasion|State")
    ERPGEvasionDirection LastEvasionDirection = ERPGEvasionDirection::Forward;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evasion|Dodge", meta = (ClampMin = "0.0"))
    float DodgeStaminaCost = 18.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evasion|Dodge", meta = (ClampMin = "0.0"))
    float DodgeDistance = 420.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evasion|Dodge", meta = (ClampMin = "0.0"))
    float DodgeDuration = 0.28f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evasion|Dodge", meta = (ClampMin = "0.0"))
    float DodgeCooldown = 0.35f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evasion|Dodge", meta = (ClampMin = "0.0"))
    float DodgeInvulnerabilityStartDelay = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evasion|Dodge", meta = (ClampMin = "0.0"))
    float DodgeInvulnerabilityDuration = 0.18f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evasion|Dodge")
    bool bUseDirectionalDodgeMontages = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evasion|Dodge")
    FRPGEvasionDirectionalMontages DodgeDirectionalMontages;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evasion|Dodge")
    UAnimMontage* DodgeMontage = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evasion|Dodge|Camera")
    bool bOverrideCameraLagDuringDodge = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evasion|Dodge|Camera")
    bool bDisableCameraLagDuringDodge = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evasion|Dodge|Camera", meta = (ClampMin = "1.0"))
    float DodgeCameraLagSpeedOverride = 25.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evasion|Roll", meta = (ClampMin = "0.0"))
    float RollStaminaCost = 28.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evasion|Roll", meta = (ClampMin = "0.0"))
    float RollDistance = 620.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evasion|Roll", meta = (ClampMin = "0.0"))
    float RollDuration = 0.48f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evasion|Roll", meta = (ClampMin = "0.0"))
    float RollCooldown = 0.75f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evasion|Roll", meta = (ClampMin = "0.0"))
    float RollInvulnerabilityStartDelay = 0.02f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evasion|Roll", meta = (ClampMin = "0.0"))
    float RollInvulnerabilityDuration = 0.24f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evasion|Roll")
    bool bUseDirectionalRollMontages = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evasion|Roll")
    FRPGEvasionDirectionalMontages RollDirectionalMontages;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evasion|Roll")
    UAnimMontage* RollMontage = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evasion|Weapon", meta=(ClampMin="0.01"))
    float WeaponEvasionStaminaMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evasion|Weight")
    bool bUseWeightRestrictionForRoll = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evasion|Weight", meta = (ClampMin = "0.0"))
    float CurrentEquipWeight = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evasion|Weight", meta = (ClampMin = "0.0"))
    float MaxWeightForRoll = 60.0f;

    UPROPERTY(BlueprintAssignable, Category = "Evasion|Events")
    FOnEvasionTriggered OnEvasionStarted;

    UPROPERTY(BlueprintAssignable, Category = "Evasion|Events")
    FOnEvasionTriggered OnEvasionEnded;

    UPROPERTY(BlueprintAssignable, Category = "Evasion|Events")
    FOnEvasionFailed OnEvasionFailed;

    UPROPERTY(BlueprintAssignable, Category = "Evasion|Events")
    FOnInvulnerabilityChanged OnInvulnerabilityChanged;

    UFUNCTION(BlueprintCallable, Category = "Evasion")
    bool StartDodge();

    UFUNCTION(BlueprintCallable, Category = "Evasion")
    bool StartCombatRoll();

    UFUNCTION(BlueprintCallable, Category = "Evasion")
    void HandleDodgeInput();

    UFUNCTION(BlueprintCallable, Category = "Evasion")
    void HandleCombatRollInput();

    UFUNCTION(BlueprintPure, Category = "Evasion")
    bool CanCombatRollByWeight() const;

    UFUNCTION(BlueprintCallable, Category = "Evasion|Weight")
    void SetCurrentEquipWeight(float NewWeight);

    UFUNCTION(BlueprintCallable, Category = "Evasion|Weapon")
    void SetWeaponEvasionProfile(
        const FRPGEvasionDirectionalMontages& InDodgeDirectionalMontages,
        UAnimMontage* InDodgeMontage,
        const FRPGEvasionDirectionalMontages& InRollDirectionalMontages,
        UAnimMontage* InRollMontage,
        bool bInUseDirectionalDodgeMontages = true,
        bool bInUseDirectionalRollMontages = true);

    UFUNCTION(BlueprintCallable, Category = "Evasion|Weapon")
    void SetWeaponEvasionTuning(float InStaminaMultiplier = 1.0f);

protected:
    virtual void BeginPlay() override;

private:
    TWeakObjectPtr<ACharacter> CachedCharacter;
    TWeakObjectPtr<UPlayerStatsComponent> CachedStats;
    TWeakObjectPtr<UCombatStateComponent> CachedCombatState;
    TWeakObjectPtr<USpringArmComponent> CachedSpringArm;
    TWeakObjectPtr<UTargetLockComponent> CachedTargetLock;

    FTimerHandle EvasionEndTimerHandle;
    FTimerHandle DodgeCooldownTimerHandle;
    FTimerHandle RollCooldownTimerHandle;
    FTimerHandle InvulnerabilityStartTimerHandle;
    FTimerHandle InvulnerabilityEndTimerHandle;

    ERPGEvasionType ActiveEvasionType = ERPGEvasionType::Dodge;
    int32 InvulnerabilityRefCount = 0;
    bool bSavedCameraLagEnabled = false;
    float SavedCameraLagSpeed = 0.0f;
    bool bCameraLagOverrideActive = false;
    bool bSavedOrientRotationToMovement = false;
    bool bSavedUseControllerDesiredRotation = false;
    bool bMovementRotationOverrideActive = false;
    uint8 SavedAnimRootMotionMode = 0;
    bool bAnimRootMotionOverrideActive = false;

    bool TryStartEvasion(ERPGEvasionType EvasionType);
    bool IsBlockedByCombatState() const;
    bool ConsumeStamina(float Cost);
    FVector ResolveEvasionDirection() const;
    ERPGEvasionDirection ResolveDirectionType(const FVector& WorldDirection) const;
    void ApplyEvasionMovement(ERPGEvasionType EvasionType, const FVector& Direction) const;
    UAnimMontage* ResolveEvasionMontage(ERPGEvasionType EvasionType, ERPGEvasionDirection DirectionType) const;
    void PlayEvasionMontage(UAnimMontage* Montage) const;
    void StartCooldown(ERPGEvasionType EvasionType);
    void StartInvulnerabilityWindow(float StartDelay, float Duration);
    void ApplyCameraLagOverrideForEvasion(ERPGEvasionType EvasionType);
    void RestoreCameraLagOverride();
    void ApplyMovementRotationOverrideForEvasion();
    void RestoreMovementRotationOverride();
    void ApplyAnimRootMotionOverrideForEvasion();
    void RestoreAnimRootMotionOverrideForEvasion();
    void BeginInvulnerability();
    void EndInvulnerability();
    void FinishActiveEvasion();
    void ClearDodgeCooldown();
    void ClearRollCooldown();

    void BroadcastFail(ERPGEvasionType EvasionType, const FString& Reason);
};


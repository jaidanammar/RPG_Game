#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TargetLockComponent.generated.h"

class AActor;
class UCombatStateComponent;
class ULocomotionComponent;
class UPlayerStatsComponent;
class UEvasionComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLockTargetChanged, AActor*, NewTarget);

UCLASS(ClassGroup=(RPG), BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class RPG_GAME_API UTargetLockComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UTargetLockComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn")
    bool bAutoRotateToTarget = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn")
    bool bDriveControllerRotationWhenLocked = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn")
    bool bSuppressLockSteeringWhileEvading = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn", meta = (ClampMin = "0.1"))
    float RotationInterpSpeed = 12.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn", meta = (ClampMin = "0.1"))
    float ControllerRotationInterpSpeed = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn", meta = (ClampMin = "100.0"))
    float SearchRadius = 1500.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn", meta = (ClampMin = "100.0"))
    float MaxLockDistance = 1400.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn", meta = (ClampMin = "1.0", ClampMax = "180.0"))
    float MaxLockAngleDegrees = 70.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn")
    bool bRequireLineOfSight = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn")
    TEnumAsByte<ECollisionChannel> LineOfSightChannel = ECC_Visibility;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn")
    FName TargetableTag = TEXT("Damageable");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn")
    bool bBreakLockIfOutOfRange = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn", meta = (ClampMin = "100.0"))
    float BreakLockDistance = 1700.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn")
    bool bAutoRelockOnTargetLost = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn|Movement")
    bool bUseFocusedMovementStyle = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn|Movement", meta = (ClampMin = "0.1", ClampMax = "1.0"))
    float FocusedMovementSpeedMultiplier = 0.88f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn|Movement")
    bool bDisableSprintWhileLocked = false;

    UPROPERTY(BlueprintAssignable, Category = "LockOn|Events")
    FOnLockTargetChanged OnLockTargetChanged;

    UFUNCTION(BlueprintCallable, Category = "LockOn")
    bool ToggleLockOn();

    UFUNCTION(BlueprintCallable, Category = "LockOn")
    bool LockBestTarget();

    UFUNCTION(BlueprintCallable, Category = "LockOn")
    void ClearLock();

    UFUNCTION(BlueprintCallable, Category = "LockOn")
    bool CycleTarget(bool bCycleRight);

    UFUNCTION(BlueprintCallable, Category = "LockOn")
    bool SetLockTarget(AActor* NewTarget);

    UFUNCTION(BlueprintPure, Category = "LockOn")
    bool IsLockedOn() const;

    UFUNCTION(BlueprintPure, Category = "LockOn")
    AActor* GetLockTarget() const;

    UFUNCTION(BlueprintPure, Category = "LockOn")
    FVector GetLockTargetLocation() const;

    UFUNCTION(BlueprintPure, Category = "Locomotion")
    bool ShouldUseStrafeLocomotion() const;

    UFUNCTION(BlueprintPure, Category = "Locomotion")
    float GetSignedForwardSpeed() const;

    UFUNCTION(BlueprintPure, Category = "Locomotion")
    float GetSignedRightSpeed() const;

    UFUNCTION(BlueprintPure, Category = "Locomotion")
    float GetMovementDirectionDegrees() const;

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    TWeakObjectPtr<AActor> CurrentTarget;
    TWeakObjectPtr<UCombatStateComponent> CachedCombatState;
    TWeakObjectPtr<UEvasionComponent> CachedEvasion;
    TWeakObjectPtr<ULocomotionComponent> CachedLocomotion;
    TWeakObjectPtr<UPlayerStatsComponent> BoundTargetStats;

    bool bMovementStrafeOverrideApplied = false;
    bool bSavedOrientRotationToMovement = false;
    bool bSavedUseControllerDesiredRotation = false;
    bool bSavedUseControllerRotationYaw = false;

    void GatherCandidateTargets(TArray<AActor*>& OutTargets) const;
    AActor* FindBestTarget() const;
    bool IsValidLockTarget(AActor* Candidate) const;
    bool HasLineOfSightTo(AActor* Candidate) const;
    bool IsTargetDead(AActor* Candidate) const;
    void RefreshMovementFacingOverride();
    UFUNCTION()
    void HandleCurrentTargetDeath();

    void RotateOwnerTowardTarget(float DeltaTime);
    void UpdateControllerFacing(float DeltaTime);
};




#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TargetLockComponent.generated.h"

class AActor;

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

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    TWeakObjectPtr<AActor> CurrentTarget;

    void GatherCandidateTargets(TArray<AActor*>& OutTargets) const;
    AActor* FindBestTarget() const;
    bool IsValidLockTarget(AActor* Candidate) const;
    bool HasLineOfSightTo(AActor* Candidate) const;
    bool IsTargetDead(AActor* Candidate) const;
    void RotateOwnerTowardTarget(float DeltaTime);
    void UpdateControllerFacing(float DeltaTime);
};

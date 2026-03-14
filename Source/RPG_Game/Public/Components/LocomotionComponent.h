#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LocomotionComponent.generated.h"

class ACharacter;
class UCharacterMovementComponent;
class URPGLocomotionDataAsset;

UENUM(BlueprintType)
enum class ERPGLocomotionGait : uint8
{
    Walk UMETA(DisplayName = "Walk"),
    Run UMETA(DisplayName = "Run"),
    Sprint UMETA(DisplayName = "Sprint")
};

UENUM(BlueprintType)
enum class ERPGMovementCapability : uint8
{
    Walk UMETA(DisplayName = "Walk"),
    Run UMETA(DisplayName = "Run"),
    Sprint UMETA(DisplayName = "Sprint"),
    Jump UMETA(DisplayName = "Jump"),
    Crouch UMETA(DisplayName = "Crouch"),
    Dodge UMETA(DisplayName = "Dodge"),
    CombatRoll UMETA(DisplayName = "Combat Roll"),
    Parry UMETA(DisplayName = "Parry")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLocomotionProfileChanged);

UCLASS(ClassGroup=(RPG), BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class RPG_GAME_API ULocomotionComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    ULocomotionComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Locomotion")
    TObjectPtr<URPGLocomotionDataAsset> DefaultLocomotionData = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
    TObjectPtr<URPGLocomotionDataAsset> ActiveLocomotionData = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Locomotion")
    ERPGLocomotionGait DesiredGait = ERPGLocomotionGait::Run;

    UPROPERTY(BlueprintAssignable, Category = "Locomotion|Events")
    FOnLocomotionProfileChanged OnLocomotionProfileChanged;

    UFUNCTION(BlueprintCallable, Category = "Locomotion")
    void SetLocomotionData(URPGLocomotionDataAsset* NewData);

    UFUNCTION(BlueprintCallable, Category = "Locomotion")
    void ResetToDefaultLocomotionData();

    UFUNCTION(BlueprintCallable, Category = "Locomotion")
    void SetDesiredGait(ERPGLocomotionGait NewGait);

    UFUNCTION(BlueprintCallable, Category = "Locomotion")
    void SetSpeedMultiplier(FName Source, float Multiplier);

    UFUNCTION(BlueprintCallable, Category = "Locomotion")
    void ClearSpeedMultiplier(FName Source);

    UFUNCTION(BlueprintCallable, Category = "Locomotion")
    void SetCapabilityAllowed(FName Source, ERPGMovementCapability Capability, bool bAllowed);

    UFUNCTION(BlueprintCallable, Category = "Locomotion")
    void ClearCapabilityOverride(FName Source, ERPGMovementCapability Capability);

    UFUNCTION(BlueprintPure, Category = "Locomotion")
    bool IsCapabilityAllowed(ERPGMovementCapability Capability) const;

    UFUNCTION(BlueprintPure, Category = "Locomotion")
    float GetResolvedMaxWalkSpeed() const;

    UFUNCTION(BlueprintPure, Category = "Locomotion")
    float GetResolvedSpeedMultiplier() const;

    UFUNCTION(BlueprintCallable, Category = "Locomotion")
    void RefreshMovementSettings();

protected:
    virtual void BeginPlay() override;

private:
    TWeakObjectPtr<ACharacter> CachedCharacter;
    TWeakObjectPtr<UCharacterMovementComponent> CachedMoveComp;
    TMap<FName, float> SpeedMultipliersBySource;
    TMap<ERPGMovementCapability, TMap<FName, bool>> CapabilityOverridesBySource;
    float BaseFallbackWalkSpeed = 0.0f;
    float BaseFallbackJumpZVelocity = 0.0f;
    bool bHasCapturedBaseFallbackWalkSpeed = false;
    bool bHasCapturedBaseFallbackJumpZVelocity = false;

    float GetBaseSpeedForGait(ERPGLocomotionGait Gait) const;
    bool ResolveCapabilityAllowed(ERPGMovementCapability Capability) const;
    void EnsureCharacterAndMoveComp();
};



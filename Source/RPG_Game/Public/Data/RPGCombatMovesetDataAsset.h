#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Components/AttackSystemComponent.h"
#include "RPGCombatMovesetDataAsset.generated.h"

UCLASS(BlueprintType)
class RPG_GAME_API URPGCombatMovesetDataAsset : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
    TArray<FRPGAttackStage> AttackStages;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack|Combo")
    TMap<ERPGAttackInputType, int32> AttackStartStageByType;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack|Combo")
    TMap<ERPGAttackInputType, FRPGAttackStartRandomPool> RandomizedStartStagesByType;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack|Combo")
    bool bUseComboWindowLock = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack|Combo")
    bool bAllowSequentialComboFallback = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack|Combo", meta = (ClampMin = "0.01"))
    float ComboInputBufferDuration = 0.25f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack|Input")
    bool bEnableHoldHeavyFromPrimaryInput = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack|Input", meta=(ClampMin="0.01"))
    float HoldHeavyTriggerTime = 0.2f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack|Input")
    ERPGAttackInputType HoldHeavyInputType = ERPGAttackInputType::Heavy;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack|Input|Light Variants")
    bool bEnableDistanceBasedLightVariants = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack|Input|Light Variants")
    ERPGAttackInputType LightSlashInputType = ERPGAttackInputType::LightSlash;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack|Input|Light Variants")
    ERPGAttackInputType LightStabInputType = ERPGAttackInputType::LightStab;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack|Input|Light Variants", meta=(ClampMin="0.0"))
    float LightStabMinDistance = 220.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack|Input|Light Variants", meta=(ClampMin="0.0"))
    float LightStabMaxDistance = 500.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack|Charged")
    bool bEnableChargedAttack = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack|Charged", meta=(ClampMin="0.01"))
    float MinChargeTime = 0.35f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack|Charged", meta=(ClampMin="0.05"))
    float MaxChargeTime = 1.1f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack|Charged")
    bool bRequireFullChargeForChargedInput = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack|Charged")
    ERPGAttackInputType PartialChargeInputType = ERPGAttackInputType::Light;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack|Charged")
    ERPGAttackInputType FullChargeInputType = ERPGAttackInputType::Charged;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack|Charged")
    bool bScaleChargedDamageByHoldTime = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack|Charged", meta=(ClampMin="0.01"))
    float MinChargedDamageMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack|Charged", meta=(ClampMin="0.01"))
    float MaxChargedDamageMultiplier = 1.8f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack|Charged", meta=(ClampMin="0.01"))
    float MinChargedStaminaMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack|Charged", meta=(ClampMin="0.01"))
    float MaxChargedStaminaMultiplier = 1.4f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack|Charged|Presentation")
    bool bAutoPlayChargePresentation = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack|Charged|Presentation")
    TObjectPtr<UAnimMontage> ChargeStartMontage = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack|Charged|Presentation")
    TObjectPtr<UAnimMontage> ChargeLoopMontage = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack|Charged|Presentation", meta=(ClampMin="0.01"))
    float ChargeStartMontagePlayRate = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack|Charged|Presentation", meta=(ClampMin="0.01"))
    float ChargeLoopMontagePlayRate = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack|Charged|Presentation", meta=(ClampMin="0.0"))
    float ChargeReleaseBlendOutTime = 0.08f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack|Charged|Presentation")
    bool bStopChargeMontagesOnRelease = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack|Finisher")
    bool bEnableFinishers = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack|Finisher", meta=(ClampMin="0.0", ClampMax="1.0"))
    float FinisherChanceOnLethalHit = 0.35f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack|Finisher")
    TArray<TObjectPtr<UAnimMontage>> FinisherMontages;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack|Finisher", meta=(ClampMin="0.01"))
    float FinisherMontagePlayRate = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack|Finisher")
    bool bStopCurrentMontageForFinisher = true;
};










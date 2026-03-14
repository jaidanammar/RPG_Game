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

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack|Flow")
    ERPGAttackContinuationMode AttackContinuationMode = ERPGAttackContinuationMode::ContextualSelection;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack|Combo", meta = (ClampMin = "0.01"))
    float ComboInputBufferDuration = 0.25f;
};












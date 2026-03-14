#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "RPGAnimNotifyState_LocomotionModifier.generated.h"

UCLASS(Blueprintable, meta = (DisplayName = "RPG Locomotion Modifier"))
class RPG_GAME_API URPGAnimNotifyState_LocomotionModifier : public UAnimNotifyState
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Locomotion", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float SpeedMultiplier = 0.3f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Locomotion")
    bool bDisableSprint = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Locomotion")
    bool bDisableJump = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Locomotion")
    FName SourceName = TEXT("AnimationLocomotionModifier");

    virtual FString GetNotifyName_Implementation() const override;
    virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
    virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};

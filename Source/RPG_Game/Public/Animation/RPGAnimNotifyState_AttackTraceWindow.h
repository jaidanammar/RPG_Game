#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "RPGAnimNotifyState_AttackTraceWindow.generated.h"

UCLASS(Blueprintable, meta = (DisplayName = "RPG Attack Trace Window"))
class RPG_GAME_API URPGAnimNotifyState_AttackTraceWindow : public UAnimNotifyState
{
    GENERATED_BODY()

public:
    virtual FString GetNotifyName_Implementation() const override;
    virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
    virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};
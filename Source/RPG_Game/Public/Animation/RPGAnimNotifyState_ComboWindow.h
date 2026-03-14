#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "RPGAnimNotifyState_ComboWindow.generated.h"

UCLASS(Blueprintable, meta = (DisplayName = "RPG Combo Window"))
class RPG_GAME_API URPGAnimNotifyState_ComboWindow : public UAnimNotifyState
{
    GENERATED_BODY()

public:
    virtual FString GetNotifyName_Implementation() const override;
    virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
    virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};
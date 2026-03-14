#include "Animation/RPGAnimNotifyState_AttackTraceWindow.h"

#include "Components/AttackSystemComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"

FString URPGAnimNotifyState_AttackTraceWindow::GetNotifyName_Implementation() const
{
    return TEXT("RPG Attack Trace Window");
}

void URPGAnimNotifyState_AttackTraceWindow::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

    if (!MeshComp)
    {
        return;
    }

    if (AActor* Owner = MeshComp->GetOwner())
    {
        if (UAttackSystemComponent* AttackSystem = Owner->FindComponentByClass<UAttackSystemComponent>())
        {
            AttackSystem->StartConfiguredTrace();
        }
    }
}

void URPGAnimNotifyState_AttackTraceWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyEnd(MeshComp, Animation, EventReference);

    if (!MeshComp)
    {
        return;
    }

    if (AActor* Owner = MeshComp->GetOwner())
    {
        if (UAttackSystemComponent* AttackSystem = Owner->FindComponentByClass<UAttackSystemComponent>())
        {
            AttackSystem->StopTrace();
        }
    }
}
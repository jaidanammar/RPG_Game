#include "Animation/RPGAnimNotifyState_ComboWindow.h"

#include "Components/AttackSystemComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"

FString URPGAnimNotifyState_ComboWindow::GetNotifyName_Implementation() const
{
    return TEXT("RPG Combo Window");
}

void URPGAnimNotifyState_ComboWindow::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
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
            AttackSystem->OpenComboWindow();
        }
    }
}

void URPGAnimNotifyState_ComboWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
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
            AttackSystem->CloseComboWindow();
        }
    }
}
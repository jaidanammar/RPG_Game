#include "Animation/RPGAnimNotifyState_LocomotionModifier.h"

#include "Components/LocomotionComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"

FString URPGAnimNotifyState_LocomotionModifier::GetNotifyName_Implementation() const
{
    return TEXT("RPG Locomotion Modifier");
}

void URPGAnimNotifyState_LocomotionModifier::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

    if (!MeshComp)
    {
        return;
    }

    if (AActor* Owner = MeshComp->GetOwner())
    {
        if (ULocomotionComponent* Locomotion = Owner->FindComponentByClass<ULocomotionComponent>())
        {
            const FName EffectiveSource = SourceName.IsNone() ? TEXT("AnimationLocomotionModifier") : SourceName;
            Locomotion->SetSpeedMultiplier(EffectiveSource, FMath::Clamp(SpeedMultiplier, 0.0f, 1.0f));

            if (bDisableSprint)
            {
                Locomotion->SetCapabilityAllowed(EffectiveSource, ERPGMovementCapability::Sprint, false);
            }

            if (bDisableJump)
            {
                Locomotion->SetCapabilityAllowed(EffectiveSource, ERPGMovementCapability::Jump, false);
            }
        }
    }
}

void URPGAnimNotifyState_LocomotionModifier::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyEnd(MeshComp, Animation, EventReference);

    if (!MeshComp)
    {
        return;
    }

    if (AActor* Owner = MeshComp->GetOwner())
    {
        if (ULocomotionComponent* Locomotion = Owner->FindComponentByClass<ULocomotionComponent>())
        {
            const FName EffectiveSource = SourceName.IsNone() ? TEXT("AnimationLocomotionModifier") : SourceName;
            Locomotion->ClearSpeedMultiplier(EffectiveSource);
            Locomotion->ClearCapabilityOverride(EffectiveSource, ERPGMovementCapability::Sprint);
            Locomotion->ClearCapabilityOverride(EffectiveSource, ERPGMovementCapability::Jump);
        }
    }
}

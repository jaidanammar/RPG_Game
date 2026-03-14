#include "Characters/RPGEnemyCharacter.h"

#include "AIController.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimSequenceBase.h"
#include "Components/AttackSystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/CombatStateComponent.h"
#include "Components/HostileEnemyComponent.h"
#include "Components/LocomotionComponent.h"
#include "Components/PlayerStatsComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TargetLockComponent.h"
#include "Components/WeaponLoadoutComponent.h"
#include "Data/RPGWeaponDataAssets.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "RPGCharacterAnimInstance.h"
#include "UObject/ConstructorHelpers.h"

ARPGEnemyCharacter::ARPGEnemyCharacter()
{
    PrimaryActorTick.bCanEverTick = false;

    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
    AIControllerClass = AAIController::StaticClass();

    StatsComponent = CreateDefaultSubobject<UPlayerStatsComponent>(TEXT("StatsComponent"));
    CombatStateComponent = CreateDefaultSubobject<UCombatStateComponent>(TEXT("CombatStateComponent"));
    AttackSystemComponent = CreateDefaultSubobject<UAttackSystemComponent>(TEXT("AttackSystemComponent"));
    LocomotionComponent = CreateDefaultSubobject<ULocomotionComponent>(TEXT("LocomotionComponent"));
    TargetLockComponent = CreateDefaultSubobject<UTargetLockComponent>(TEXT("TargetLockComponent"));
    WeaponLoadoutComponent = CreateDefaultSubobject<UWeaponLoadoutComponent>(TEXT("WeaponLoadoutComponent"));
    HostileEnemyComponent = CreateDefaultSubobject<UHostileEnemyComponent>(TEXT("HostileEnemyComponent"));

    WeaponTraceStart = CreateDefaultSubobject<USceneComponent>(TEXT("WeaponTraceStart"));
    WeaponTraceStart->SetupAttachment(GetMesh());

    WeaponTraceEnd = CreateDefaultSubobject<USceneComponent>(TEXT("WeaponTraceEnd"));
    WeaponTraceEnd->SetupAttachment(GetMesh());

    VisibleWeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisibleWeaponMesh"));
    VisibleWeaponMesh->SetupAttachment(GetMesh(), EquippedWeaponSocketName);
    VisibleWeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    VisibleWeaponMesh->SetGenerateOverlapEvents(false);
    VisibleWeaponMesh->SetCanEverAffectNavigation(false);

    Tags.AddUnique(TEXT("Damageable"));

    if (WeaponLoadoutComponent && !WeaponLoadoutComponent->EquippedWeaponInstance)
    {
        static ConstructorHelpers::FObjectFinder<URPGWeaponInstanceDataAsset> DefaultSwordWeapon(
            TEXT("/Game/Core/Combat/Weapons/Sword/Data/DA_WeaponInstance_Sword_Base.DA_WeaponInstance_Sword_Base"));
        if (DefaultSwordWeapon.Succeeded())
        {
            WeaponLoadoutComponent->EquippedWeaponInstance = DefaultSwordWeapon.Object;
        }
    }

    if (VisibleWeaponMesh)
    {
        static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultSwordMesh(
            TEXT("/Game/Core/Blueprints/Items/Weapons/Sword/SM_uitlbiaga_tier_2.SM_uitlbiaga_tier_2"));
        if (DefaultSwordMesh.Succeeded())
        {
            VisibleWeaponMesh->SetStaticMesh(DefaultSwordMesh.Object);
        }
    }

    if (GetCharacterMovement())
    {
        GetCharacterMovement()->bOrientRotationToMovement = true;
        GetCharacterMovement()->bUseControllerDesiredRotation = false;
        GetCharacterMovement()->MaxWalkSpeed = 260.0f;
    }
}

void ARPGEnemyCharacter::BeginPlay()
{
    Super::BeginPlay();

    if (AttackSystemComponent)
    {
        AttackSystemComponent->DamageableTag = TEXT("Damageable");
    }

    if (CombatStateComponent)
    {
        CombatStateComponent->OnHitReactionUpdated.AddDynamic(this, &ARPGEnemyCharacter::HandleHitReactionUpdated);
    }

    if (StatsComponent)
    {
        StatsComponent->OnDeath.AddDynamic(this, &ARPGEnemyCharacter::HandleDeath);
    }
}

void ARPGEnemyCharacter::HandleHitReactionUpdated(ERPGHitReactionStrength ReactionStrength, ERPGHitDirection HitDirection)
{
    if (!bPlayHitReactions || !GetMesh() || !StatsComponent || StatsComponent->IsDead())
    {
        return;
    }

    UAnimSequenceBase* HitReaction = Cast<UAnimSequenceBase>(ResolveHitReactionAnimation(ReactionStrength, HitDirection));
    if (!HitReaction)
    {
        return;
    }

    if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
    {
        AnimInstance->PlaySlotAnimationAsDynamicMontage(
            HitReaction,
            HitReactionSlotName,
            HitReactionBlendInTime,
            HitReactionBlendOutTime,
            1.0f,
            1,
            0.0f,
            0.0f);
    }
}

void ARPGEnemyCharacter::HandleDeath()
{
    if (AAIController* AIController = Cast<AAIController>(GetController()))
    {
        AIController->StopMovement();
        AIController->UnPossess();
    }

    if (HostileEnemyComponent)
    {
        HostileEnemyComponent->ClearHostileTarget();
    }

    if (AttackSystemComponent)
    {
        AttackSystemComponent->StopCombo();
    }

    if (GetCharacterMovement())
    {
        GetCharacterMovement()->DisableMovement();
        GetCharacterMovement()->StopMovementImmediately();
    }

    if (GetCapsuleComponent())
    {
        GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }

    if (bEnableRagdollOnDeath)
    {
        EnterRagdoll();
    }

    if (DeathLifeSpan > 0.0f)
    {
        SetLifeSpan(DeathLifeSpan);
    }
}

UAnimationAsset* ARPGEnemyCharacter::ResolveHitReactionAnimation(ERPGHitReactionStrength ReactionStrength, ERPGHitDirection HitDirection) const
{
    const URPGCharacterAnimInstance* RPGAnimInstance = Cast<URPGCharacterAnimInstance>(GetMesh() ? GetMesh()->GetAnimInstance() : nullptr);
    if (!RPGAnimInstance)
    {
        return nullptr;
    }

    if (ReactionStrength == ERPGHitReactionStrength::GuardBreak)
    {
        return RPGAnimInstance->GuardBreakAnimation;
    }

    const bool bBackHit = HitDirection == ERPGHitDirection::Back;
    if (ReactionStrength == ERPGHitReactionStrength::Heavy)
    {
        return bBackHit ? RPGAnimInstance->HitHeavyBackAnimation : RPGAnimInstance->HitHeavyFrontAnimation;
    }

    if (ReactionStrength == ERPGHitReactionStrength::Light)
    {
        return bBackHit ? RPGAnimInstance->HitLightBackAnimation : RPGAnimInstance->HitLightFrontAnimation;
    }

    return nullptr;
}

void ARPGEnemyCharacter::EnterRagdoll()
{
    if (!GetMesh())
    {
        return;
    }

    GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
    GetMesh()->SetAllBodiesSimulatePhysics(true);
    GetMesh()->SetSimulatePhysics(true);
    GetMesh()->WakeAllRigidBodies();
    GetMesh()->bBlendPhysics = true;
}
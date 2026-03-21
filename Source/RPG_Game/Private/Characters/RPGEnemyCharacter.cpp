#include "Characters/RPGEnemyCharacter.h"

#include "AIController.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimSequenceBase.h"
#include "Components/AttackSystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/CombatStateComponent.h"
#include "Components/EnemyCombatDisplayComponent.h"
#include "Components/EvasionComponent.h"
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
    EvasionComponent = CreateDefaultSubobject<UEvasionComponent>(TEXT("EvasionComponent"));
    WeaponLoadoutComponent = CreateDefaultSubobject<UWeaponLoadoutComponent>(TEXT("WeaponLoadoutComponent"));
    HostileEnemyComponent = CreateDefaultSubobject<UHostileEnemyComponent>(TEXT("HostileEnemyComponent"));
    EnemyCombatDisplayComponent = CreateDefaultSubobject<UEnemyCombatDisplayComponent>(TEXT("EnemyCombatDisplayComponent"));

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
            TEXT("/Game/Core/Combat/Weapons/Sword/Data/Steel/DA_WeaponInstance_Sword_Steel.DA_WeaponInstance_Sword_Steel"));
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
        GetCharacterMovement()->bOrientRotationToMovement = false;
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

    if (bAutoApplyArchetypeTuning)
    {
        ApplyArchetypeTuning();
    }
}

void ARPGEnemyCharacter::ApplyArchetypeTuning()
{
    if (LocomotionComponent)
    {
        LocomotionComponent->ClearSpeedMultiplier(TEXT("ArmorProfile"));
        LocomotionComponent->SetDesiredGait(ERPGLocomotionGait::Run);
    }

    if (StatsComponent)
    {
        StatsComponent->MaxHealth = 100.0f;
        StatsComponent->CurrentHealth = StatsComponent->MaxHealth;
        StatsComponent->MaxStamina = 100.0f;
        StatsComponent->CurrentStamina = StatsComponent->MaxStamina;
        StatsComponent->BroadcastAllStats();
    }

    if (HostileEnemyComponent)
    {
        HostileEnemyComponent->bMaintainFocusLockWhileHostile = true;
        HostileEnemyComponent->AttackRange = 180.0f;
        HostileEnemyComponent->AttackInterval = 1.52f;
        HostileEnemyComponent->PressureAttackChanceBonus = 0.1f;
        HostileEnemyComponent->CloseRangeAttackChanceBonus = 0.05f;
        HostileEnemyComponent->PostGuardAttackDelay = 0.2f;
        HostileEnemyComponent->RetreatDuration = 0.34f;
        HostileEnemyComponent->AdvanceCommitDuration = 0.38f;
        HostileEnemyComponent->FaceTargetInterpSpeed = 7.0f;
        HostileEnemyComponent->ThreatGuardRange = 228.0f;
        HostileEnemyComponent->ThreatEvadeRange = 198.0f;
        HostileEnemyComponent->PersonalSpaceDistance = 88.0f;
        HostileEnemyComponent->PersonalSpaceStrafeMultiplier = 1.7f;
        HostileEnemyComponent->HeavyAttackChance = 0.14f;
        HostileEnemyComponent->PunishHeavyAttackChance = 0.3f;
        HostileEnemyComponent->ComboFollowUpChance = 0.22f;
        HostileEnemyComponent->PunishComboFollowUpChance = 0.44f;
        HostileEnemyComponent->ParryAgainstAttackChance = 0.12f;
        HostileEnemyComponent->GuardParryChance = 0.18f;
        HostileEnemyComponent->ParryThreatRange = 172.0f;
    }

        if (TargetLockComponent)
    {
        TargetLockComponent->bUseFocusedMovementStyle = true;
        TargetLockComponent->FocusedMovementSpeedMultiplier = 0.94f;
        TargetLockComponent->bAutoRotateToTarget = true;
        TargetLockComponent->bDriveControllerRotationWhenLocked = true;
        TargetLockComponent->RotationInterpSpeed = 9.0f;
        TargetLockComponent->ControllerRotationInterpSpeed = 8.0f;
    }

    if (CombatStateComponent)
    {
        CombatStateComponent->bAllowGuardState = true;
        CombatStateComponent->bAllowParry = true;
        CombatStateComponent->bBeginParryOnGuardPressed = true;
        CombatStateComponent->bEnterGuardStateOnParrySuccess = true;
        CombatStateComponent->GuardWalkSpeedMultiplier = 0.72f;
        CombatStateComponent->ParryWindowDuration = 0.24f;
        CombatStateComponent->PerfectParryWindowDuration = 0.1f;
        CombatStateComponent->ParryCooldown = 0.42f;
    }
    if (EvasionComponent)
    {
        EvasionComponent->bCanDodge = true;
        EvasionComponent->bCanRoll = false;
        EvasionComponent->bAllowEvasionDuringAttack = false;
        EvasionComponent->bCancelGuardOnEvasion = true;
        EvasionComponent->bRotateActorTowardEvasionDirection = false;
    }

    switch (ArmorType)
    {
    case ERPGEnemyArmorType::LightArmor:
        if (EvasionComponent)
        {
            EvasionComponent->DodgeDistance = 460.0f;
            EvasionComponent->DodgeDuration = 0.26f;
            EvasionComponent->DodgeCooldown = 0.3f;
            EvasionComponent->DodgeStaminaCost = 14.0f;
        }

        if (LocomotionComponent)
        {
            LocomotionComponent->SetSpeedMultiplier(TEXT("ArmorProfile"), 1.08f);
        }
        else if (GetCharacterMovement())
        {
            GetCharacterMovement()->MaxWalkSpeed = 280.0f;
        }

        if (StatsComponent)
        {
            StatsComponent->MaxHealth = 90.0f;
            StatsComponent->CurrentHealth = StatsComponent->MaxHealth;
            StatsComponent->MaxStamina = 100.0f;
            StatsComponent->CurrentStamina = StatsComponent->MaxStamina;
        }
        break;

    case ERPGEnemyArmorType::HeavyArmor:
    default:
        if (EvasionComponent)
        {
            EvasionComponent->DodgeDistance = 325.0f;
            EvasionComponent->DodgeDuration = 0.24f;
            EvasionComponent->DodgeCooldown = 0.75f;
            EvasionComponent->DodgeStaminaCost = 18.0f;
        }

        if (LocomotionComponent)
        {
            LocomotionComponent->SetSpeedMultiplier(TEXT("ArmorProfile"), 0.88f);
        }
        else if (GetCharacterMovement())
        {
            GetCharacterMovement()->MaxWalkSpeed = 228.0f;
        }

        if (StatsComponent)
        {
            StatsComponent->MaxHealth = 125.0f;
            StatsComponent->CurrentHealth = StatsComponent->MaxHealth;
            StatsComponent->MaxStamina = 115.0f;
            StatsComponent->CurrentStamina = StatsComponent->MaxStamina;
        }
        break;
    }

    if (HostileEnemyComponent)
    {
        HostileEnemyComponent->bMaintainFocusLockWhileHostile = true;
        switch (TrainingLevel)
        {
        case ERPGEnemyTrainingLevel::Untrained:
            HostileEnemyComponent->BaseAttackChance = 0.22f;
            HostileEnemyComponent->PunishAttackChance = 0.5f;
            HostileEnemyComponent->MinDecisionInterval = 0.32f;
            HostileEnemyComponent->MaxDecisionInterval = 0.58f;
            HostileEnemyComponent->GuardAgainstAttackChance = (ArmorType == ERPGEnemyArmorType::HeavyArmor) ? 0.42f : 0.32f;
            HostileEnemyComponent->MinGuardHoldDuration = 0.24f;
            HostileEnemyComponent->MaxGuardHoldDuration = 0.46f;
            HostileEnemyComponent->HesitationAfterTakingHit = (ArmorType == ERPGEnemyArmorType::HeavyArmor) ? 0.34f : 0.4f;
            HostileEnemyComponent->FeintChance = 0.02f;
            HostileEnemyComponent->PressureBuildPerSecond = (ArmorType == ERPGEnemyArmorType::HeavyArmor) ? 0.1f : 0.12f;
            HostileEnemyComponent->PressureDecayPerSecond = (ArmorType == ERPGEnemyArmorType::HeavyArmor) ? 0.14f : 0.16f;
            HostileEnemyComponent->PreferredCombatDistance = (ArmorType == ERPGEnemyArmorType::HeavyArmor) ? 132.0f : 140.0f;
            HostileEnemyComponent->CombatDistanceTolerance = (ArmorType == ERPGEnemyArmorType::HeavyArmor) ? 30.0f : 34.0f;
            HostileEnemyComponent->StrafeWeight = (ArmorType == ERPGEnemyArmorType::HeavyArmor) ? 0.16f : 0.22f;
            HostileEnemyComponent->RecentDamagePenaltyDuration = 0.6f;
            HostileEnemyComponent->DamageNervesPenalty = 0.12f;
            HostileEnemyComponent->MaxPressureBonus = 0.2f;
            HostileEnemyComponent->EvadeAgainstAttackChance = (ArmorType == ERPGEnemyArmorType::HeavyArmor) ? 0.1f : 0.22f;
            HostileEnemyComponent->HeavyAttackChance = (ArmorType == ERPGEnemyArmorType::HeavyArmor) ? 0.14f : 0.08f;
            HostileEnemyComponent->PunishHeavyAttackChance = (ArmorType == ERPGEnemyArmorType::HeavyArmor) ? 0.24f : 0.18f;
            HostileEnemyComponent->ComboFollowUpChance = (ArmorType == ERPGEnemyArmorType::HeavyArmor) ? 0.14f : 0.18f;
            HostileEnemyComponent->PunishComboFollowUpChance = (ArmorType == ERPGEnemyArmorType::HeavyArmor) ? 0.28f : 0.34f;
            HostileEnemyComponent->ParryAgainstAttackChance = (ArmorType == ERPGEnemyArmorType::HeavyArmor) ? 0.08f : 0.06f;
            HostileEnemyComponent->GuardParryChance = (ArmorType == ERPGEnemyArmorType::HeavyArmor) ? 0.12f : 0.10f;
            HostileEnemyComponent->ParryThreatRange = (ArmorType == ERPGEnemyArmorType::HeavyArmor) ? 164.0f : 156.0f;
            break;

        case ERPGEnemyTrainingLevel::Trained:
        default:
            HostileEnemyComponent->BaseAttackChance = (ArmorType == ERPGEnemyArmorType::HeavyArmor) ? 0.34f : 0.38f;
            HostileEnemyComponent->PunishAttackChance = (ArmorType == ERPGEnemyArmorType::HeavyArmor) ? 0.68f : 0.76f;
            HostileEnemyComponent->MinDecisionInterval = (ArmorType == ERPGEnemyArmorType::HeavyArmor) ? 0.24f : 0.22f;
            HostileEnemyComponent->MaxDecisionInterval = (ArmorType == ERPGEnemyArmorType::HeavyArmor) ? 0.38f : 0.34f;
            HostileEnemyComponent->GuardAgainstAttackChance = (ArmorType == ERPGEnemyArmorType::HeavyArmor) ? 0.78f : 0.42f;
            HostileEnemyComponent->MinGuardHoldDuration = 0.18f;
            HostileEnemyComponent->MaxGuardHoldDuration = 0.36f;
            HostileEnemyComponent->HesitationAfterTakingHit = (ArmorType == ERPGEnemyArmorType::HeavyArmor) ? 0.18f : 0.16f;
            HostileEnemyComponent->FeintChance = (ArmorType == ERPGEnemyArmorType::HeavyArmor) ? 0.06f : 0.12f;
            HostileEnemyComponent->PressureBuildPerSecond = (ArmorType == ERPGEnemyArmorType::HeavyArmor) ? 0.22f : 0.26f;
            HostileEnemyComponent->PressureDecayPerSecond = (ArmorType == ERPGEnemyArmorType::HeavyArmor) ? 0.08f : 0.09f;
            HostileEnemyComponent->PreferredCombatDistance = (ArmorType == ERPGEnemyArmorType::HeavyArmor) ? 126.0f : 132.0f;
            HostileEnemyComponent->CombatDistanceTolerance = (ArmorType == ERPGEnemyArmorType::HeavyArmor) ? 22.0f : 26.0f;
            HostileEnemyComponent->StrafeWeight = (ArmorType == ERPGEnemyArmorType::HeavyArmor) ? 0.18f : 0.28f;
            HostileEnemyComponent->RecentDamagePenaltyDuration = 0.45f;
            HostileEnemyComponent->DamageNervesPenalty = 0.08f;
            HostileEnemyComponent->MaxPressureBonus = 0.28f;
            HostileEnemyComponent->EvadeAgainstAttackChance = (ArmorType == ERPGEnemyArmorType::HeavyArmor) ? 0.14f : 0.48f;
            HostileEnemyComponent->HeavyAttackChance = (ArmorType == ERPGEnemyArmorType::HeavyArmor) ? 0.18f : 0.12f;
            HostileEnemyComponent->PunishHeavyAttackChance = (ArmorType == ERPGEnemyArmorType::HeavyArmor) ? 0.32f : 0.38f;
            HostileEnemyComponent->ComboFollowUpChance = (ArmorType == ERPGEnemyArmorType::HeavyArmor) ? 0.22f : 0.34f;
            HostileEnemyComponent->PunishComboFollowUpChance = (ArmorType == ERPGEnemyArmorType::HeavyArmor) ? 0.42f : 0.58f;
            HostileEnemyComponent->ParryAgainstAttackChance = (ArmorType == ERPGEnemyArmorType::HeavyArmor) ? 0.22f : 0.18f;
            HostileEnemyComponent->GuardParryChance = (ArmorType == ERPGEnemyArmorType::HeavyArmor) ? 0.24f : 0.22f;
            HostileEnemyComponent->ParryThreatRange = (ArmorType == ERPGEnemyArmorType::HeavyArmor) ? 176.0f : 184.0f;
            break;
        }

        HostileEnemyComponent->ChaseStopDistance = FMath::Max(HostileEnemyComponent->PreferredCombatDistance + 32.0f, 160.0f);
        HostileEnemyComponent->RetreatDistance = FMath::Max(78.0f, HostileEnemyComponent->PreferredCombatDistance - 40.0f);
        HostileEnemyComponent->PersonalSpaceDistance = FMath::Max(72.0f, HostileEnemyComponent->PreferredCombatDistance - 48.0f);

                if (CombatStateComponent)
        {
            CombatStateComponent->GuardWalkSpeedMultiplier = (ArmorType == ERPGEnemyArmorType::HeavyArmor) ? 0.68f : 0.76f;
            CombatStateComponent->ParryWindowDuration = (TrainingLevel == ERPGEnemyTrainingLevel::Trained) ? 0.26f : 0.2f;
            CombatStateComponent->PerfectParryWindowDuration = (TrainingLevel == ERPGEnemyTrainingLevel::Trained) ? 0.12f : 0.08f;
            CombatStateComponent->ParryCooldown = (TrainingLevel == ERPGEnemyTrainingLevel::Trained) ? 0.34f : 0.5f;
        }

        if (TargetLockComponent)
        {
            TargetLockComponent->FocusedMovementSpeedMultiplier = (ArmorType == ERPGEnemyArmorType::HeavyArmor) ? 0.9f : 0.96f;
            TargetLockComponent->RotationInterpSpeed = (ArmorType == ERPGEnemyArmorType::HeavyArmor) ? 8.0f : 10.0f;
            TargetLockComponent->ControllerRotationInterpSpeed = (ArmorType == ERPGEnemyArmorType::HeavyArmor) ? 7.5f : 9.0f;
        }

        if (EvasionComponent)
        {
            EvasionComponent->bAllowEvasionDuringAttack = (TrainingLevel == ERPGEnemyTrainingLevel::Trained && ArmorType == ERPGEnemyArmorType::LightArmor);
        }
    }

    if (StatsComponent)
    {
        StatsComponent->BroadcastAllStats();
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
        HostileEnemyComponent->bMaintainFocusLockWhileHostile = true;
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



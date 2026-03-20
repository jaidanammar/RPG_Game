#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Components/EvasionComponent.h"
#include "Data/RPGCombatMovesetDataAsset.h"
#include "RPGWeaponDataAssets.generated.h"

class UAnimationAsset;
class URPGLocomotionDataAsset;
class UStaticMesh;

UENUM(BlueprintType)
enum class ERPGWeaponType : uint8
{
    Unarmed UMETA(DisplayName = "Unarmed"),
    Sword UMETA(DisplayName = "Sword"),
    SwordShield UMETA(DisplayName = "Sword & Shield"),
    DualSword UMETA(DisplayName = "Dual Sword"),
    Greatsword UMETA(DisplayName = "Greatsword"),
    Battleaxe UMETA(DisplayName = "Battleaxe"),
    DualBattleaxe UMETA(DisplayName = "Dual Battleaxe")
};

UENUM(BlueprintType)
enum class ERPGAnimationSlot : uint8
{
    Idle UMETA(DisplayName = "Idle"),
    Walk UMETA(DisplayName = "Walk"),
    Run UMETA(DisplayName = "Run"),
    Sprint UMETA(DisplayName = "Sprint"),
    FocusIdle UMETA(DisplayName = "Focus Idle"),
    FocusMove UMETA(DisplayName = "Focus Move"),
    GuardIdle UMETA(DisplayName = "Guard Idle"),
    GuardMove UMETA(DisplayName = "Guard Move"),
    RunStart UMETA(DisplayName = "Run Start"),
    RunStop UMETA(DisplayName = "Run Stop"),
    JumpStart UMETA(DisplayName = "Jump Start"),
    JumpLoop UMETA(DisplayName = "Jump Loop"),
    JumpLand UMETA(DisplayName = "Jump Land"),
    FallLoop UMETA(DisplayName = "Fall Loop"),
    GuardEnter UMETA(DisplayName = "Guard Enter"),
    GuardLoop UMETA(DisplayName = "Guard Loop"),
    GuardExit UMETA(DisplayName = "Guard Exit"),
    Parry UMETA(DisplayName = "Parry"),
    Parried UMETA(DisplayName = "Parried"),
    HitLightFront UMETA(DisplayName = "Hit Light Front"),
    HitLightBack UMETA(DisplayName = "Hit Light Back"),
    HitHeavyFront UMETA(DisplayName = "Hit Heavy Front"),
    HitHeavyBack UMETA(DisplayName = "Hit Heavy Back"),
    GuardBreak UMETA(DisplayName = "Guard Break"),
    Equip UMETA(DisplayName = "Equip"),
    Unequip UMETA(DisplayName = "Unequip"),
    Count UMETA(Hidden)
};

USTRUCT(BlueprintType)
struct FRPGWeaponAnimationSet
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Animation")
    TObjectPtr<UAnimationAsset> Idle = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Animation")
    TObjectPtr<UAnimationAsset> Walk = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Animation")
    TObjectPtr<UAnimationAsset> Run = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Animation")
    TObjectPtr<UAnimationAsset> Sprint = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Animation")
    TObjectPtr<UAnimationAsset> FocusIdle = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Animation")
    TObjectPtr<UAnimationAsset> FocusMove = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Animation")
    TObjectPtr<UAnimationAsset> GuardIdle = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Animation")
    TObjectPtr<UAnimationAsset> GuardMove = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Animation")
    TObjectPtr<UAnimationAsset> RunStart = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Animation")
    TObjectPtr<UAnimationAsset> RunStop = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Animation")
    TObjectPtr<UAnimationAsset> JumpStart = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Animation")
    TObjectPtr<UAnimationAsset> JumpLoop = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Animation")
    TObjectPtr<UAnimationAsset> JumpLand = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Animation")
    TObjectPtr<UAnimationAsset> FallLoop = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Animation")
    TObjectPtr<UAnimationAsset> GuardEnter = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Animation")
    TObjectPtr<UAnimationAsset> GuardLoop = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Animation")
    TObjectPtr<UAnimationAsset> GuardExit = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Animation")
    TObjectPtr<UAnimationAsset> Parry = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Animation")
    TObjectPtr<UAnimationAsset> Parried = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Animation")
    TObjectPtr<UAnimationAsset> HitLightFront = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Animation")
    TObjectPtr<UAnimationAsset> HitLightBack = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Animation")
    TObjectPtr<UAnimationAsset> HitHeavyFront = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Animation")
    TObjectPtr<UAnimationAsset> HitHeavyBack = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Animation")
    TObjectPtr<UAnimationAsset> GuardBreak = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Animation")
    TObjectPtr<UAnimationAsset> Equip = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Animation")
    TObjectPtr<UAnimationAsset> Unequip = nullptr;

    UAnimationAsset* GetAnimationForSlot(ERPGAnimationSlot Slot) const;
    void SetAnimationForSlot(ERPGAnimationSlot Slot, UAnimationAsset* Animation);
};

USTRUCT(BlueprintType)
struct FRPGWeaponCombatTuning
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Tuning", meta=(ClampMin="0.01"))
    float AttackDamageMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Tuning", meta=(ClampMin="0.01"))
    float AttackStaminaMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Tuning", meta=(ClampMin="0.01"))
    float EvasionStaminaMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Tuning", meta=(ClampMin="0.01"))
    float WalkSpeedMultiplier = 1.0f;
};

USTRUCT(BlueprintType)
struct FRPGWeaponVisualDefinition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Visual")
    TObjectPtr<UStaticMesh> EquippedMesh = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Visual")
    FName EquippedSocketName = TEXT("hand_r");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Visual")
    FTransform EquippedRelativeTransform;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Visual")
    FName SheathedSocketName = TEXT("sheathe");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Visual")
    FTransform SheathedRelativeTransform;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Visual")
    TObjectPtr<UStaticMesh> PickupMesh = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Visual")
    FTransform PickupRelativeTransform;
};

UCLASS(BlueprintType)
class RPG_GAME_API URPGWeaponTypeDataAsset : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
    ERPGWeaponType WeaponType = ERPGWeaponType::Unarmed;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Attack")
    TObjectPtr<URPGCombatMovesetDataAsset> AttackMoveset = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Evasion")
    bool bUseDirectionalDodgeMontages = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Evasion")
    bool bOverrideDodgeProfile = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Evasion")
    FRPGEvasionDirectionalMontages DodgeDirectionalMontages;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Evasion")
    TObjectPtr<UAnimMontage> DodgeMontage = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Evasion")
    bool bUseDirectionalRollMontages = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Evasion")
    bool bOverrideRollProfile = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Evasion")
    FRPGEvasionDirectionalMontages RollDirectionalMontages;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Evasion")
    TObjectPtr<UAnimMontage> RollMontage = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Animation")
    FRPGWeaponAnimationSet AnimationOverrides;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Locomotion")
    bool bOverrideLocomotionData = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Locomotion")
    TObjectPtr<URPGLocomotionDataAsset> LocomotionData = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Tuning")
    FRPGWeaponCombatTuning BaseTuning;
};

UCLASS(BlueprintType)
class RPG_GAME_API URPGWeaponInstanceDataAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
    TObjectPtr<URPGWeaponTypeDataAsset> WeaponTypeProfile = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
    FName TierName = TEXT("Common");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Visual")
    FRPGWeaponVisualDefinition VisualDefinition;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Tuning", meta=(ClampMin="0.01"))
    float AttackDamageMultiplierBonus = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Tuning", meta=(ClampMin="0.01"))
    float AttackStaminaMultiplierBonus = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Tuning", meta=(ClampMin="0.01"))
    float EvasionStaminaMultiplierBonus = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Tuning", meta=(ClampMin="0.01"))
    float WalkSpeedMultiplierBonus = 1.0f;

    UFUNCTION(BlueprintPure, Category = "Weapon|Tuning")
    FRPGWeaponCombatTuning ResolveFinalTuning() const;

    UFUNCTION(BlueprintPure, Category = "Weapon|Visual")
    UStaticMesh* ResolveEquippedMesh() const;

    UFUNCTION(BlueprintPure, Category = "Weapon|Visual")
    UStaticMesh* ResolvePickupMesh() const;

    UFUNCTION(BlueprintPure, Category = "Weapon|Visual")
    FName ResolveEquippedSocketName() const;

    UFUNCTION(BlueprintPure, Category = "Weapon|Visual")
    FName ResolveSheathedSocketName() const;
};

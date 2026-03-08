#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Components/EvasionComponent.h"
#include "Data/RPGCombatMovesetDataAsset.h"
#include "RPGWeaponDataAssets.generated.h"

class UAnimationAsset;

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
    CrouchIdle UMETA(DisplayName = "Crouch Idle"),
    CrouchWalk UMETA(DisplayName = "Crouch Walk"),
    JumpStart UMETA(DisplayName = "Jump Start"),
    JumpLoop UMETA(DisplayName = "Jump Loop"),
    JumpLand UMETA(DisplayName = "Jump Land"),
    FallLoop UMETA(DisplayName = "Fall Loop")
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
    TObjectPtr<UAnimationAsset> CrouchIdle = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Animation")
    TObjectPtr<UAnimationAsset> CrouchWalk = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Animation")
    TObjectPtr<UAnimationAsset> JumpStart = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Animation")
    TObjectPtr<UAnimationAsset> JumpLoop = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Animation")
    TObjectPtr<UAnimationAsset> JumpLand = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Animation")
    TObjectPtr<UAnimationAsset> FallLoop = nullptr;

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
    FRPGEvasionDirectionalMontages DodgeDirectionalMontages;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Evasion")
    TObjectPtr<UAnimMontage> DodgeMontage = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Evasion")
    bool bUseDirectionalRollMontages = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Evasion")
    FRPGEvasionDirectionalMontages RollDirectionalMontages;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Evasion")
    TObjectPtr<UAnimMontage> RollMontage = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Animation")
    FRPGWeaponAnimationSet AnimationOverrides;

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
};

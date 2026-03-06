#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Components/EvasionComponent.h"
#include "Data/RPGCombatMovesetDataAsset.h"
#include "RPGWeaponDataAssets.generated.h"

UENUM(BlueprintType)
enum class ERPGWeaponType : uint8
{
    Sword UMETA(DisplayName = "Sword"),
    SwordShield UMETA(DisplayName = "Sword & Shield"),
    DualSword UMETA(DisplayName = "Dual Sword"),
    Greatsword UMETA(DisplayName = "Greatsword"),
    Battleaxe UMETA(DisplayName = "Battleaxe"),
    DualBattleaxe UMETA(DisplayName = "Dual Battleaxe")
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
};

UCLASS(BlueprintType)
class RPG_GAME_API URPGWeaponTypeDataAsset : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
    ERPGWeaponType WeaponType = ERPGWeaponType::Sword;

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

    UFUNCTION(BlueprintPure, Category = "Weapon|Tuning")
    FRPGWeaponCombatTuning ResolveFinalTuning() const;
};

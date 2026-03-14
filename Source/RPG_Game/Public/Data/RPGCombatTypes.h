#pragma once

#include "CoreMinimal.h"
#include "RPGCombatTypes.generated.h"

class AActor;
class AController;

UENUM(BlueprintType)
enum class ERPGHitReactionStrength : uint8
{
    None UMETA(DisplayName = "None"),
    Light UMETA(DisplayName = "Light"),
    Heavy UMETA(DisplayName = "Heavy"),
    GuardBreak UMETA(DisplayName = "Guard Break")
};

UENUM(BlueprintType)
enum class ERPGHitDirection : uint8
{
    Front UMETA(DisplayName = "Front"),
    Back UMETA(DisplayName = "Back"),
    Left UMETA(DisplayName = "Left"),
    Right UMETA(DisplayName = "Right")
};

USTRUCT(BlueprintType)
struct FRPGDamageSpec
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
    float Damage = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage", meta = (ClampMin = "0.0"))
    float HitstunDuration = 0.25f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
    ERPGHitReactionStrength ReactionStrength = ERPGHitReactionStrength::Light;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
    ERPGHitDirection HitDirection = ERPGHitDirection::Front;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage", meta = (ClampMin = "0.0"))
    float StaggerDamage = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
    bool bCanBeBlocked = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
    bool bCanBeParried = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
    TObjectPtr<AActor> DamageCauser = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
    TObjectPtr<AController> EventInstigator = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
    FVector HitLocation = FVector::ZeroVector;
};

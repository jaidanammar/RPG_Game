#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RPGLocomotionDataAsset.generated.h"

USTRUCT(BlueprintType)
struct FRPGLocomotionSpeedSet
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion|Speed", meta=(ClampMin="0.0"))
    float WalkSpeed = 220.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion|Speed", meta=(ClampMin="0.0"))
    float RunSpeed = 420.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion|Speed", meta=(ClampMin="0.0"))
    float SprintSpeed = 650.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion|Speed", meta=(ClampMin="0.0"))
    float CrouchSpeed = 180.0f;
};

UCLASS(BlueprintType)
class RPG_GAME_API URPGLocomotionDataAsset : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion")
    FRPGLocomotionSpeedSet Speeds;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion|Air", meta=(ClampMin="0.0"))
    float JumpZVelocity = 420.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion|Air", meta=(ClampMin="0.0", ClampMax="1.0"))
    float AirControl = 0.35f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion|Movement", meta=(ClampMin="0.0"))
    float MaxAcceleration = 2048.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion|Movement", meta=(ClampMin="0.0"))
    float BrakingDecelerationWalking = 2048.0f;
};


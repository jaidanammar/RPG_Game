#include "Components/PlayerStatsComponent.h"

#include "Math/UnrealMathUtility.h"

UPlayerStatsComponent::UPlayerStatsComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UPlayerStatsComponent::BeginPlay()
{
    Super::BeginPlay();

    ClampStats();
    BroadcastAllStats();
}

bool UPlayerStatsComponent::DecreaseHealth(float Damage)
{
    if (Damage <= 0.0f || IsDead() || bIsInvulnerable)
    {
        return IsDead();
    }

    CurrentHealth = FMath::Clamp(CurrentHealth - Damage, 0.0f, MaxHealth);
    OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
    OnDamaged.Broadcast(Damage, CurrentHealth, MaxHealth);

    if (IsDead())
    {
        OnDeath.Broadcast();
        return true;
    }

    return false;
}

void UPlayerStatsComponent::IncreaseHealth(float HealthRegeneration)
{
    if (HealthRegeneration <= 0.0f || IsDead())
    {
        return;
    }

    CurrentHealth = FMath::Clamp(CurrentHealth + HealthRegeneration, 0.0f, MaxHealth);
    OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
}

void UPlayerStatsComponent::IncreaseMaxHealth(float HealthIncrement)
{
    if (HealthIncrement <= 0.0f)
    {
        return;
    }

    MaxHealth = FMath::Max(1.0f, MaxHealth + HealthIncrement);
    CurrentHealth = FMath::Clamp(CurrentHealth, 0.0f, MaxHealth);
    OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
}

bool UPlayerStatsComponent::DecreaseStamina(float StaminaDepletion)
{
    if (StaminaDepletion <= 0.0f)
    {
        return CurrentStamina <= 0.0f;
    }

    CurrentStamina = FMath::Clamp(CurrentStamina - StaminaDepletion, 0.0f, MaxStamina);
    OnStaminaChanged.Broadcast(CurrentStamina, MaxStamina);
    return CurrentStamina <= 0.0f;
}

void UPlayerStatsComponent::IncreaseStamina(float StaminaRegeneration)
{
    if (StaminaRegeneration <= 0.0f || !bAllowStaminaRegen)
    {
        return;
    }

    CurrentStamina = FMath::Clamp(CurrentStamina + StaminaRegeneration, 0.0f, MaxStamina);
    OnStaminaChanged.Broadcast(CurrentStamina, MaxStamina);
}

void UPlayerStatsComponent::IncreaseMaxStamina(float StaminaIncrement)
{
    if (StaminaIncrement <= 0.0f)
    {
        return;
    }

    MaxStamina = FMath::Max(1.0f, MaxStamina + StaminaIncrement);
    CurrentStamina = FMath::Clamp(CurrentStamina, 0.0f, MaxStamina);
    OnStaminaChanged.Broadcast(CurrentStamina, MaxStamina);
}

void UPlayerStatsComponent::IncreaseXP(float AddedXP)
{
    if (AddedXP <= 0.0f)
    {
        return;
    }

    XP += AddedXP;

    const int32 MaxLevelUpsPerCall = 50;
    int32 LevelUps = 0;

    while (XP >= MaxXP && MaxXP > 0.0f && LevelUps < MaxLevelUpsPerCall)
    {
        XP -= MaxXP;
        IncreaseLevel(1);
        ++LevelUps;
    }

    OnXPChanged.Broadcast(XP, MaxXP, Level);
}

void UPlayerStatsComponent::IncreaseLevel(int32 AddedLevel)
{
    const int32 SafeAddedLevel = FMath::Max(1, AddedLevel);
    Level = FMath::Max(1, Level + SafeAddedLevel);

    MaxXP = FMath::Max(1.0f, MaxXP * FMath::Pow(LevelXPScale, static_cast<float>(SafeAddedLevel)));

    OnLevelChanged.Broadcast(Level);
    OnXPChanged.Broadcast(XP, MaxXP, Level);
}

void UPlayerStatsComponent::BroadcastAllStats()
{
    ClampStats();
    OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
    OnStaminaChanged.Broadcast(CurrentStamina, MaxStamina);
    OnXPChanged.Broadcast(XP, MaxXP, Level);
}

void UPlayerStatsComponent::ClampStats()
{
    MaxHealth = FMath::Max(1.0f, MaxHealth);
    MaxStamina = FMath::Max(1.0f, MaxStamina);
    MaxXP = FMath::Max(1.0f, MaxXP);
    Level = FMath::Max(1, Level);

    CurrentHealth = FMath::Clamp(CurrentHealth, 0.0f, MaxHealth);
    CurrentStamina = FMath::Clamp(CurrentStamina, 0.0f, MaxStamina);
    XP = FMath::Clamp(XP, 0.0f, MaxXP);
}




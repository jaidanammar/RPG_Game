#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/RPGCombatTypes.h"
#include "PlayerStatsComponent.generated.h"

class AActor;
class AController;
class UCombatStateComponent;
class UDamageType;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStatChanged, float, CurrentValue, float, MaxValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnXPChanged, float, XP, float, MaxXP, int32, Level);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLevelChanged, int32, NewLevel);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDeath);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnDamaged, float, Damage, float, NewHealth, float, MaxHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDamageNegated, AActor*, DamageCauser);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnHitReceived, FRPGDamageSpec, DamageSpec, float, DamageApplied, float, NewHealth, float, MaxHealth);

UCLASS(ClassGroup=(RPG), BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class RPG_GAME_API UPlayerStatsComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UPlayerStatsComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Health")
    float CurrentHealth = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Health")
    float MaxHealth = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Stamina")
    float CurrentStamina = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Stamina")
    float MaxStamina = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Stamina")
    bool bAllowStaminaRegen = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Stamina", meta=(ClampMin="0.0"))
    float StaminaRegenPerSecond = 20.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats|Combat")
    bool bIsInvulnerable = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Combat")
    bool bConsumeOwnerAnyDamageEvents = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|XP")
    float XP = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|XP")
    float MaxXP = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|XP", meta=(ClampMin="1"))
    int32 Level = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|XP", meta=(ClampMin="1.0"))
    float LevelXPScale = 1.5f;

    UPROPERTY(BlueprintAssignable, Category = "Stats|Events")
    FOnStatChanged OnHealthChanged;

    UPROPERTY(BlueprintAssignable, Category = "Stats|Events")
    FOnStatChanged OnStaminaChanged;

    UPROPERTY(BlueprintAssignable, Category = "Stats|Events")
    FOnXPChanged OnXPChanged;

    UPROPERTY(BlueprintAssignable, Category = "Stats|Events")
    FOnLevelChanged OnLevelChanged;

    UPROPERTY(BlueprintAssignable, Category = "Stats|Events")
    FOnDeath OnDeath;

    UPROPERTY(BlueprintAssignable, Category = "Stats|Events")
    FOnDamaged OnDamaged;

    UPROPERTY(BlueprintAssignable, Category = "Stats|Events")
    FOnDamageNegated OnDamageNegated;

    UPROPERTY(BlueprintAssignable, Category = "Stats|Events")
    FOnHitReceived OnHitReceived;

    UFUNCTION(BlueprintCallable, Category = "Stats|Health")
    bool DecreaseHealth(float Damage);

    UFUNCTION(BlueprintCallable, Category = "Stats|Health")
    bool ApplyIncomingDamage(float Damage, AActor* DamageCauser);

    UFUNCTION(BlueprintCallable, Category = "Stats|Health")
    bool ApplyIncomingHit(const FRPGDamageSpec& DamageSpec);

    UFUNCTION(BlueprintCallable, Category = "Stats|Health")
    void IncreaseHealth(float HealthRegeneration);

    UFUNCTION(BlueprintCallable, Category = "Stats|Health")
    void IncreaseMaxHealth(float HealthIncrement);

    UFUNCTION(BlueprintCallable, Category = "Stats|Stamina")
    bool DecreaseStamina(float StaminaDepletion);

    UFUNCTION(BlueprintCallable, Category = "Stats|Stamina")
    void IncreaseStamina(float StaminaRegeneration);

    UFUNCTION(BlueprintCallable, Category = "Stats|Stamina")
    void IncreaseMaxStamina(float StaminaIncrement);

    UFUNCTION(BlueprintCallable, Category = "Stats|XP")
    void IncreaseXP(float AddedXP);

    UFUNCTION(BlueprintCallable, Category = "Stats|XP")
    void IncreaseLevel(int32 AddedLevel = 1);

    UFUNCTION(BlueprintCallable, Category = "Stats")
    void BroadcastAllStats();

    UFUNCTION(BlueprintPure, Category = "Stats")
    bool IsDead() const { return CurrentHealth <= 0.0f; }

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    void ClampStats();

    UFUNCTION()
    void HandleOwnerAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser);
};





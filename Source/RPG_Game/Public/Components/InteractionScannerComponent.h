#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InteractionScannerComponent.generated.h"

class AActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCurrentInteractableChanged, AActor*, NewInteractable);

UCLASS(ClassGroup=(RPG), BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class RPG_GAME_API UInteractionScannerComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UInteractionScannerComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction", meta=(ClampMin="0.0"))
    float TraceDistance = 225.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction", meta=(ClampMin="0.0"))
    float TraceRadius = 40.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
    TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
    bool bRefreshContinuously = true;

    UPROPERTY(BlueprintAssignable, Category = "Interaction")
    FOnCurrentInteractableChanged OnCurrentInteractableChanged;

    UFUNCTION(BlueprintCallable, Category = "Interaction")
    bool TryInteract();

    UFUNCTION(BlueprintCallable, Category = "Interaction")
    void RefreshCurrentInteractable();

    UFUNCTION(BlueprintPure, Category = "Interaction")
    AActor* FindBestInteractable() const;

    UFUNCTION(BlueprintPure, Category = "Interaction")
    AActor* GetCurrentInteractable() const { return CurrentInteractable.Get(); }

    UFUNCTION(BlueprintPure, Category = "Interaction")
    FText GetCurrentInteractionLabel() const;

protected:
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    UPROPERTY(Transient)
    TObjectPtr<AActor> CurrentInteractable = nullptr;

    void UpdateCurrentInteractable(AActor* NewInteractable);
};

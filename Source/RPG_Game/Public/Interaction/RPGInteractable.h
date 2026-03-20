#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "RPGInteractable.generated.h"

UINTERFACE(BlueprintType)
class RPG_GAME_API URPGInteractable : public UInterface
{
    GENERATED_BODY()
};

class RPG_GAME_API IRPGInteractable
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
    bool CanInteract(AActor* Interactor);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
    void Interact(AActor* Interactor);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
    FText GetInteractionLabel();
};

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CBKeyPickup.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class UCBInteractionComponent;
class APlayerController;

/** A single demo key. The character keeps possession after the pickup is removed. */
UCLASS(Blueprintable)
class CREEPYBROSKY_API ACBKeyPickup : public AActor
{
    GENERATED_BODY()
public:
    ACBKeyPickup();
    void SetInteractionFocus(bool bFocused, APlayerController* Viewer);
    void Collect(UCBInteractionComponent* PlayerInteraction);

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Key")
    TObjectPtr<UBoxComponent> PickupBounds;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Key")
    TObjectPtr<UStaticMeshComponent> KeyHead;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Key")
    TObjectPtr<UStaticMeshComponent> KeyShaft;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Key")
    TObjectPtr<UStaticMeshComponent> KeyTeeth;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Key")
    TObjectPtr<UTextRenderComponent> InteractionPrompt;

private:
    bool bCollected = false;
};

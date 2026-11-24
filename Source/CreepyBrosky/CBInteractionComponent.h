#pragma once

#include "CoreMinimal.h"
#include "CBDemoGameMode.h"
#include "Components/ActorComponent.h"
#include "CBInteractionComponent.generated.h"

class ACBInteractableDoor;
class APlayerController;
class UEnhancedInputComponent;
class UInputAction;

/** Player-owned targeting and the small amount of state needed for this single-key demo. */
UCLASS(ClassGroup=(CreepyBrosky), meta=(BlueprintSpawnableComponent))
class CREEPYBROSKY_API UCBInteractionComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UCBInteractionComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction", meta=(ClampMin="1.0", Units="cm"))
    float MaxInteractionDistance = 250.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Interaction")
    TObjectPtr<UInputAction> InteractionAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Demo")
    TObjectPtr<UInputAction> RestartAction;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction")
    bool bHasKey = false;

    UFUNCTION(BlueprintPure, Category="Interaction")
    AActor* GetFocusedActor() const { return FocusedActor.Get(); }

    UFUNCTION(BlueprintPure, Category="Interaction")
    ACBInteractableDoor* GetFocusedDoor() const;

    UFUNCTION(BlueprintCallable, Category="Demo")
    void CompleteDemo();

    /** Practice maps without demo rules remain usable. Terminal runs do not accept interaction. */
    UFUNCTION(BlueprintPure, Category="Interaction")
    bool IsInteractionAllowed() const;

    /** Compatibility query; the GameMode owns the actual outcome. */
    UFUNCTION(BlueprintPure, Category="Demo")
    bool IsDemoCompleted() const;

    UFUNCTION(BlueprintCallable, Category="Demo")
    void NotifyPlayer(const FText& Message);

    UFUNCTION(BlueprintPure, Category="Demo")
    FText GetPlayerMessage() const;

    /** The first blocking hit wins; intervening walls block both doors and pickups. */
    static AActor* TraceInteractableInView(UWorld* World, const FVector& Origin,
        const FVector& Direction, float Distance, AActor* IgnoredActor);

    // Retained for existing callers that specifically ask for a door.
    static ACBInteractableDoor* TraceDoorInView(UWorld* World, const FVector& Origin,
        const FVector& Direction, float Distance, AActor* IgnoredActor);

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    void SetFocus(AActor* Actor, APlayerController* Controller);
    void ClearFocus();
    void RemoveInputBinding();
    void HandleInteract();
    void HandleRestart();
    APlayerController* GetLocalController() const;
    void RefreshRunStateBinding();

    UFUNCTION()
    void HandleRunStateChanged(ECBRunState NewState);

    TWeakObjectPtr<ACBDemoGameMode> BoundGameMode;
    TWeakObjectPtr<AActor> FocusedActor;
    TWeakObjectPtr<APlayerController> ViewingController;
    TWeakObjectPtr<UEnhancedInputComponent> BoundInput;
    TWeakObjectPtr<UInputAction> BoundAction;
    TWeakObjectPtr<UInputAction> BoundRestartAction;
    uint32 InteractionBindingHandle = 0;
    uint32 RestartBindingHandle = 0;
    FText PlayerMessage;
    double MessageExpiresAt = 0.0;
};

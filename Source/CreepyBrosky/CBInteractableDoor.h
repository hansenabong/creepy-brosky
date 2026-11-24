#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CBInteractableDoor.generated.h"

class UStaticMeshComponent;
class USceneComponent;
class UTextRenderComponent;
class APlayerController;
class UCBInteractionComponent;

/** Owns one door's geometry and motion; the player's interaction component supplies requests. */
UCLASS(Blueprintable)
class CREEPYBROSKY_API ACBInteractableDoor : public AActor
{
    GENERATED_BODY()

public:
    ACBInteractableDoor();
    virtual void Tick(float DeltaSeconds) override;

    UFUNCTION(BlueprintCallable, Category="Door")
    void ToggleDoor();

    UFUNCTION(BlueprintCallable, Category="Door")
    void SetDoorOpen(bool bOpen);

    UFUNCTION(BlueprintPure, Category="Door")
    bool IsDoorOpen() const { return bTargetOpen; }

    UFUNCTION(BlueprintPure, Category="Door")
    float GetOpenFraction() const { return OpenFraction; }

    void SetInteractionFocus(bool bFocused, APlayerController* Viewer);

    /** The door owns the decision to unlock; the player owns key possession. */
    void TryInteract(UCBInteractionComponent* PlayerInteraction, APlayerController* Viewer);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Door", meta=(ClampMin="0.1", ClampMax="5.0", Units="s"))
    float OpeningDuration = 0.8f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Door", meta=(ClampMin="30.0", ClampMax="120.0", Units="deg"))
    float OpenAngle = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Door")
    bool bOpenAwayFromPlayer = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door")
    bool bLocked = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Door|Components")
    TObjectPtr<USceneComponent> DoorRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Door|Components")
    TObjectPtr<USceneComponent> Hinge;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Door|Components")
    TObjectPtr<UStaticMeshComponent> DoorPanel;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Door|Components")
    TObjectPtr<UStaticMeshComponent> FrameLeft;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Door|Components")
    TObjectPtr<UStaticMeshComponent> FrameRight;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Door|Components")
    TObjectPtr<UStaticMeshComponent> FrameTop;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Door|Components")
    TObjectPtr<UStaticMeshComponent> Handle;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Door|Components")
    TObjectPtr<UTextRenderComponent> InteractionPrompt;

protected:
    virtual void BeginPlay() override;

private:
    // Target intent and current progress are separate, so a second press can reverse a moving door.
    bool bTargetOpen = false;
    float OpenFraction = 0.0f;
    float ActiveOpenAngle = 100.0f;
    TWeakObjectPtr<APlayerController> FocusedViewer;
    bool WouldTouchPawn(float ProposedYaw) const;
    void RefreshPrompt();
};

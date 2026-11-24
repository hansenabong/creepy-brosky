#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CBExitZone.generated.h"
class UBoxComponent;
class ACBInteractableDoor;

/** Place beyond the exit. The gate reference makes completion an explicit level rule. */
UCLASS(Blueprintable)
class CREEPYBROSKY_API ACBExitZone : public AActor
{
    GENERATED_BODY()
public:
    ACBExitZone();
    virtual void Tick(float DeltaSeconds) override;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Exit")
    TObjectPtr<UBoxComponent> ExitVolume;
    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Exit")
    TObjectPtr<ACBInteractableDoor> ExitDoor;

private:
    bool bWarnedMissingGameMode = false;
};

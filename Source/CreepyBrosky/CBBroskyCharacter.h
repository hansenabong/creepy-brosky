#pragma once

#include "CoreMinimal.h"
#include "CBDemoGameMode.h"
#include "GameFramework/Character.h"
#include "CBBroskyCharacter.generated.h"

class UStaticMesh;
class UStaticMeshComponent;
class ATargetPoint;

/** Navigation body and capture execution. The AIController owns destinations and sight. */
UCLASS(Blueprintable)
class CREEPYBROSKY_API ACBBroskyCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    ACBBroskyCharacter();
    virtual void Tick(float DeltaSeconds) override;
    virtual void OnConstruction(const FTransform& Transform) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Brosky|Appearance")
    TObjectPtr<UStaticMeshComponent> CreatureMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Brosky|Appearance")
    TObjectPtr<UStaticMesh> AppearanceMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Brosky|Appearance", meta=(ClampMin="0.01"))
    float VisualScale = 0.8f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Brosky|Appearance", meta=(Units="deg"))
    float VisualYawOffset = 0.f;

    /** Mesh origin measured from the bottom of the collision capsule, before bobbing. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Brosky|Appearance", meta=(Units="cm"))
    float VisualHeightOffset = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Brosky|Appearance", meta=(ClampMin="0", ClampMax="25", Units="cm"))
    float HoverAmplitude = 8.f;

    // Authoring data lives on the placed character; the controller owns changing AI state.
    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Brosky|Patrol")
    TArray<TObjectPtr<ATargetPoint>> PatrolPoints;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Brosky|AI")
    bool bEnablePursuit = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Brosky|Patrol", meta=(ClampMin="0", Units="cm/s"))
    float PatrolSpeed = 70.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Brosky|Chase", meta=(ClampMin="0", Units="cm/s"))
    float ChaseSpeed = 110.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Brosky|Patrol", meta=(ClampMin="0", Units="s"))
    float PatrolWaitDuration = 1.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Brosky|Patrol", meta=(ClampMin="1", Units="cm"))
    float PatrolAcceptanceRadius = 35.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Brosky|Sight", meta=(ClampMin="1", Units="cm"))
    float SightRange = 450.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Brosky|Sight", meta=(ClampMin="0", ClampMax="180", Units="deg"))
    float SightHalfAngle = 65.f;

    /** Close peripheral awareness still requires an unobstructed body-height sight trace. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Brosky|Sight", meta=(ClampMin="0", Units="cm"))
    float NearAwarenessRadius = 180.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Brosky|Sight", meta=(ClampMin="0", Units="cm"))
    float MaxSightHeightDifference = 180.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Brosky|Search", meta=(ClampMin="0.1", Units="s"))
    float SearchDuration = 6.f;

    /** Actor-center distance, deliberately below CaptureRange to allow a capture attempt. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Brosky|Chase", meta=(ClampMin="1", Units="cm"))
    float ChaseStopDistance = 145.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Brosky|Navigation", meta=(ClampMin="0.1", Units="s"))
    float RepathInterval = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Brosky|Navigation", meta=(ClampMin="0.1", Units="s"))
    float PathRetryDelay = 1.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Brosky|Debug")
    bool bShowAIDebug = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Brosky|Capture")
    bool bCanCapturePlayer = true;

    /** Horizontal actor-center distance; the controller must stop inside this reach. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Brosky|Capture", meta=(ClampMin="1", Units="cm"))
    float CaptureRange = 180.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Brosky|Capture", meta=(ClampMin="0.1", Units="s"))
    float CaptureWindupDuration = 1.2f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Brosky|Capture", meta=(ClampMin="0", Units="cm"))
    float MaxCaptureHeightDifference = 130.f;

    UFUNCTION(BlueprintPure, Category="Brosky|Capture")
    float GetCaptureProgress() const;

    UFUNCTION(BlueprintPure, Category="Brosky|Capture")
    bool IsCapturePending() const { return CaptureTarget.IsValid(); }

    /** Called once per controller tick with its currently visible player, or nullptr to cancel. */
    void UpdateCapture(APawn* VisiblePlayer, float DeltaSeconds);
    void CancelCapture();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    bool IsCaptureEligible(APawn* Player) const;
    void ApplyAppearance();

    UFUNCTION()
    void HandleRunStateChanged(ECBRunState NewState);

    TWeakObjectPtr<ACBDemoGameMode> RunMode;
    TWeakObjectPtr<APawn> CaptureTarget;
    float CaptureElapsed = 0.f;
    float TimeAlive = 0.f;
    bool bWarnedMissingRunMode = false;
};

#pragma once
#include "CoreMinimal.h"
#include "CBDemoGameMode.h"
#include "GameFramework/Actor.h"
#include "CBSpectralCreature.generated.h"
class UCapsuleComponent;
class UStaticMeshComponent;
class UStaticMesh;
class APawn;

UENUM(BlueprintType)
enum class ECGhostMotion : uint8 { Drifting, Approaching, Watching };

/** Floating movement prototype with optional capture; navigation comes later. */
UCLASS(Blueprintable)
class CREEPYBROSKY_API ACBSpectralCreature : public AActor
{
    GENERATED_BODY()
public:
    ACBSpectralCreature();
    virtual void Tick(float DeltaSeconds) override;
    virtual void OnConstruction(const FTransform& Transform) override;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ghost")
    TObjectPtr<UCapsuleComponent> Collision;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ghost")
    TObjectPtr<UStaticMeshComponent> CreatureMesh;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ghost|Appearance")
    TObjectPtr<UStaticMesh> AppearanceMesh;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ghost|Appearance",meta=(ClampMin="0.01"))
    float VisualScale=1.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ghost|Movement", meta=(ClampMin="0",Units="cm/s"))
    float MoveSpeed=60.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ghost|Movement", meta=(Units="cm"))
    FVector PatrolOffset=FVector(0,220,0);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ghost|Movement")
    bool bApproachPlayer=true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ghost|Movement", meta=(ClampMin="100",Units="cm"))
    float DetectionRange=650.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ghost|Movement", meta=(ClampMin="110",Units="cm"))
    float StopDistance=170.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ghost|Appearance", meta=(ClampMin="0",ClampMax="25",Units="cm"))
    float HoverAmplitude=8.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ghost|Appearance", meta=(Units="deg"))
    float VisualYawOffset=0.f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ghost")
    ECGhostMotion MotionState=ECGhostMotion::Drifting;

    /** Disabled by default so the original appearance/movement lab stays safe. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ghost|Capture")
    bool bCanCapturePlayer=false;

    /** Horizontal actor-center distance. Must reach beyond the movement stopping distance. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ghost|Capture", meta=(ClampMin="1", Units="cm"))
    float CaptureRange=180.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ghost|Capture", meta=(ClampMin="0.1", Units="s"))
    float CaptureWindupDuration=1.2f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ghost|Capture", meta=(ClampMin="0", Units="cm"))
    float MaxCaptureHeightDifference=130.f;

    UFUNCTION(BlueprintPure, Category="Ghost|Capture")
    float GetCaptureProgress() const;

    UFUNCTION(BlueprintPure, Category="Ghost|Capture")
    bool IsCapturePending() const { return CaptureTarget.IsValid(); }

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
private:
    bool IsCaptureEligible(APawn* Player) const;
    void UpdateCapture(APawn* Player, float DeltaSeconds);
    void CancelCapture();

    UFUNCTION()
    void HandleRunStateChanged(ECBRunState NewState);

    TWeakObjectPtr<ACBDemoGameMode> RunMode;
    TWeakObjectPtr<APawn> CaptureTarget;
    float CaptureElapsed=0.f;
    bool bWarnedMissingRunMode=false;
    FVector PatrolStart,PatrolEnd,MeshRestLocation;
    bool bGoingToEnd=true;
    float TimeAlive=0.f;
    double NextTurnTime=0;
};

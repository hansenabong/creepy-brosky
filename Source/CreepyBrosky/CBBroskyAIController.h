#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "CBDemoGameMode.h"
#include "CBBroskyAIController.generated.h"

class ACBBroskyCharacter;

UENUM(BlueprintType)
enum class ECBBroskyState : uint8 { Inactive, Patrol, Chase, Search };

/** Chooses destinations; CharacterMovement and path following move the body. */
UCLASS()
class CREEPYBROSKY_API ACBBroskyAIController : public AAIController
{
    GENERATED_BODY()
public:
    ACBBroskyAIController();
    virtual void Tick(float DeltaSeconds) override;

    UFUNCTION(BlueprintPure, Category="Brosky|AI")
    ECBBroskyState GetBehaviorState() const { return BehaviorState; }

    UFUNCTION(BlueprintPure, Category="Brosky|AI")
    FString GetDebugSummary() const;

protected:
    virtual void OnPossess(APawn* InPawn) override;
    virtual void OnUnPossess() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result) override;

private:
    bool CanSeePlayer(const APawn* Player) const;
    void UpdateSight(double Now);
    void SetBehaviorState(ECBBroskyState NewState);
    void UpdateDestination(double Now);
    void RequestDestination(const FVector& Destination, float AcceptanceRadius, double Now);
    void HandleArrival(double Now);
    void HandlePathFailure(double Now);
    void StopCurrentPath();
    void DetachRunOwner();

    UFUNCTION()
    void HandleRunStateChanged(ECBRunState NewState);

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Brosky|Debug", meta=(AllowPrivateAccess="true"))
    ECBBroskyState BehaviorState=ECBBroskyState::Inactive;

    /** Updated only after a successful sight check, never by a hidden actor goal. */
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Brosky|Debug", meta=(AllowPrivateAccess="true"))
    FVector LastSeenLocation=FVector::ZeroVector;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Brosky|Debug", meta=(AllowPrivateAccess="true"))
    bool bHasLastSeenLocation=false;

    TWeakObjectPtr<ACBBroskyCharacter> Brosky;
    TWeakObjectPtr<ACBDemoGameMode> RunMode;
    TWeakObjectPtr<APawn> VisiblePlayer;
    FAIRequestID ActiveMoveRequest=FAIRequestID::InvalidRequest;
    FVector LastRequestedLocation=FVector::ZeroVector;
    FString PathStatus=TEXT("Inactive");
    int32 PatrolIndex=0;
    double NextSenseTime=0;
    double NextMoveTime=0;
    double SearchEndTime=0;
    double SearchStartTime=0;
    float SearchFacingYaw=0;
    bool bReachedSearchLocation=false;
    bool bPathBlocked=false;
    bool bWarnedNoPatrol=false;
};

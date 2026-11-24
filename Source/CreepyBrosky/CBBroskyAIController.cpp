#include "CBBroskyAIController.h"
#include "CBBroskyCharacter.h"
#include "Engine/TargetPoint.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogCBBroskyAI, Log, All);

ACBBroskyAIController::ACBBroskyAIController()
{
    PrimaryActorTick.bCanEverTick=true;
    bAllowStrafe=true;
}

void ACBBroskyAIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);
    Brosky=Cast<ACBBroskyCharacter>(InPawn);
    RunMode=GetWorld()->GetAuthGameMode<ACBDemoGameMode>();
    if (!Brosky.IsValid() || !RunMode.IsValid())
    {
        UE_LOG(LogCBBroskyAI, Warning, TEXT("Brosky AI needs CBBroskyCharacter and CBDemoGameMode; staying inactive."));
        return;
    }
    RunMode->OnRunStateChanged.AddDynamic(this, &ACBBroskyAIController::HandleRunStateChanged);
    PatrolIndex=0;
    NextSenseTime=GetWorld()->GetTimeSeconds()+0.25;
    SetBehaviorState(RunMode->IsRunActive() ? ECBBroskyState::Patrol : ECBBroskyState::Inactive);
}

void ACBBroskyAIController::DetachRunOwner()
{
    if (RunMode.IsValid())
        RunMode->OnRunStateChanged.RemoveDynamic(this, &ACBBroskyAIController::HandleRunStateChanged);
    RunMode.Reset();
}

void ACBBroskyAIController::OnUnPossess()
{
    SetBehaviorState(ECBBroskyState::Inactive);
    DetachRunOwner();
    Brosky.Reset();
    Super::OnUnPossess();
}

void ACBBroskyAIController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    DetachRunOwner();
    Super::EndPlay(EndPlayReason);
}

void ACBBroskyAIController::HandleRunStateChanged(ECBRunState NewState)
{
    if (NewState != ECBRunState::Playing) SetBehaviorState(ECBBroskyState::Inactive);
}

void ACBBroskyAIController::StopCurrentPath()
{
    // StopMovement can synchronously deliver an aborted callback for the previous path.
    // Invalidate its ID first so that intentional state changes aren't counted as failures.
    ActiveMoveRequest=FAIRequestID::InvalidRequest;
    StopMovement();
}

void ACBBroskyAIController::SetBehaviorState(ECBBroskyState NewState)
{
    if (BehaviorState == NewState) return;
    BehaviorState=NewState;
    StopCurrentPath();
    ClearFocus(EAIFocusPriority::Gameplay);
    if (Brosky.IsValid())
    {
        Brosky->CancelCapture();
        UCharacterMovementComponent* Movement=Brosky->GetCharacterMovement();
        Movement->MaxWalkSpeed=NewState == ECBBroskyState::Chase ? Brosky->ChaseSpeed : Brosky->PatrolSpeed;
        Movement->bOrientRotationToMovement=NewState == ECBBroskyState::Patrol;
        Movement->bUseControllerDesiredRotation=NewState == ECBBroskyState::Chase || NewState == ECBBroskyState::Search;
    }
    const double Now=GetWorld()->GetTimeSeconds();
    NextMoveTime=Now;
    bPathBlocked=false;
    bReachedSearchLocation=false;
    if (NewState != ECBBroskyState::Chase) VisiblePlayer.Reset();
    if (NewState == ECBBroskyState::Search)
    {
        SearchStartTime=Now;
        SearchEndTime=Now+(Brosky.IsValid() ? FMath::Max(Brosky->SearchDuration, 0.1f) : 0.1f);
        SearchFacingYaw=GetPawn() ? GetPawn()->GetActorRotation().Yaw : 0.f;
        UE_LOG(LogCBBroskyAI, Log, TEXT("Search destination is last observed position %s"), *LastSeenLocation.ToCompactString());
    }
    if (NewState == ECBBroskyState::Patrol || NewState == ECBBroskyState::Inactive)
        bHasLastSeenLocation=false;
    PathStatus=NewState == ECBBroskyState::Inactive ? TEXT("Stopped") : TEXT("Choosing destination");
    UE_LOG(LogCBBroskyAI, Log, TEXT("%s: state %s"), *GetName(), *UEnum::GetValueAsString(NewState));
}

bool ACBBroskyAIController::CanSeePlayer(const APawn* Player) const
{
    if (!Brosky.IsValid() || !Brosky->bEnablePursuit || !IsValid(Player) || !Player->IsLocallyControlled()) return false;
    const FVector Delta=Player->GetActorLocation()-Brosky->GetActorLocation();
    const float Distance=Delta.Size2D();
    if (Distance > FMath::Max(Brosky->SightRange, 0.f)
        || FMath::Abs(Delta.Z) > FMath::Max(Brosky->MaxSightHeightDifference, 0.f)) return false;
    if (Distance > FMath::Max(Brosky->NearAwarenessRadius, 0.f))
    {
        const float FacingDot=FVector::DotProduct(Brosky->GetActorForwardVector().GetSafeNormal2D(), Delta.GetSafeNormal2D());
        const float ConeDot=FMath::Cos(FMath::DegreesToRadians(FMath::Clamp(Brosky->SightHalfAngle, 0.f, 180.f)));
        if (FacingDot < ConeDot) return false;
    }
    // Body-height sight, matching capture's cover rule. Near awareness never ignores walls.
    FHitResult Hit;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(CBBroskySight), false, Brosky.Get());
    const bool bHit=GetWorld()->LineTraceSingleByChannel(Hit, Brosky->GetActorLocation(), Player->GetActorLocation(), ECC_Visibility, Query);
    return !bHit || Hit.GetActor() == Player;
}

void ACBBroskyAIController::UpdateSight(double Now)
{
    NextSenseTime=Now+0.1;
    APawn* Player=UGameplayStatics::GetPlayerPawn(this, 0);
    if (CanSeePlayer(Player))
    {
        LastSeenLocation=Player->GetActorLocation();
        bHasLastSeenLocation=true;
        SetBehaviorState(ECBBroskyState::Chase);
        VisiblePlayer=Player;
        SetFocalPoint(LastSeenLocation);
    }
    else if (BehaviorState == ECBBroskyState::Chase)
    {
        // No player transform is read here: the search uses the previous successful observation.
        SetBehaviorState(bHasLastSeenLocation ? ECBBroskyState::Search : ECBBroskyState::Patrol);
    }
}

void ACBBroskyAIController::RequestDestination(const FVector& Destination, float AcceptanceRadius, double Now)
{
    if (!Brosky.IsValid()) return;
    UNavigationSystemV1* Navigation=FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
    FNavLocation Projected;
    const FNavAgentProperties& Agent=Brosky->GetCharacterMovement()->GetNavAgentPropertiesRef();
    if (!Navigation || !Navigation->ProjectPointToNavigation(Destination, Projected, FVector(60,60,180), &Agent))
    {
        StopCurrentPath();
        HandlePathFailure(Now);
        return;
    }
    FAIMoveRequest Request(Projected.Location);
    Request.SetUsePathfinding(true);
    Request.SetAllowPartialPath(false);
    Request.SetProjectGoalLocation(false);
    Request.SetAcceptanceRadius(FMath::Max(AcceptanceRadius, 1.f));
    // Keep the acceptance distance explicit: do not silently add the capsule radius.
    Request.SetReachTestIncludesAgentRadius(false);
    Request.SetReachTestIncludesGoalRadius(false);
    Request.SetCanStrafe(BehaviorState != ECBBroskyState::Patrol);
    ActiveMoveRequest=FAIRequestID::InvalidRequest;
    const FPathFollowingRequestResult Result=MoveTo(Request);
    LastRequestedLocation=Destination;
    NextMoveTime=Now+FMath::Max(Brosky->RepathInterval, 0.1f);
    if (Result.Code == EPathFollowingRequestResult::Failed)
    {
        HandlePathFailure(Now);
        return;
    }
    bPathBlocked=false;
    if (Result.Code == EPathFollowingRequestResult::AlreadyAtGoal)
        HandleArrival(Now);
    else
    {
        ActiveMoveRequest=Result.MoveId;
        PathStatus=TEXT("Following path");
    }
}

void ACBBroskyAIController::HandleArrival(double Now)
{
    ActiveMoveRequest=FAIRequestID::InvalidRequest;
    PathStatus=TEXT("At destination");
    if (!Brosky.IsValid()) return;
    if (BehaviorState == ECBBroskyState::Patrol)
    {
        PatrolIndex=(PatrolIndex+1)%FMath::Max(Brosky->PatrolPoints.Num(), 1);
        NextMoveTime=Now+FMath::Max(Brosky->PatrolWaitDuration, 0.1f);
    }
    else if (BehaviorState == ECBBroskyState::Search)
        bReachedSearchLocation=true;
}

void ACBBroskyAIController::HandlePathFailure(double Now)
{
    ActiveMoveRequest=FAIRequestID::InvalidRequest;
    bPathBlocked=true;
    PathStatus=TEXT("Path blocked / retry waiting");
    if (!Brosky.IsValid()) return;
    Brosky->CancelCapture();
    NextMoveTime=Now+FMath::Max(Brosky->PathRetryDelay, 0.25f);
    if (BehaviorState == ECBBroskyState::Patrol)
        PatrolIndex=(PatrolIndex+1)%FMath::Max(Brosky->PatrolPoints.Num(), 1);
    UE_LOG(LogCBBroskyAI, Log, TEXT("%s: path unavailable or blocked; retry after %.2f seconds"),
        *GetName(), FMath::Max(Brosky->PathRetryDelay, 0.25f));
}

void ACBBroskyAIController::OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
    Super::OnMoveCompleted(RequestID, Result);
    if (!ActiveMoveRequest.IsValid() || RequestID != ActiveMoveRequest || BehaviorState == ECBBroskyState::Inactive) return;
    if (Result.IsSuccess()) HandleArrival(GetWorld()->GetTimeSeconds());
    else HandlePathFailure(GetWorld()->GetTimeSeconds());
}

void ACBBroskyAIController::UpdateDestination(double Now)
{
    if (!Brosky.IsValid() || Now < NextMoveTime) return;
    if (BehaviorState == ECBBroskyState::Chase)
    {
        const float StopDistance=FMath::Max(1.f, FMath::Min(Brosky->ChaseStopDistance, Brosky->CaptureRange-10.f));
        if (FVector::Dist2D(Brosky->GetActorLocation(), LastSeenLocation) <= StopDistance)
        {
            StopCurrentPath();
            bPathBlocked=false;
            PathStatus=TEXT("Within approach distance");
            NextMoveTime=Now+0.1;
        }
        else if (!ActiveMoveRequest.IsValid() || FVector::DistSquared2D(LastRequestedLocation, LastSeenLocation) >= FMath::Square(30.f))
            RequestDestination(LastSeenLocation, StopDistance, Now);
        else
            NextMoveTime=Now+FMath::Max(Brosky->RepathInterval, 0.1f);
        return;
    }
    if (ActiveMoveRequest.IsValid()) return;
    if (BehaviorState == ECBBroskyState::Search)
    {
        if (!bReachedSearchLocation) RequestDestination(LastSeenLocation, Brosky->PatrolAcceptanceRadius, Now);
        return;
    }
    if (BehaviorState != ECBBroskyState::Patrol) return;
    const int32 Count=Brosky->PatrolPoints.Num();
    for (int32 Attempt=0; Attempt<Count; ++Attempt)
    {
        PatrolIndex=PatrolIndex%Count;
        if (IsValid(Brosky->PatrolPoints[PatrolIndex]))
        {
            RequestDestination(Brosky->PatrolPoints[PatrolIndex]->GetActorLocation(), Brosky->PatrolAcceptanceRadius, Now);
            return;
        }
        ++PatrolIndex;
    }
    PathStatus=TEXT("No valid patrol points");
    NextMoveTime=Now+1.0;
    if (!bWarnedNoPatrol)
    {
        UE_LOG(LogCBBroskyAI, Warning, TEXT("%s: assign Patrol Points on the placed BP_Brosky."), *GetName());
        bWarnedNoPatrol=true;
    }
}

void ACBBroskyAIController::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!Brosky.IsValid()) return;
    if (!RunMode.IsValid() || !RunMode->IsRunActive())
    {
        SetBehaviorState(ECBBroskyState::Inactive);
        return;
    }
    if (BehaviorState == ECBBroskyState::Inactive) return;
    const double Now=GetWorld()->GetTimeSeconds();
    if (Now >= NextSenseTime) UpdateSight(Now);
    if (BehaviorState == ECBBroskyState::Search)
    {
        if (Now >= SearchEndTime) SetBehaviorState(ECBBroskyState::Patrol);
        else if (bReachedSearchLocation)
        {
            const float Yaw=SearchFacingYaw+FMath::Sin(float(Now-SearchStartTime)*1.4f)*70.f;
            SetFocalPoint(Brosky->GetActorLocation()+FRotator(0,Yaw,0).Vector()*200.f);
        }
        else SetFocalPoint(LastSeenLocation);
    }
    UpdateDestination(Now);
    // Capture receives only a currently observed target and never the hidden search target.
    Brosky->UpdateCapture(BehaviorState == ECBBroskyState::Chase && !bPathBlocked ? VisiblePlayer.Get() : nullptr, DeltaSeconds);
}

FString ACBBroskyAIController::GetDebugSummary() const
{
    return FString::Printf(TEXT("%s | %s | Patrol point %d"),
        *UEnum::GetValueAsString(BehaviorState), *PathStatus, PatrolIndex+1);
}

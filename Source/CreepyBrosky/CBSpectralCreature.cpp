#include "CBSpectralCreature.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogCBTwin, Log, All);

ACBSpectralCreature::ACBSpectralCreature()
{
    PrimaryActorTick.bCanEverTick=true;
    Collision=CreateDefaultSubobject<UCapsuleComponent>(TEXT("Collision"));
    SetRootComponent(Collision);
    Collision->InitCapsuleSize(40.f,135.f);
    Collision->SetCollisionProfileName(TEXT("Pawn"));
    Collision->SetGenerateOverlapEvents(false);
    CreatureMesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CreatureMesh"));
    CreatureMesh->SetupAttachment(Collision);
    CreatureMesh->SetRelativeLocation(FVector(0,0,-135));
    CreatureMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ACBSpectralCreature::BeginPlay()
{
    Super::BeginPlay();
    PatrolStart=GetActorLocation();
    PatrolEnd=PatrolStart+GetActorQuat().RotateVector(PatrolOffset);
    PatrolEnd.Z=PatrolStart.Z;
    MeshRestLocation=CreatureMesh->GetRelativeLocation();

    RunMode=GetWorld()->GetAuthGameMode<ACBDemoGameMode>();
    if (RunMode.IsValid())
    {
        RunMode->OnRunStateChanged.AddDynamic(this, &ACBSpectralCreature::HandleRunStateChanged);
        HandleRunStateChanged(RunMode->GetRunState());
    }
    if (bCanCapturePlayer && CaptureRange <= StopDistance + 1.f)
    {
        UE_LOG(LogCBTwin, Warning, TEXT("%s: Capture Range should exceed Stop Distance + 1 cm so an approaching Twin can reach its target."), *GetName());
    }
}

void ACBSpectralCreature::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (RunMode.IsValid())
        RunMode->OnRunStateChanged.RemoveDynamic(this, &ACBSpectralCreature::HandleRunStateChanged);
    CaptureTarget.Reset();
    CaptureElapsed=0.f;
    RunMode.Reset();
    Super::EndPlay(EndPlayReason);
}

void ACBSpectralCreature::HandleRunStateChanged(ECBRunState NewState)
{
    if (NewState == ECBRunState::Playing) return;
    // A final result is not an interrupted attempt; clear the warning without a cancellation log.
    CaptureTarget.Reset();
    CaptureElapsed=0.f;
    // Stop both the collision body's motion and the mesh's visual bobbing immediately.
    SetActorTickEnabled(false);
}

float ACBSpectralCreature::GetCaptureProgress() const
{
    return IsCapturePending()
        ? FMath::Clamp(CaptureElapsed / FMath::Max(CaptureWindupDuration, 0.1f), 0.f, 1.f)
        : 0.f;
}

bool ACBSpectralCreature::IsCaptureEligible(APawn* Player) const
{
    if (!bCanCapturePlayer || !bApproachPlayer || !IsValid(Player) || !Player->IsLocallyControlled()
        || !RunMode.IsValid() || !RunMode->IsRunActive()) return false;

    const FVector Delta=Player->GetActorLocation()-GetActorLocation();
    if (Delta.SizeSquared2D() > FMath::Square(FMath::Max(CaptureRange, 0.f))
        || FMath::Abs(Delta.Z) > FMath::Max(MaxCaptureHeightDifference, 0.f)) return false;

    // Capture uses body-height sight, so a low doorway/closed door cannot be bypassed by the tall head ray.
    FHitResult Hit;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(CBTwinCapture), false, this);
    const bool bBlocked=GetWorld()->LineTraceSingleByChannel(Hit, GetActorLocation(),
        Player->GetActorLocation(), ECC_Visibility, Query);
    return !bBlocked || Hit.GetActor() == Player;
}

void ACBSpectralCreature::CancelCapture()
{
    if (IsCapturePending() || CaptureElapsed > 0.f)
        UE_LOG(LogCBTwin, Log, TEXT("%s: capture cancelled"), *GetName());
    CaptureTarget.Reset();
    CaptureElapsed=0.f;
}

void ACBSpectralCreature::UpdateCapture(APawn* Player, float DeltaSeconds)
{
    if (bCanCapturePlayer && !RunMode.IsValid() && !bWarnedMissingRunMode)
    {
        UE_LOG(LogCBTwin, Warning, TEXT("%s: capture disabled because this map has no CBDemoGameMode run owner."), *GetName());
        bWarnedMissingRunMode=true;
    }
    if (!IsCaptureEligible(Player))
    {
        CancelCapture();
        return;
    }
    if (CaptureTarget.Get() != Player)
    {
        CancelCapture();
        CaptureTarget=Player;
        UE_LOG(LogCBTwin, Log, TEXT("%s: capture warning started"), *GetName());
        return;
    }

    CaptureElapsed+=FMath::Max(DeltaSeconds, 0.f);
    if (CaptureElapsed < FMath::Max(CaptureWindupDuration, 0.1f)) return;

    // Recheck at the moment of capture. The run owner accepts at most one final result.
    if (IsCaptureEligible(Player) && RunMode->TryFinishRun(ECBRunState::Lost))
        UE_LOG(LogCBTwin, Log, TEXT("%s: player caught by The Hollow Twin"), *GetName());
    CancelCapture();
}

void ACBSpectralCreature::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    CreatureMesh->SetStaticMesh(AppearanceMesh);
    CreatureMesh->SetRelativeScale3D(FVector(VisualScale));
    CreatureMesh->SetRelativeRotation(FRotator(0,VisualYawOffset,0));
}

void ACBSpectralCreature::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (RunMode.IsValid() && !RunMode->IsRunActive())
    {
        HandleRunStateChanged(RunMode->GetRunState());
        return;
    }
    TimeAlive+=DeltaSeconds;
    // Visual bobbing stays separate from the swept collision body's movement.
    CreatureMesh->SetRelativeLocation(MeshRestLocation+FVector(0,0,FMath::Sin(TimeAlive*1.7f)*HoverAmplitude));
    CreatureMesh->SetRelativeRotation(FRotator(FMath::Sin(TimeAlive*0.9f)*1.5f,VisualYawOffset,FMath::Sin(TimeAlive*1.2f)*2.f));

    APawn* Player=UGameplayStatics::GetPlayerPawn(this,0);
    bool bVisible=false;
    float PlayerDistance=MAX_flt;
    if (IsValid(Player))
    {
        PlayerDistance=FVector::Dist2D(Player->GetActorLocation(),GetActorLocation());
        if (PlayerDistance<=DetectionRange*1.4f)
        {
            FHitResult Sight;
            FCollisionQueryParams Query(SCENE_QUERY_STAT(CBGhostSight),false,this);
            const bool bHit=GetWorld()->LineTraceSingleByChannel(Sight,GetActorLocation()+FVector(0,0,80),
                Player->GetActorLocation()+FVector(0,0,50),ECC_Visibility,Query);
            bVisible=!bHit || Sight.GetActor()==Player;
        }
    }
    FVector Destination=bGoingToEnd ? PatrolEnd : PatrolStart;
    MotionState=ECGhostMotion::Drifting;
    bool bMove=true;
    if (bApproachPlayer && bVisible && PlayerDistance<=DetectionRange)
    {
        Destination=Player->GetActorLocation();
        Destination.Z=PatrolStart.Z;
        bMove=PlayerDistance>StopDistance+1.f;
        MotionState=bMove ? ECGhostMotion::Approaching : ECGhostMotion::Watching;
    }
    else if (FVector::Dist2D(GetActorLocation(),Destination)<15.f)
    {
        bGoingToEnd=!bGoingToEnd;
        Destination=bGoingToEnd ? PatrolEnd : PatrolStart;
    }
    FVector Direction=Destination-GetActorLocation();
    Direction.Z=0;
    const float Remaining=Direction.Size();
    if (bMove && Remaining>1.f)
    {
        float Step=FMath::Min(FMath::Max(0.f,MoveSpeed)*DeltaSeconds,Remaining);
        if (MotionState==ECGhostMotion::Approaching) Step=FMath::Min(Step,FMath::Max(0.f,PlayerDistance-StopDistance));
        FHitResult Hit;
        AddActorWorldOffset(Direction.GetSafeNormal()*Step,true,&Hit);
        if (Hit.bBlockingHit && MotionState==ECGhostMotion::Drifting && GetWorld()->GetTimeSeconds()>NextTurnTime)
        {
            bGoingToEnd=!bGoingToEnd;
            NextTurnTime=GetWorld()->GetTimeSeconds()+0.5;
        }
    }
    // A nearby visible observer draws its gaze even while it drifts sideways.
    FVector Facing=(bVisible && Player) ? Player->GetActorLocation()-GetActorLocation() : Direction;
    Facing.Z=0;
    if (!Facing.IsNearlyZero())
        SetActorRotation(FMath::RInterpTo(GetActorRotation(),FRotator(0,Facing.Rotation().Yaw,0),DeltaSeconds,3.f));

    UpdateCapture(Player, DeltaSeconds);
}

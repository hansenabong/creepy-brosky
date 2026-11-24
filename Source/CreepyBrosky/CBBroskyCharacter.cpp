#include "CBBroskyCharacter.h"
#include "CBBroskyAIController.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogCBBroskyCapture, Log, All);

ACBBroskyCharacter::ACBBroskyCharacter()
{
    PrimaryActorTick.bCanEverTick = true;
    GetCapsuleComponent()->InitCapsuleSize(34.f, 88.f);
    GetCapsuleComponent()->SetGenerateOverlapEvents(false);
    // Moving pawns must not carve their own route out of the navigation surface.
    SetCanAffectNavigationGeneration(false);
    GetCapsuleComponent()->SetCanEverAffectNavigation(false);

    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;
    AIControllerClass = ACBBroskyAIController::StaticClass();
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

    UCharacterMovementComponent* Movement = GetCharacterMovement();
    Movement->bOrientRotationToMovement = true;
    Movement->bUseControllerDesiredRotation = false;
    Movement->RotationRate = FRotator(0.f, 180.f, 0.f);
    Movement->MaxWalkSpeed = PatrolSpeed;
    Movement->MaxStepHeight = 35.f;

    CreatureMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CreatureMesh"));
    CreatureMesh->SetupAttachment(GetCapsuleComponent());
    CreatureMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    CreatureMesh->SetGenerateOverlapEvents(false);
    CreatureMesh->SetCanEverAffectNavigation(false);
    CreatureMesh->SetRelativeLocation(FVector(0.f, 0.f, -88.f));
}

void ACBBroskyCharacter::ApplyAppearance()
{
    CreatureMesh->SetStaticMesh(AppearanceMesh);
    CreatureMesh->SetRelativeScale3D(FVector(FMath::Max(VisualScale, 0.01f)));
    CreatureMesh->SetRelativeLocation(FVector(0.f, 0.f,
        -GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() + VisualHeightOffset));
    CreatureMesh->SetRelativeRotation(FRotator(0.f, VisualYawOffset, 0.f));
}

void ACBBroskyCharacter::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    ApplyAppearance();
}

void ACBBroskyCharacter::BeginPlay()
{
    Super::BeginPlay();
    ApplyAppearance();
    RunMode = GetWorld()->GetAuthGameMode<ACBDemoGameMode>();
    if (RunMode.IsValid())
    {
        RunMode->OnRunStateChanged.AddDynamic(this, &ACBBroskyCharacter::HandleRunStateChanged);
        HandleRunStateChanged(RunMode->GetRunState());
    }
}

void ACBBroskyCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (RunMode.IsValid())
        RunMode->OnRunStateChanged.RemoveDynamic(this, &ACBBroskyCharacter::HandleRunStateChanged);
    CaptureTarget.Reset();
    CaptureElapsed = 0.f;
    RunMode.Reset();
    Super::EndPlay(EndPlayReason);
}

void ACBBroskyCharacter::HandleRunStateChanged(ECBRunState NewState)
{
    if (NewState == ECBRunState::Playing) return;
    CaptureTarget.Reset();
    CaptureElapsed = 0.f;
    GetCharacterMovement()->StopMovementImmediately();
    GetCharacterMovement()->DisableMovement();
    SetActorTickEnabled(false);
}

void ACBBroskyCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (RunMode.IsValid() && !RunMode->IsRunActive())
    {
        HandleRunStateChanged(RunMode->GetRunState());
        return;
    }

    // Only the visual moves here. CharacterMovement moves the collision body along paths.
    TimeAlive += DeltaSeconds;
    const float RestHeight = -GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() + VisualHeightOffset;
    CreatureMesh->SetRelativeLocation(FVector(0.f, 0.f,
        RestHeight + FMath::Sin(TimeAlive * 1.7f) * FMath::Max(HoverAmplitude, 0.f)));
    CreatureMesh->SetRelativeRotation(FRotator(FMath::Sin(TimeAlive * 0.9f) * 1.5f,
        VisualYawOffset, FMath::Sin(TimeAlive * 1.2f) * 2.f));
}

float ACBBroskyCharacter::GetCaptureProgress() const
{
    return IsCapturePending()
        ? FMath::Clamp(CaptureElapsed / FMath::Max(CaptureWindupDuration, 0.1f), 0.f, 1.f)
        : 0.f;
}

bool ACBBroskyCharacter::IsCaptureEligible(APawn* Player) const
{
    if (!bCanCapturePlayer || !IsValid(Player) || !Player->IsPlayerControlled()
        || !Player->IsLocallyControlled() || !RunMode.IsValid() || !RunMode->IsRunActive()) return false;

    const FVector Delta = Player->GetActorLocation() - GetActorLocation();
    if (Delta.SizeSquared2D() > FMath::Square(FMath::Max(CaptureRange, 0.f))
        || FMath::Abs(Delta.Z) > FMath::Max(MaxCaptureHeightDifference, 0.f)) return false;

    FHitResult Hit;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(CBBroskyCapture), false, this);
    const bool bBlocked = GetWorld()->LineTraceSingleByChannel(Hit, GetActorLocation(),
        Player->GetActorLocation(), ECC_Visibility, Query);
    return !bBlocked || Hit.GetActor() == Player;
}

void ACBBroskyCharacter::CancelCapture()
{
    if (IsCapturePending() || CaptureElapsed > 0.f)
        UE_LOG(LogCBBroskyCapture, Log, TEXT("%s: capture cancelled"), *GetName());
    CaptureTarget.Reset();
    CaptureElapsed = 0.f;
}

void ACBBroskyCharacter::UpdateCapture(APawn* VisiblePlayer, float DeltaSeconds)
{
    if (bCanCapturePlayer && !RunMode.IsValid() && !bWarnedMissingRunMode)
    {
        UE_LOG(LogCBBroskyCapture, Warning,
            TEXT("%s: capture disabled because this map has no CBDemoGameMode run owner."), *GetName());
        bWarnedMissingRunMode = true;
    }
    // The controller passes nullptr as soon as sight is lost; capture never uses a remembered position.
    if (!IsCaptureEligible(VisiblePlayer))
    {
        CancelCapture();
        return;
    }
    if (CaptureTarget.Get() != VisiblePlayer)
    {
        CancelCapture();
        CaptureTarget = VisiblePlayer;
        UE_LOG(LogCBBroskyCapture, Log, TEXT("%s: capture warning started"), *GetName());
        return;
    }

    CaptureElapsed += FMath::Max(DeltaSeconds, 0.f);
    if (CaptureElapsed < FMath::Max(CaptureWindupDuration, 0.1f)) return;

    // GameMode rechecks the run state and accepts at most one final outcome.
    if (IsCaptureEligible(VisiblePlayer) && RunMode->TryFinishRun(ECBRunState::Lost))
        UE_LOG(LogCBBroskyCapture, Log, TEXT("%s: player caught by The Hollow Twin"), *GetName());
    CancelCapture();
}

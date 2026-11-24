#include "CBInteractableDoor.h"
#include "CBDemoGameMode.h"
#include "CBInteractionComponent.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogCBDoor, Log, All);

ACBInteractableDoor::ACBInteractableDoor()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = false;
    DoorRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DoorRoot"));
    SetRootComponent(DoorRoot);
    Hinge = CreateDefaultSubobject<USceneComponent>(TEXT("Hinge"));
    Hinge->SetupAttachment(DoorRoot);
    Hinge->SetRelativeLocation(FVector(0.0f, -60.0f, 0.0f));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    auto MakeMesh = [this](const TCHAR* Name, USceneComponent* Parent, const FVector& Position, const FVector& Scale)
    {
        UStaticMeshComponent* Mesh = CreateDefaultSubobject<UStaticMeshComponent>(Name);
        Mesh->SetupAttachment(Parent);
        Mesh->SetStaticMesh(Cube.Object);
        Mesh->SetRelativeLocation(Position);
        Mesh->SetRelativeScale3D(Scale);
        Mesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
        Mesh->SetGenerateOverlapEvents(false);
        Mesh->SetMobility(EComponentMobility::Movable);
        return Mesh;
    };
    DoorPanel = MakeMesh(TEXT("DoorPanel"), Hinge, FVector(0,60,110), FVector(0.1,1.2,2.2));
    FrameLeft = MakeMesh(TEXT("FrameLeft"), DoorRoot, FVector(0,-67,115), FVector(0.2,0.14,2.3));
    FrameRight = MakeMesh(TEXT("FrameRight"), DoorRoot, FVector(0,67,115), FVector(0.2,0.14,2.3));
    FrameTop = MakeMesh(TEXT("FrameTop"), DoorRoot, FVector(0,0,227), FVector(0.2,1.48,0.14));
    // Attach to the hinge rather than the scaled panel, so the handle keeps its intended dimensions.
    Handle = MakeMesh(TEXT("Handle"), Hinge, FVector(0,105,105), FVector(0.18,0.06,0.06));
    Handle->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    InteractionPrompt = CreateDefaultSubobject<UTextRenderComponent>(TEXT("InteractionPrompt"));
    InteractionPrompt->SetupAttachment(DoorRoot);
    InteractionPrompt->SetRelativeLocation(FVector(0,0,165));
    InteractionPrompt->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
    InteractionPrompt->SetVerticalAlignment(EVerticalTextAligment::EVRTA_TextCenter);
    InteractionPrompt->SetWorldSize(10.0f);
    InteractionPrompt->SetTextRenderColor(FColor(255,224,150));
    InteractionPrompt->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    InteractionPrompt->SetHiddenInGame(true);
    InteractionPrompt->SetText(FText::FromString(TEXT("[E] Open door")));
}

void ACBInteractableDoor::BeginPlay()
{
    Super::BeginPlay();
    ActiveOpenAngle = OpenAngle;
    Hinge->SetRelativeRotation(FRotator::ZeroRotator);
    InteractionPrompt->SetHiddenInGame(true);
}

void ACBInteractableDoor::ToggleDoor()
{
    if (bLocked)
    {
        return;
    }

    SetDoorOpen(!bTargetOpen);
}

void ACBInteractableDoor::SetDoorOpen(bool bOpen)
{
    const ACBDemoGameMode* Rules = GetWorld() ? GetWorld()->GetAuthGameMode<ACBDemoGameMode>() : nullptr;
    if (Rules && !Rules->IsRunActive()) return;
    if (bOpen && bLocked) return;
    if (bOpen == bTargetOpen) return;
    if (bOpen && OpenFraction <= KINDA_SMALL_NUMBER)
    {
        ActiveOpenAngle = OpenAngle;
        if (bOpenAwayFromPlayer && FocusedViewer.IsValid() && FocusedViewer->GetPawn())
        {
            const FVector LocalPlayer = GetActorTransform().InverseTransformPosition(FocusedViewer->GetPawn()->GetActorLocation());
            ActiveOpenAngle = LocalPlayer.X < 0 ? -FMath::Abs(OpenAngle) : FMath::Abs(OpenAngle);
        }
    }
    bTargetOpen = bOpen;
    UE_LOG(LogCBDoor, Log, TEXT("%s: %s"), *GetName(), bOpen ? TEXT("opening") : TEXT("closing"));
    SetActorTickEnabled(true);
    RefreshPrompt();
}

void ACBInteractableDoor::TryInteract(UCBInteractionComponent* PlayerInteraction, APlayerController* Viewer)
{
    if (!IsValid(PlayerInteraction) || !Viewer || !PlayerInteraction->IsInteractionAllowed()) return;
    SetInteractionFocus(true,Viewer);
    if (bLocked)
    {
        if (!PlayerInteraction->bHasKey)
        {
            PlayerInteraction->NotifyPlayer(FText::FromString(TEXT("Locked. Find the exit key.")));
            return;
        }
        bLocked = false;
        PlayerInteraction->NotifyPlayer(FText::FromString(TEXT("Door unlocked. Head through the exit.")));
    }
    ToggleDoor();
}

bool ACBInteractableDoor::WouldTouchPawn(float ProposedYaw) const
{
    if (!GetWorld() || !DoorPanel->GetStaticMesh()) return false;
    const FTransform ProposedHinge(FRotator(0,ProposedYaw,0), Hinge->GetRelativeLocation(), Hinge->GetRelativeScale3D());
    const FTransform PanelWorld = DoorPanel->GetRelativeTransform() * ProposedHinge * DoorRoot->GetComponentTransform();
    const FBoxSphereBounds LocalBounds = DoorPanel->GetStaticMesh()->GetBounds();
    const FVector Center = PanelWorld.TransformPosition(LocalBounds.Origin);
    const FVector Extent = LocalBounds.BoxExtent * PanelWorld.GetScale3D().GetAbs();
    FCollisionQueryParams Params(SCENE_QUERY_STAT(DoorPawnSafety), false, this);
    return GetWorld()->OverlapAnyTestByObjectType(Center, PanelWorld.GetRotation(),
        FCollisionObjectQueryParams(ECC_Pawn), FCollisionShape::MakeBox(Extent * 0.96f), Params);
}

void ACBInteractableDoor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    const ACBDemoGameMode* Rules = GetWorld() ? GetWorld()->GetAuthGameMode<ACBDemoGameMode>() : nullptr;
    if (Rules && !Rules->IsRunActive())
    {
        SetActorTickEnabled(false);
        return;
    }
    const float Goal = bTargetOpen ? 1.0f : 0.0f;
    const float Proposed = FMath::FInterpConstantTo(OpenFraction, Goal, DeltaSeconds, 1.0f / FMath::Max(OpeningDuration,0.1f));
    const float Eased = Proposed * Proposed * (3.0f - 2.0f * Proposed);
    const float CurrentYaw = Hinge->GetRelativeRotation().Yaw;
    const float ProposedYaw = Eased * ActiveOpenAngle;
    const int32 Steps = FMath::Max(1,FMath::CeilToInt(FMath::Abs(ProposedYaw-CurrentYaw)/3.0f));
    bool bObstructed = false;
    for (int32 Step=1; Step<=Steps; ++Step)
    {
        if (WouldTouchPawn(FMath::Lerp(CurrentYaw,ProposedYaw,float(Step)/Steps)))
        {
            bObstructed=true;
            break;
        }
    }
    if (bObstructed)
    {
        // Reopen on a closing obstruction; do not push the player through nearby geometry.
        if (!bTargetOpen) SetDoorOpen(true);
        return;
    }
    OpenFraction = Proposed;
    Hinge->SetRelativeRotation(FRotator(0,ProposedYaw,0));
    if (FMath::IsNearlyEqual(OpenFraction,Goal)) SetActorTickEnabled(false);
}

void ACBInteractableDoor::RefreshPrompt()
{
    if (bLocked)
    {
        const APawn* Pawn = FocusedViewer.IsValid() ? FocusedViewer->GetPawn() : nullptr;
        const UCBInteractionComponent* Interaction = Pawn ? Pawn->FindComponentByClass<UCBInteractionComponent>() : nullptr;
        InteractionPrompt->SetText(FText::FromString(Interaction && Interaction->bHasKey
            ? TEXT("[E] Unlock door") : TEXT("[E] Locked - find the key")));
        return;
    }

    InteractionPrompt->SetText(
        FText::FromString(
            bTargetOpen ? TEXT("[E] Close door") : TEXT("[E] Open door")
        )
    );
}

void ACBInteractableDoor::SetInteractionFocus(bool bFocused, APlayerController* Viewer)
{
    FocusedViewer = bFocused ? Viewer : nullptr;
    InteractionPrompt->SetHiddenInGame(!bFocused);
    if (!bFocused || !Viewer) return;
    FVector Eye;
    FRotator ViewRotation;
    Viewer->GetPlayerViewPoint(Eye, ViewRotation);
    // Place the prompt on the viewer's side of the panel to avoid hiding it inside the mesh.
    const FVector PanelCenter = DoorPanel->GetComponentLocation();
    FVector Direction = (Eye - PanelCenter).GetSafeNormal();
    FVector PromptPosition = PanelCenter + Direction * 18.0f + FVector(0,0,35);
    InteractionPrompt->SetWorldLocation(PromptPosition);
    InteractionPrompt->SetWorldRotation((Eye - PromptPosition).Rotation());
    RefreshPrompt();
}

#include "CBKeyPickup.h"

#include "CBInteractionComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/PlayerController.h"
#include "UObject/ConstructorHelpers.h"

ACBKeyPickup::ACBKeyPickup()
{
    PrimaryActorTick.bCanEverTick = false;
    PickupBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("PickupBounds"));
    SetRootComponent(PickupBounds);
    PickupBounds->SetBoxExtent(FVector(30,18,16));
    PickupBounds->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    PickupBounds->SetCollisionObjectType(ECC_WorldDynamic);
    PickupBounds->SetCollisionResponseToAllChannels(ECR_Ignore);
    PickupBounds->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
    PickupBounds->SetGenerateOverlapEvents(false);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    auto MakePart = [this](const TCHAR* Name, UStaticMesh* Mesh, FVector Position, FVector Scale)
    {
        UStaticMeshComponent* Part = CreateDefaultSubobject<UStaticMeshComponent>(Name);
        Part->SetupAttachment(PickupBounds);
        Part->SetStaticMesh(Mesh);
        Part->SetRelativeLocation(Position);
        Part->SetRelativeScale3D(Scale);
        Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        return Part;
    };
    KeyHead = MakePart(TEXT("KeyHead"),Cylinder.Object,FVector(-13,0,0),FVector(0.20,0.20,0.04));
    KeyShaft = MakePart(TEXT("KeyShaft"),Cube.Object,FVector(6,0,0),FVector(0.30,0.05,0.04));
    KeyTeeth = MakePart(TEXT("KeyTeeth"),Cube.Object,FVector(17,4,0),FVector(0.05,0.12,0.04));
    InteractionPrompt = CreateDefaultSubobject<UTextRenderComponent>(TEXT("InteractionPrompt"));
    InteractionPrompt->SetupAttachment(PickupBounds);
    InteractionPrompt->SetText(FText::FromString(TEXT("[E] Take exit key")));
    InteractionPrompt->SetWorldSize(8.0f);
    InteractionPrompt->SetTextRenderColor(FColor(255,224,150));
    InteractionPrompt->SetHorizontalAlignment(EHTA_Center);
    InteractionPrompt->SetVerticalAlignment(EVRTA_TextCenter);
    InteractionPrompt->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    InteractionPrompt->SetHiddenInGame(true);
}

void ACBKeyPickup::SetInteractionFocus(bool bFocused, APlayerController* Viewer)
{
    InteractionPrompt->SetHiddenInGame(!bFocused || bCollected);
    if (!bFocused || bCollected || !Viewer) return;
    FVector Eye;
    FRotator ViewRotation;
    Viewer->GetPlayerViewPoint(Eye,ViewRotation);
    const FVector PromptPosition=GetActorLocation()+FVector(0,0,28);
    InteractionPrompt->SetWorldLocation(PromptPosition);
    InteractionPrompt->SetWorldRotation((Eye-PromptPosition).Rotation());
}

void ACBKeyPickup::Collect(UCBInteractionComponent* PlayerInteraction)
{
    if (bCollected || !IsValid(PlayerInteraction) || !PlayerInteraction->IsInteractionAllowed()) return;
    bCollected = true;
    PlayerInteraction->bHasKey = true;
    PlayerInteraction->NotifyPlayer(FText::FromString(TEXT("Exit key collected. Return to the locked door.")));
    SetActorEnableCollision(false);
    SetActorHiddenInGame(true);
    Destroy();
}

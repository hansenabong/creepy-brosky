#include "CBExitZone.h"
#include "CBDemoGameMode.h"
#include "CBInteractableDoor.h"
#include "CBInteractionComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/Pawn.h"

ACBExitZone::ACBExitZone()
{
    PrimaryActorTick.bCanEverTick=true;
    PrimaryActorTick.TickInterval=0.1f;
    ExitVolume=CreateDefaultSubobject<UBoxComponent>(TEXT("ExitVolume"));
    SetRootComponent(ExitVolume);
    ExitVolume->SetBoxExtent(FVector(80,55,110));
    ExitVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    ExitVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
    ExitVolume->SetCollisionResponseToChannel(ECC_Pawn,ECR_Overlap);
    ExitVolume->SetGenerateOverlapEvents(true);
}

void ACBExitZone::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!GetWorld()) return;
    ACBDemoGameMode* Rules = GetWorld()->GetAuthGameMode<ACBDemoGameMode>();
    if (!Rules)
    {
        if (!bWarnedMissingGameMode)
        {
            UE_LOG(LogTemp, Warning, TEXT("CreepyBrosky: %s needs CBDemoGameMode (or a subclass) to finish a run."), *GetName());
            bWarnedMissingGameMode = true;
        }
        return;
    }
    if (!Rules->IsRunActive())
    {
        SetActorTickEnabled(false);
        return;
    }
    if (!IsValid(ExitDoor) || ExitDoor->bLocked || ExitDoor->GetOpenFraction()<0.65f) return;
    // Poll while inside, so stepping in during the opening animation cannot miss completion.
    TArray<FOverlapResult> Overlapping;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(CBDemoExit),false,this);
    GetWorld()->OverlapMultiByObjectType(Overlapping,ExitVolume->GetComponentLocation(),
        ExitVolume->GetComponentQuat(),FCollisionObjectQueryParams(ECC_Pawn),
        FCollisionShape::MakeBox(ExitVolume->GetScaledBoxExtent()),Query);
    for (const FOverlapResult& Overlap:Overlapping)
    {
        APawn* Pawn=Cast<APawn>(Overlap.GetActor());
        UCBInteractionComponent* Interaction=Pawn ? Pawn->FindComponentByClass<UCBInteractionComponent>() : nullptr;
        if (Pawn && Pawn->IsPlayerControlled() && Pawn->IsLocallyControlled() && Interaction && Interaction->bHasKey)
        {
            if (Rules->TryFinishRun(ECBRunState::Won))
            {
                UE_LOG(LogTemp,Log,TEXT("CreepyBrosky: escape accepted by the run rules"));
                SetActorTickEnabled(false);
            }
            break;
        }
    }
}

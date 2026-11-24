#include "CBDemoGameMode.h"
#include "CBDemoHUD.h"
#include "CBInteractionComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogCBRun, Log, All);

ACBDemoGameMode::ACBDemoGameMode()
{
    static ConstructorHelpers::FClassFinder<APawn> Character(TEXT("/Game/Variant_Horror/Blueprints/BP_HorrorCharacter"));
    static ConstructorHelpers::FClassFinder<APlayerController> Controller(TEXT("/Game/Variant_Horror/Blueprints/BP_HorrorPlayerController"));
    if (Character.Succeeded()) DefaultPawnClass=Character.Class;
    if (Controller.Succeeded()) PlayerControllerClass=Controller.Class;
    HUDClass=ACBDemoHUD::StaticClass();
}

void ACBDemoGameMode::RestartPlayer(AController* NewPlayer)
{
    if (!IsRunActive()) return;
    Super::RestartPlayer(NewPlayer);
    if (NewPlayer && NewPlayer->GetPawn())
    {
        NewPlayer->ResetIgnoreMoveInput();
        NewPlayer->ResetIgnoreLookInput();
        if (UCBInteractionComponent* Interaction=NewPlayer->GetPawn()->FindComponentByClass<UCBInteractionComponent>())
        {
            // The demo always starts empty, even if the practice Blueprint has a debug key enabled.
            Interaction->bHasKey=false;
        }
    }
}

bool ACBDemoGameMode::TryFinishRun(ECBRunState Result)
{
    if (!IsRunActive() || (Result != ECBRunState::Won && Result != ECBRunState::Lost)) return false;

    // Assign before notifying listeners so a second request in this frame cannot change the result.
    RunState = Result;
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        APlayerController* Controller = It->Get();
        if (!Controller) continue;
        Controller->StopMovement();
        Controller->SetIgnoreMoveInput(true);
        Controller->SetIgnoreLookInput(true);
        if (ACharacter* Character = Cast<ACharacter>(Controller->GetPawn()))
        {
            Character->GetCharacterMovement()->StopMovementImmediately();
            Character->GetCharacterMovement()->DisableMovement();
        }
    }
    // Do not pause or disable input: the existing R action must still reload the level.
    OnRunStateChanged.Broadcast(RunState);
    UE_LOG(LogCBRun, Log, TEXT("Run finished: %s"), Result == ECBRunState::Won ? TEXT("Won") : TEXT("Lost"));
    return true;
}

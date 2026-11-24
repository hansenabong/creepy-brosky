#include "CBDemoHUD.h"
#include "CBDemoGameMode.h"
#include "CBInteractionComponent.h"
#include "CBSpectralCreature.h"
#include "CBBroskyCharacter.h"
#include "CBBroskyAIController.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"

float ACBDemoHUD::GetHUDScale() const
{
    return Canvas ? FMath::Clamp(FMath::Min(Canvas->ClipX / 1280.f, Canvas->ClipY / 720.f), 0.5f, 2.f) : 1.f;
}

bool ACBDemoHUD::DrawRunResult()
{
    const ACBDemoGameMode* Rules = GetWorld() ? GetWorld()->GetAuthGameMode<ACBDemoGameMode>() : nullptr;
    if (!Canvas || !GEngine || !Rules || Rules->IsRunActive()) return false;

    const bool bWon = Rules->GetRunState() == ECBRunState::Won;
    const float W = Canvas->ClipX, H = Canvas->ClipY, S = GetHUDScale();
    const FLinearColor Accent = bWon ? FLinearColor(0.95f, 0.72f, 0.3f) : FLinearColor(1.f, 0.25f, 0.2f);
    const FLinearColor White(0.9f, 0.93f, 0.94f);
    UFont* Font = GEngine->GetMediumFont();
    DrawRect(FLinearColor(0.01f, 0.02f, 0.025f, 0.96f), 0, 0, W, H);

    const float X = FMath::Max(20.f, W / 2 - 330 * S), Y = H / 2 - 85 * S;
    DrawRect(Accent, X, Y, 64 * S, 4 * S);
    DrawText(bWon ? TEXT("YOU MADE IT OUT") : TEXT("CAUGHT BY THE HOLLOW TWIN"),
        White, X, Y + 25 * S, Font, 1.8f * S);
    DrawText(bWon ? TEXT("Key found. Exit unlocked. You escaped.")
        : TEXT("Break sight or leave his reach before the warning fills."),
        White, X, Y + 78 * S, Font, S);
    DrawText(TEXT("Press R to start a fresh run"), Accent, X, Y + 125 * S, Font, 1.2f * S);
    return true;
}

void ACBDemoHUD::DrawCaptureWarning()
{
    const ACBDemoGameMode* Rules = GetWorld() ? GetWorld()->GetAuthGameMode<ACBDemoGameMode>() : nullptr;
    if (!Canvas || !GEngine || !Rules || !Rules->IsRunActive()) return;

    // The enemy owns capture progress. The HUD only reads the most urgent pending attempt.
    float Progress = 0.f;
    for (TActorIterator<ACBSpectralCreature> It(GetWorld()); It; ++It)
    {
        if (It->IsCapturePending()) Progress = FMath::Max(Progress, It->GetCaptureProgress());
    }
    for (TActorIterator<ACBBroskyCharacter> It(GetWorld()); It; ++It)
    {
        if (It->IsCapturePending()) Progress = FMath::Max(Progress, It->GetCaptureProgress());
    }
    if (Progress <= 0.f) return;

    const float W = Canvas->ClipX, H = Canvas->ClipY, S = GetHUDScale();
    const float X = W / 2 - 280 * S, Y = H - 280 * S;
    const FLinearColor Red(1.f, 0.2f, 0.15f), White(1.f, 0.93f, 0.9f);
    UFont* Font = GEngine->GetMediumFont();
    DrawRect(FLinearColor(0.12f, 0.012f, 0.01f, 0.94f), X, Y, 560 * S, 96 * S);
    DrawRect(Red, X, Y, 5 * S, 96 * S);
    DrawText(TEXT("THE HOLLOW TWIN IS REACHING FOR YOU"), Red, X + 20 * S, Y + 13 * S, Font, 1.2f * S);
    DrawText(TEXT("MOVE AWAY / BREAK SIGHT"), White, X + 20 * S, Y + 41 * S, Font, 1.3f * S);
    DrawRect(FLinearColor(0.25f, 0.08f, 0.07f), X + 20 * S, Y + 78 * S, 520 * S, 8 * S);
    DrawRect(Red, X + 20 * S, Y + 78 * S, 520 * S * FMath::Clamp(Progress, 0.f, 1.f), 8 * S);
}

void ACBDemoHUD::DrawHUD()
{
    Super::DrawHUD();
    if (!Canvas || !GEngine || DrawRunResult() || !PlayerOwner || !PlayerOwner->GetPawn()) return;
    const UCBInteractionComponent* Interaction = PlayerOwner->GetPawn()->FindComponentByClass<UCBInteractionComponent>();
    if (!Interaction) return;
    const float W=Canvas->ClipX, H=Canvas->ClipY;
    const float S=GetHUDScale();
    const FLinearColor Ink(0.035f,0.045f,0.055f,0.9f), Accent(0.95f,0.72f,0.3f), White(0.9f,0.93f,0.94f);
    UFont* Font=GEngine->GetMediumFont();
    DrawRect(Ink,24*S,24*S,550*S,108*S);
    DrawRect(Accent,24*S,24*S,4*S,108*S);
    DrawText(TEXT("CREEPYBROSKY  /  THE SERVICE EXIT"),Accent,44*S,38*S,Font,1.3f*S);
    DrawText(Interaction->bHasKey ? TEXT("Unlock the exit and get outside.") : TEXT("Find the brass key in the storage room."),White,44*S,69*S,Font,1.55f*S);
    DrawText(Interaction->bHasKey ? TEXT("KEY: COLLECTED") : TEXT("KEY: MISSING"),Accent,44*S,104*S,Font,1.1f*S);
    // Optional learning overlay. It reads AI state; it never decides or advances behavior.
    for (TActorIterator<ACBBroskyCharacter> It(GetWorld()); It; ++It)
    {
        if (!It->bShowAIDebug) continue;
        if (const ACBBroskyAIController* AI=Cast<ACBBroskyAIController>(It->GetController()))
        {
            DrawRect(Ink,24*S,145*S,900*S,32*S);
            DrawText(AI->GetDebugSummary(),White,36*S,152*S,Font,0.9f*S);
        }
        break;
    }
    DrawRect(White,W/2-2*S,H/2-2*S,4*S,4*S);
    const FString Message=Interaction->GetPlayerMessage().ToString();
    if (!Message.IsEmpty())
    {
        DrawRect(Ink,W/2-270*S,H-160*S,540*S,44*S);
        DrawText(Message,Accent,W/2-250*S,H-150*S,Font,1.3f*S);
    }
    DrawRect(Ink,24*S,H-56*S,570*S,32*S);
    DrawText(TEXT("WASD Move   |   Mouse Look   |   E Use   |   R Restart"),White,36*S,H-49*S,Font,1.2f*S);
    DrawCaptureWarning();
}

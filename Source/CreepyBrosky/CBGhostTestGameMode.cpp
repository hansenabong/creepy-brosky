#include "CBGhostTestGameMode.h"
#include "CBSpectralCreature.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"

ACBGhostTestGameMode::ACBGhostTestGameMode()
{
    HUDClass=ACBGhostTestHUD::StaticClass();
}

void ACBGhostTestHUD::DrawHUD()
{
    // Share result/warning drawing while keeping this map's teaching overlay instead of the escape objective.
    AHUD::DrawHUD();
    if (!Canvas || !GEngine || DrawRunResult()) return;
    if (!Creature.IsValid()) Creature=Cast<ACBSpectralCreature>(UGameplayStatics::GetActorOfClass(this,ACBSpectralCreature::StaticClass()));
    const float S=GetHUDScale();
    const FLinearColor Pale(0.78f,0.91f,0.86f), Ink(0.015f,0.025f,0.03f,0.9f);
    DrawRect(Ink,24*S,24*S,665*S,137*S);
    DrawText(TEXT("THE HOLLOW TWIN  /  CREATURE STUDY"),Pale,40*S,36*S,GEngine->GetMediumFont(),1.4f*S);
    DrawText(TEXT("Approach to draw its attention. Step back to keep your distance."),FLinearColor::White,40*S,68*S,GEngine->GetMediumFont(),1.2f*S);
    FString State=TEXT("Drifting");
    if (Creature.IsValid())
    {
        if (Creature->MotionState==ECGhostMotion::Approaching) State=TEXT("Approaching");
        if (Creature->MotionState==ECGhostMotion::Watching) State=TEXT("Watching");
    }
    DrawText(TEXT("STATE: ")+State,Pale,40*S,100*S,GEngine->GetMediumFont(),1.2f*S);
    const FString CaptureMode = !Creature.IsValid() ? TEXT("CAPTURE: no creature found")
        : Creature->bCanCapturePlayer ? TEXT("CAPTURE: enabled - retreat during the warning")
        : TEXT("CAPTURE: disabled - harmless movement study");
    DrawText(CaptureMode,Pale,40*S,130*S,GEngine->GetMediumFont(),1.1f*S);
    DrawRect(Ink,24*S,Canvas->ClipY-56*S,470*S,32*S);
    DrawText(TEXT("WASD Move   |   Mouse Look   |   R Reset"),Pale,36*S,Canvas->ClipY-50*S,GEngine->GetMediumFont(),1.3f*S);
    DrawRect(Pale,Canvas->ClipX/2-1.5f,Canvas->ClipY/2-1.5f,3,3);
    DrawCaptureWarning();
}

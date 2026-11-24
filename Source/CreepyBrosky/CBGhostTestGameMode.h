#pragma once
#include "CoreMinimal.h"
#include "CBDemoGameMode.h"
#include "CBDemoHUD.h"
#include "CBGhostTestGameMode.generated.h"
class ACBSpectralCreature;

UCLASS()
class CREEPYBROSKY_API ACBGhostTestHUD : public ACBDemoHUD
{
    GENERATED_BODY()
public:
    virtual void DrawHUD() override;
private:
    TWeakObjectPtr<ACBSpectralCreature> Creature;
};

UCLASS()
class CREEPYBROSKY_API ACBGhostTestGameMode : public ACBDemoGameMode
{
    GENERATED_BODY()
public:
    ACBGhostTestGameMode();
};

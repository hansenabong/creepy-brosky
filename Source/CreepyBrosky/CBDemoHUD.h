#pragma once
#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "CBDemoHUD.generated.h"

/** Read-only presentation of the local player's objective and progress. */
UCLASS()
class CREEPYBROSKY_API ACBDemoHUD : public AHUD
{
    GENERATED_BODY()
public:
    virtual void DrawHUD() override;

protected:
    /** Shared presentation for the escape level and the creature study. */
    bool DrawRunResult();
    void DrawCaptureWarning();
    float GetHUDScale() const;
};

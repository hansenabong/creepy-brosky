#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CBDemoGameMode.generated.h"

/** A run has one final outcome. Restarting the level creates a new Playing run. */
UENUM(BlueprintType)
enum class ECBRunState : uint8
{
    Playing,
    Won,
    Lost
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCBRunStateChanged, ECBRunState, NewState);

UCLASS()
class CREEPYBROSKY_API ACBDemoGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    ACBDemoGameMode();
    virtual void RestartPlayer(AController* NewPlayer) override;

    UFUNCTION(BlueprintPure, Category="Run")
    ECBRunState GetRunState() const { return RunState; }

    UFUNCTION(BlueprintPure, Category="Run")
    bool IsRunActive() const { return RunState == ECBRunState::Playing; }

    /** First valid terminal request wins; callers cannot overwrite an outcome. */
    UFUNCTION(BlueprintCallable, Category="Run")
    bool TryFinishRun(ECBRunState Result);

    UPROPERTY(BlueprintAssignable, Category="Run")
    FCBRunStateChanged OnRunStateChanged;

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Run", meta=(AllowPrivateAccess="true"))
    ECBRunState RunState = ECBRunState::Playing;
};

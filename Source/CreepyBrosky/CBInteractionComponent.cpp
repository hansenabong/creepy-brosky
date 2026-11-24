#include "CBInteractionComponent.h"

#include "CBInteractableDoor.h"
#include "CBKeyPickup.h"
#include "EnhancedInputComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "Kismet/GameplayStatics.h"

UCBInteractionComponent::UCBInteractionComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
}

void UCBInteractionComponent::BeginPlay()
{
    Super::BeginPlay();
    RefreshRunStateBinding();
}

bool UCBInteractionComponent::IsInteractionAllowed() const
{
    const ACBDemoGameMode* Rules = GetWorld() ? GetWorld()->GetAuthGameMode<ACBDemoGameMode>() : nullptr;
    return !Rules || Rules->IsRunActive();
}

bool UCBInteractionComponent::IsDemoCompleted() const
{
    const ACBDemoGameMode* Rules = GetWorld() ? GetWorld()->GetAuthGameMode<ACBDemoGameMode>() : nullptr;
    return Rules && Rules->GetRunState() == ECBRunState::Won;
}

void UCBInteractionComponent::RefreshRunStateBinding()
{
    ACBDemoGameMode* Rules = GetWorld() ? GetWorld()->GetAuthGameMode<ACBDemoGameMode>() : nullptr;
    if (BoundGameMode.Get() == Rules) return;
    if (ACBDemoGameMode* Previous = BoundGameMode.Get())
        Previous->OnRunStateChanged.RemoveDynamic(this, &UCBInteractionComponent::HandleRunStateChanged);
    BoundGameMode = Rules;
    if (Rules)
    {
        Rules->OnRunStateChanged.AddUniqueDynamic(this, &UCBInteractionComponent::HandleRunStateChanged);
        HandleRunStateChanged(Rules->GetRunState());
    }
}

void UCBInteractionComponent::HandleRunStateChanged(ECBRunState NewState)
{
    if (NewState == ECBRunState::Playing) return;
    ClearFocus();
    PlayerMessage = FText::GetEmpty();
    MessageExpiresAt = 0.0;
    // Keep both bindings alive: interaction rejects the outcome, while R can reload the map.
}

AActor* UCBInteractionComponent::TraceInteractableInView(UWorld* World,
    const FVector& Origin, const FVector& Direction, float Distance, AActor* IgnoredActor)
{
    if (!World || Distance <= 0.0f || Direction.IsNearlyZero()) return nullptr;
    FHitResult Hit;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(CBInteractionTrace), false, IgnoredActor);
    const FVector End = Origin + Direction.GetSafeNormal() * Distance;
    if (World->LineTraceSingleByChannel(Hit, Origin, End, ECC_Visibility, Query))
    {
        AActor* HitActor = Hit.GetActor();
        if (IsValid(HitActor) && (HitActor->IsA<ACBInteractableDoor>() || HitActor->IsA<ACBKeyPickup>()))
            return HitActor;
    }
    return nullptr;
}

ACBInteractableDoor* UCBInteractionComponent::TraceDoorInView(UWorld* World,
    const FVector& Origin, const FVector& Direction, float Distance, AActor* IgnoredActor)
{
    return Cast<ACBInteractableDoor>(TraceInteractableInView(World, Origin, Direction, Distance, IgnoredActor));
}

ACBInteractableDoor* UCBInteractionComponent::GetFocusedDoor() const
{
    return Cast<ACBInteractableDoor>(FocusedActor.Get());
}

void UCBInteractionComponent::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    RefreshRunStateBinding();
    APlayerController* Controller = GetLocalController();
    if (!Controller)
    {
        ClearFocus();
        RemoveInputBinding();
        ViewingController.Reset();
        return;
    }
    if (ViewingController.Get() != Controller)
    {
        ClearFocus();
        RemoveInputBinding();
        ViewingController = Controller;
    }

    // Possession can create or replace this input component after BeginPlay.
    UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(GetOwner()->InputComponent);
    if (BoundInput.Get() != Input || BoundAction.Get() != InteractionAction.Get()
        || BoundRestartAction.Get() != RestartAction.Get())
    {
        RemoveInputBinding();
        if (Input)
        {
            BoundInput = Input;
            if (InteractionAction)
            {
                InteractionBindingHandle = Input->BindAction(InteractionAction, ETriggerEvent::Started,
                    this, &UCBInteractionComponent::HandleInteract).GetHandle();
                BoundAction = InteractionAction.Get();
            }
            if (RestartAction)
            {
                RestartBindingHandle = Input->BindAction(RestartAction, ETriggerEvent::Started,
                    this, &UCBInteractionComponent::HandleRestart).GetHandle();
                BoundRestartAction = RestartAction.Get();
            }
        }
    }

    // Bind input before checking the outcome, so R also works after winning or losing.
    if (!IsInteractionAllowed())
    {
        ClearFocus();
        return;
    }
    FVector ViewLocation;
    FRotator ViewRotation;
    Controller->GetPlayerViewPoint(ViewLocation, ViewRotation);
    SetFocus(TraceInteractableInView(GetWorld(), ViewLocation, ViewRotation.Vector(),
        MaxInteractionDistance, GetOwner()), Controller);
}

APlayerController* UCBInteractionComponent::GetLocalController() const
{
    const APawn* Pawn = Cast<APawn>(GetOwner());
    APlayerController* Controller = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
    return Pawn && Pawn->IsLocallyControlled() && Controller && Controller->IsLocalController()
        ? Controller : nullptr;
}

void UCBInteractionComponent::SetFocus(AActor* Actor, APlayerController* Controller)
{
    if (FocusedActor.Get() != Actor)
    {
        ClearFocus();
        FocusedActor = Actor;
    }
    if (ACBInteractableDoor* Door = Cast<ACBInteractableDoor>(Actor))
        Door->SetInteractionFocus(true, Controller);
    else if (ACBKeyPickup* Key = Cast<ACBKeyPickup>(Actor))
        Key->SetInteractionFocus(true, Controller);
}

void UCBInteractionComponent::HandleInteract()
{
    APlayerController* Controller = GetLocalController();
    if (!IsInteractionAllowed() || !Controller || Controller != ViewingController.Get()
        || BoundInput.Get() != GetOwner()->InputComponent.Get()) return;

    FVector ViewLocation;
    FRotator ViewRotation;
    Controller->GetPlayerViewPoint(ViewLocation, ViewRotation);
    // Input runs before the focus tick. Recheck range, wall occlusion and viewer this frame.
    AActor* Target = TraceInteractableInView(GetWorld(), ViewLocation, ViewRotation.Vector(),
        MaxInteractionDistance, GetOwner());
    SetFocus(Target, Controller);
    if (ACBInteractableDoor* Door = Cast<ACBInteractableDoor>(Target))
        Door->TryInteract(this, Controller);
    else if (ACBKeyPickup* Key = Cast<ACBKeyPickup>(Target))
    {
        ClearFocus();
        Key->Collect(this);
    }
}

void UCBInteractionComponent::HandleRestart()
{
    if (!GetLocalController()) return;
    const FString LevelName = UGameplayStatics::GetCurrentLevelName(this, true);
    if (!LevelName.IsEmpty()) UGameplayStatics::OpenLevel(this, FName(*LevelName));
}

void UCBInteractionComponent::CompleteDemo()
{
    if (!GetLocalController()) return;
    if (ACBDemoGameMode* Rules = GetWorld() ? GetWorld()->GetAuthGameMode<ACBDemoGameMode>() : nullptr)
        Rules->TryFinishRun(ECBRunState::Won);
}

void UCBInteractionComponent::NotifyPlayer(const FText& Message)
{
    if (!IsInteractionAllowed()) return;
    PlayerMessage = Message;
    MessageExpiresAt = GetWorld() ? GetWorld()->GetTimeSeconds() + 3.0 : 0.0;
}

FText UCBInteractionComponent::GetPlayerMessage() const
{
    return IsInteractionAllowed() && GetWorld() && GetWorld()->GetTimeSeconds() < MessageExpiresAt
        ? PlayerMessage : FText::GetEmpty();
}

void UCBInteractionComponent::RemoveInputBinding()
{
    if (UEnhancedInputComponent* Input = BoundInput.Get())
    {
        if (BoundAction.IsValid()) Input->RemoveBindingByHandle(InteractionBindingHandle);
        if (BoundRestartAction.IsValid()) Input->RemoveBindingByHandle(RestartBindingHandle);
    }
    BoundInput.Reset();
    BoundAction.Reset();
    BoundRestartAction.Reset();
    InteractionBindingHandle = 0;
    RestartBindingHandle = 0;
}

void UCBInteractionComponent::ClearFocus()
{
    if (ACBInteractableDoor* Door = Cast<ACBInteractableDoor>(FocusedActor.Get()))
        Door->SetInteractionFocus(false, ViewingController.Get());
    else if (ACBKeyPickup* Key = Cast<ACBKeyPickup>(FocusedActor.Get()))
        Key->SetInteractionFocus(false, ViewingController.Get());
    FocusedActor.Reset();
}

void UCBInteractionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (ACBDemoGameMode* Rules = BoundGameMode.Get())
        Rules->OnRunStateChanged.RemoveDynamic(this, &UCBInteractionComponent::HandleRunStateChanged);
    BoundGameMode.Reset();
    ClearFocus();
    RemoveInputBinding();
    ViewingController.Reset();
    Super::EndPlay(EndPlayReason);
}

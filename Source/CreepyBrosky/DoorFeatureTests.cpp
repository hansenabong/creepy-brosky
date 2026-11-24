#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#include "CBInteractableDoor.h"
#include "CBInteractionComponent.h"
#include "Components/SceneComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "Engine/World.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCBDoorMovementTest, "CreepyBrosky.Door.MovementAndReversal",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCBDoorMovementTest::RunTest(const FString& Parameters)
{
    UWorld* World = FAutomationEditorCommonUtils::CreateNewMap();
    ACBInteractableDoor* Door = World->SpawnActor<ACBInteractableDoor>();
    if (!TestNotNull(TEXT("Door spawned"),Door)) return false;
    Door->bOpenAwayFromPlayer = false;
    TestFalse(TEXT("Starts closed"),Door->IsDoorOpen());
    Door->ToggleDoor();
    Door->Tick(0.4f);
    TestTrue(TEXT("Opening is animated, not instant"),Door->GetOpenFraction()>0.0f && Door->GetOpenFraction()<1.0f);
    const float PartlyOpen=Door->GetOpenFraction();
    Door->ToggleDoor();
    Door->Tick(0.1f);
    TestTrue(TEXT("Second press reverses smoothly"),Door->GetOpenFraction()<PartlyOpen && Door->GetOpenFraction()>0.0f);
    Door->Tick(1.0f);
    TestEqual(TEXT("Fully closes"),Door->GetOpenFraction(),0.0f);
    TestTrue(TEXT("Hinge returns to zero"),Door->Hinge->GetRelativeRotation().IsNearlyZero());
    Door->SetDoorOpen(true);
    Door->Tick(1.0f);
    TestEqual(TEXT("Fully opens"),Door->GetOpenFraction(),1.0f);
    TestTrue(TEXT("Hinge reaches configured angle"),FMath::IsNearlyEqual(Door->Hinge->GetRelativeRotation().Yaw,Door->OpenAngle));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCBDoorTargetTest, "CreepyBrosky.Door.RangeAndOcclusion",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCBDoorTargetTest::RunTest(const FString& Parameters)
{
    UWorld* World = FAutomationEditorCommonUtils::CreateNewMap();
    ACBInteractableDoor* Door = World->SpawnActor<ACBInteractableDoor>();
    const FVector Eye(-200,0,150);
    const FVector Forward(1,0,0);
    TestTrue(TEXT("Nearby aimed door can be targeted"),UCBInteractionComponent::TraceDoorInView(World,Eye,Forward,250,nullptr)==Door);
    TestNull(TEXT("Door beyond range cannot be targeted"),UCBInteractionComponent::TraceDoorInView(World,Eye,Forward,100,nullptr));
    TestNull(TEXT("Looking away clears target"),UCBInteractionComponent::TraceDoorInView(World,Eye,-Forward,250,nullptr));
    AActor* Wall = World->SpawnActor<AActor>();
    UBoxComponent* WallBox = NewObject<UBoxComponent>(Wall);
    Wall->SetRootComponent(WallBox);
    WallBox->SetBoxExtent(FVector(10,150,150));
    WallBox->SetCollisionProfileName(TEXT("BlockAll"));
    WallBox->RegisterComponent();
    Wall->SetActorLocation(FVector(-100,0,150));
    TestNull(TEXT("Wall blocks interaction with door behind it"),UCBInteractionComponent::TraceDoorInView(World,Eye,Forward,250,nullptr));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCBDoorSafetyTest, "CreepyBrosky.Door.ClosingObstruction",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCBDoorSafetyTest::RunTest(const FString& Parameters)
{
    UWorld* World=FAutomationEditorCommonUtils::CreateNewMap();
    ACBInteractableDoor* Door=World->SpawnActor<ACBInteractableDoor>();
    Door->SetDoorOpen(true);
    Door->Tick(1.0f);
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    ACharacter* Pawn=World->SpawnActor<ACharacter>(ACharacter::StaticClass(),FVector(0,0,96),FRotator::ZeroRotator,SpawnParams);
    if (!TestNotNull(TEXT("Blocking pawn spawned"),Pawn)) return false;
    Door->SetDoorOpen(false);
    Door->Tick(1.0f);
    TestTrue(TEXT("Closing obstruction requests reopening"),Door->IsDoorOpen());
    TestEqual(TEXT("Panel never snaps through pawn"),Door->GetOpenFraction(),1.0f);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCBDoorArcSafetyTest, "CreepyBrosky.Door.ClosingArcObstruction",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCBDoorArcSafetyTest::RunTest(const FString& Parameters)
{
    UWorld* World=FAutomationEditorCommonUtils::CreateNewMap();
    ACBInteractableDoor* Door=World->SpawnActor<ACBInteractableDoor>();
    Door->bOpenAwayFromPlayer=false;
    Door->SetDoorOpen(true);
    Door->Tick(1.0f);
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    World->SpawnActor<ACharacter>(ACharacter::StaticClass(),FVector(-75,10,96),FRotator::ZeroRotator,SpawnParams);
    Door->SetDoorOpen(false);
    Door->Tick(1.0f);
    TestTrue(TEXT("Pawn inside arc is protected even during a one-second hitch"),Door->IsDoorOpen());
    TestEqual(TEXT("Door does not teleport through intermediate obstruction"),Door->GetOpenFraction(),1.0f);
    return true;
}
#endif

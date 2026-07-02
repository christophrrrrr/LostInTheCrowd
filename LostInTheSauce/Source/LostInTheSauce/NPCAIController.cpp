#include "NPCAIController.h"

#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"
#include "TimerManager.h"

void ANPCAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// Stagger the first move so 50 NPCs don't all path on the same frame.
	GetWorldTimerManager().SetTimer(IdleTimerHandle, this, &ANPCAIController::PickNewDestination,
		FMath::FRandRange(0.2f, 1.5f), false);
}

void ANPCAIController::OnUnPossess()
{
	GetWorldTimerManager().ClearTimer(IdleTimerHandle);
	Super::OnUnPossess();
}

void ANPCAIController::SetWanderEnabled(bool bEnabled)
{
	bWanderEnabled = bEnabled;
	if (!bEnabled)
	{
		GetWorldTimerManager().ClearTimer(IdleTimerHandle);
		StopMovement();
	}
	else
	{
		PickNewDestination();
	}
}

void ANPCAIController::PickNewDestination()
{
	if (!bWanderEnabled || !GetPawn())
	{
		return;
	}

	UNavigationSystemV1* NavSystem = UNavigationSystemV1::GetCurrent(GetWorld());
	FNavLocation Destination;
	if (NavSystem && NavSystem->GetRandomReachablePointInRadius(GetPawn()->GetActorLocation(), WanderRadius, Destination))
	{
		MoveToLocation(Destination.Location, 40.f);
	}
	else
	{
		// No navmesh (yet) — try again shortly instead of spamming every frame.
		GetWorldTimerManager().SetTimer(IdleTimerHandle, this, &ANPCAIController::PickNewDestination, 2.f, false);
	}
}

void ANPCAIController::OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
	Super::OnMoveCompleted(RequestID, Result);

	if (!bWanderEnabled)
	{
		return;
	}

	// Idle a little: browse a stall, watch a juggler, contemplate turnips.
	GetWorldTimerManager().SetTimer(IdleTimerHandle, this, &ANPCAIController::PickNewDestination,
		FMath::FRandRange(0.5f, 4.f), false);
}

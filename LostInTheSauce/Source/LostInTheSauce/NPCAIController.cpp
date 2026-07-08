#include "NPCAIController.h"

#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"
#include "TimerManager.h"

void ANPCAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// Stagger the first move so 100 NPCs don't all path on the same frame.
	GetWorldTimerManager().SetTimer(IdleTimerHandle, this, &ANPCAIController::PickNewDestination,
		FMath::FRandRange(0.2f, 1.5f), false);

	// Cheap stuck-detection (no crowd/navmesh cost): if an agent is trying to
	// move but hasn't made progress, it's jammed at a chokepoint — repath.
	GetWorldTimerManager().SetTimer(StuckTimerHandle, this, &ANPCAIController::CheckStuck,
		FMath::FRandRange(1.5f, 2.0f), true);
}

void ANPCAIController::OnUnPossess()
{
	GetWorldTimerManager().ClearTimer(IdleTimerHandle);
	GetWorldTimerManager().ClearTimer(StuckTimerHandle);
	Super::OnUnPossess();
}

void ANPCAIController::CheckStuck()
{
	if (!bWanderEnabled || !bMovingToDestination || !GetPawn())
	{
		return;
	}
	// Made less than ~35cm of progress since the last check while supposedly
	// walking → jammed. Abort and pick a fresh destination (often away from
	// the clogged doorway).
	const FVector Now = GetPawn()->GetActorLocation();
	if (FVector::DistSquared2D(Now, LastStuckCheckPos) < FMath::Square(35.f))
	{
		StopMovement();
		bMovingToDestination = false;
		PickNewDestination();
	}
	LastStuckCheckPos = Now;
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
	if (!bWanderEnabled)
	{
		return;
	}
	if (!GetPawn())
	{
		// Not possessed yet (spawn race) — never give up, just retry.
		GetWorldTimerManager().SetTimer(IdleTimerHandle, this, &ANPCAIController::PickNewDestination, 2.f, false);
		return;
	}

	// One-time diagnostics so a broken navmesh is obvious in the log.
	static bool bLoggedNavStatus = false;

	UNavigationSystemV1* NavSystem = UNavigationSystemV1::GetCurrent(GetWorld());
	FNavLocation Destination;
	bool bFound = NavSystem &&
		NavSystem->GetRandomReachablePointInRadius(GetPawn()->GetActorLocation(), WanderRadius, Destination);
	if (!bFound && NavSystem)
	{
		// Pawn may be standing off-mesh (fallback spawn) — route back toward
		// the market center instead of retrying from a dead spot forever.
		bFound = NavSystem->GetRandomReachablePointInRadius(FVector::ZeroVector, 1800.f, Destination);
	}

	if (bFound)
	{
		if (!bLoggedNavStatus)
		{
			bLoggedNavStatus = true;
			UE_LOG(LogTemp, Log, TEXT("LITS: navmesh OK, NPCs wandering"));
		}
		MoveToLocation(Destination.Location, 40.f);
		bMovingToDestination = true;
		LastStuckCheckPos = GetPawn()->GetActorLocation();
	}
	else
	{
		if (!bLoggedNavStatus)
		{
			bLoggedNavStatus = true;
			UE_LOG(LogTemp, Warning, TEXT("LITS: no reachable navmesh point found - NPCs will stand still"));
		}
		// No navmesh (yet) — try again shortly instead of spamming every frame.
		GetWorldTimerManager().SetTimer(IdleTimerHandle, this, &ANPCAIController::PickNewDestination, 2.f, false);
	}
}

void ANPCAIController::OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
	Super::OnMoveCompleted(RequestID, Result);

	bMovingToDestination = false;
	if (!bWanderEnabled)
	{
		return;
	}

	// Idle a little: browse a stall, watch a juggler, contemplate turnips.
	GetWorldTimerManager().SetTimer(IdleTimerHandle, this, &ANPCAIController::PickNewDestination,
		FMath::FRandRange(0.5f, 4.f), false);
}

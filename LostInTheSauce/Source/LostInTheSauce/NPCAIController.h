#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "NPCAIController.generated.h"

// Dead-simple wander brain: walk to a random reachable point, idle a moment,
// repeat. No behavior trees needed for a market crowd.
UCLASS()
class LOSTINTHESAUCE_API ANPCAIController : public AAIController
{
	GENERATED_BODY()

public:
	void SetWanderEnabled(bool bEnabled);

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result) override;

private:
	void PickNewDestination();
	void CheckStuck();

	UPROPERTY(EditAnywhere, Category = "Wander")
	float WanderRadius = 1200.f;

	bool bWanderEnabled = true;
	bool bMovingToDestination = false;
	FVector LastStuckCheckPos = FVector::ZeroVector;
	FTimerHandle IdleTimerHandle;
	FTimerHandle StuckTimerHandle;
};

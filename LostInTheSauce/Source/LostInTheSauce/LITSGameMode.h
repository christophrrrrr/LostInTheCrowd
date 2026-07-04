#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "NPCTypes.h"
#include "LITSGameMode.generated.h"

class ANPCCharacter;

// Round flow: pick a target type, spawn the crowd with exactly ONE NPC of the
// target type, wait for the player to click them. R restarts after a win.
UCLASS()
class LOSTINTHESAUCE_API ALITSGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ALITSGameMode();

	virtual void BeginPlay() override;

	void HandleNPCClicked(ANPCCharacter* NPC);
	void RequestRestart();

	ENPCType GetTargetType() const { return TargetType; }
	bool IsRoundWon() const { return bRoundWon; }
	float GetLastWrongClickTime() const { return LastWrongClickTime; }
	int32 GetRoundNumber() const { return RoundNumber; }

	// 0 = every type wears its signature colors, 1 = whole crowd converges
	// into the same muted palette. Scales with the round number.
	float GetColorSimilarity() const;
	int32 GetCrowdSize() const;

	UPROPERTY(EditAnywhere, Category = "Round")
	int32 BaseNPCCount = 100;

	UPROPERTY(EditAnywhere, Category = "Round")
	int32 MaxNPCCount = 140;

	UPROPERTY(EditAnywhere, Category = "Round")
	float SpawnRadius = 1900.f;

protected:
	void StartRound();
	void ClearRound();
	bool FindSpawnPoint(FVector& OutLocation) const;
	void DumpDiagnostics();

	ENPCType TargetType = ENPCType::Farmer;
	bool bRoundWon = false;
	float LastWrongClickTime = -1000.f;
	int32 RoundNumber = 1;
	FTimerHandle AutoShotTimerHandle;

	UPROPERTY()
	TArray<TObjectPtr<ANPCCharacter>> SpawnedNPCs;
};

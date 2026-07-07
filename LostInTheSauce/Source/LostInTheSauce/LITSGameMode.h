#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "NPCTypes.h"
#include "LITSGameMode.generated.h"

class ANPCCharacter;

UENUM()
enum class ERoundFlow : uint8
{
	Menu,       // start screen over the live market
	Transition, // fade + batched despawn/spawn behind the curtain
	Playing,
	Won
};

// Round flow: pick a target type, spawn the crowd with exactly ONE NPC of the
// target type, wait for the player to click them. R starts the next round via
// a fade transition that hides the (batched) crowd swap.
UCLASS()
class LOSTINTHESAUCE_API ALITSGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ALITSGameMode();

	virtual void BeginPlay() override;

	void HandleNPCClicked(ANPCCharacter* NPC);
	void RequestRestart();

	// Start screen handoff: round 1 already simmers behind the menu.
	void StartFromMenu();
	bool IsMenuPending() const { return bMenuPending; }
	bool IsPortraitMode() const { return bPortraitMode; }

	// Pause-menu actions.
	void RestartFromRoundOne();
	void ReturnToTitle();

	void PlayGameSound(const TCHAR* AssetPath) const;

	// Win-screen stats.
	int32 GetWrongGuesses() const { return WrongGuesses; }
	float GetRoundElapsedSeconds() const;

	ENPCType GetTargetType() const { return TargetType; }
	ERoundFlow GetFlow() const { return Flow; }
	bool IsRoundWon() const { return Flow == ERoundFlow::Won; }
	bool IsPlaying() const { return Flow == ERoundFlow::Playing; }
	float GetLastWrongClickTime() const { return LastWrongClickTime; }
	int32 GetRoundNumber() const { return RoundNumber; }
	float GetTransitionStartTime() const { return TransitionStartTime; }
	float GetTransitionEndTime() const { return TransitionEndTime; }

	// 0 = every type wears its signature colors, 1 = whole crowd converges
	// into the same muted palette. Scales with the round number.
	float GetColorSimilarity() const;
	int32 GetCrowdSize() const;


	UPROPERTY(EditAnywhere, Category = "Round")
	int32 BaseNPCCount = 100;

	UPROPERTY(EditAnywhere, Category = "Round")
	int32 MaxNPCCount = 140;

	UPROPERTY(EditAnywhere, Category = "Round")
	float SpawnRadius = 3000.f;

	// Minimum desired spacing between spawned NPCs (best-effort).
	UPROPERTY(EditAnywhere, Category = "Round")
	float MinSpawnSpacing = 220.f;

	// How many NPCs get destroyed/spawned per batch tick during a
	// transition. Keeps the per-frame cost flat instead of one 100-actor
	// hitch when a round restarts.
	UPROPERTY(EditAnywhere, Category = "Round")
	int32 BatchSize = 12;

	UPROPERTY(EditAnywhere, Category = "Round")
	float TransitionMinSeconds = 1.6f;

protected:
	void StartRoundTransition();
	void TransitionBatchTick();
	void SpawnOneNPC(ENPCType Type);
	bool FindSpawnPoint(FVector& OutLocation) const;
	void DumpDiagnostics();
	void SetupPortraitCapture(const FString& TypeName);
	void StartAmbientAudio();

	ENPCType TargetType = ENPCType::Farmer;
	ERoundFlow Flow = ERoundFlow::Transition;
	bool bMenuPending = true;
	bool bPortraitMode = false;
	float LastWrongClickTime = -1000.f;
	int32 RoundNumber = 1;
	int32 WrongGuesses = 0;
	float RoundStartTime = 0.f;
	float RoundWonTime = 0.f;
	float TransitionStartTime = 0.f;
	float TransitionEndTime = -1000.f;

	UPROPERTY()
	TObjectPtr<class UAudioComponent> MusicComponent;

	UPROPERTY()
	TObjectPtr<class UAudioComponent> CrowdComponent;

	TArray<ENPCType> PendingSpawnTypes;

	UPROPERTY()
	TArray<TObjectPtr<ANPCCharacter>> PendingDestroy;

	UPROPERTY()
	TArray<TObjectPtr<ANPCCharacter>> SpawnedNPCs;

	FTimerHandle BatchTimerHandle;
	FTimerHandle AutoShotTimerHandle;
};

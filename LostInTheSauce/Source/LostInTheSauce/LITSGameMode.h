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
	Won,        // endless: target found, wait for R
	Revealing,  // time attack: timer out, pulsing red markers on the missed
	Results     // time attack: results screen
};

UENUM()
enum class EGameMode : uint8
{
	Endless,    // one target, crowd grows each round, forever
	TimeAttack  // find all 10 targets before the timer; each clear cuts 10s
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
	virtual void Tick(float DeltaSeconds) override;

	void HandleNPCClicked(ANPCCharacter* NPC);
	void RequestRestart();

	// Start screen handoff: the player picks a mode, round 1 begins.
	void StartMode(EGameMode Mode);
	bool IsMenuPending() const { return bMenuPending; }
	bool IsPortraitMode() const { return bPortraitMode; }

	// Pause / results actions.
	void RestartFromRoundOne();
	void ReturnToTitle();

	void PlayGameSound(const TCHAR* AssetPath) const;

	EGameMode GetGameMode() const { return CurrentMode; }
	bool IsTimeAttack() const { return CurrentMode == EGameMode::TimeAttack; }

	// Win-screen stats.
	int32 GetWrongGuesses() const { return WrongGuesses; }
	float GetRoundElapsedSeconds() const;

	// Time Attack state (for the HUD / results).
	float GetTimeRemaining() const;
	int32 GetTargetsFound() const { return TargetsFound; }
	int32 GetTargetCount() const { return TATargetCount; }
	int32 GetRoundsCleared() const { return RoundNumber - 1; }
	int32 GetTotalFound() const { return TotalFound; }
	float GetRevealStartTime() const { return RevealStartTime; }
	// Enumerate the still-missed target NPCs for the reveal markers.
	void GetMissedTargets(TArray<class ANPCCharacter*>& Out) const;

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

	// --- Time Attack tuning ---
	UPROPERTY(EditAnywhere, Category = "TimeAttack")
	int32 TACrowdSize = 100;

	UPROPERTY(EditAnywhere, Category = "TimeAttack")
	int32 TATargetCount = 10;

	UPROPERTY(EditAnywhere, Category = "TimeAttack")
	float TABaseTime = 120.f;

	UPROPERTY(EditAnywhere, Category = "TimeAttack")
	float TATimeStep = 10.f;

	UPROPERTY(EditAnywhere, Category = "TimeAttack")
	float TAMinTime = 30.f;

	UPROPERTY(EditAnywhere, Category = "TimeAttack")
	float RevealSeconds = 10.f;

protected:
	void StartRoundTransition();
	void TransitionBatchTick();
	void SpawnOneNPC(ENPCType Type);
	bool FindSpawnPoint(FVector& OutLocation) const;
	void DumpDiagnostics();
	void SetupPortraitCapture(const FString& TypeName);
	void StartAmbientAudio();
	void OnRoundReady();
	void EnterReveal();

	EGameMode CurrentMode = EGameMode::Endless;
	ENPCType TargetType = ENPCType::Farmer;
	ERoundFlow Flow = ERoundFlow::Transition;
	bool bMenuPending = true;
	bool bPortraitMode = false;
	float LastWrongClickTime = -1000.f;
	int32 RoundNumber = 1;
	int32 WrongGuesses = 0;
	int32 TargetsFound = 0;
	int32 TotalFound = 0;
	float CurrentTimeLimit = 120.f;
	float RevealStartTime = 0.f;
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

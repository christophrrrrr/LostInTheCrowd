#include "LITSGameMode.h"

#include "Engine/World.h"
#include "LITSHUD.h"
#include "LITSPlayerController.h"
#include "NPCCharacter.h"
#include "NavigationSystem.h"
#include "OrbitCameraPawn.h"

ALITSGameMode::ALITSGameMode()
{
	DefaultPawnClass = AOrbitCameraPawn::StaticClass();
	PlayerControllerClass = ALITSPlayerController::StaticClass();
	HUDClass = ALITSHUD::StaticClass();
}

void ALITSGameMode::BeginPlay()
{
	Super::BeginPlay();
	StartRound();
}

void ALITSGameMode::StartRound()
{
	ClearRound();

	bRoundWon = false;
	TargetType = static_cast<ENPCType>(FMath::RandRange(0, NPCTypeCount - 1));

	for (int32 i = 0; i < NPCCount; ++i)
	{
		// Exactly one NPC of the target type per round: the first one spawned.
		ENPCType Type;
		if (i == 0)
		{
			Type = TargetType;
		}
		else
		{
			int32 Pick = FMath::RandRange(0, NPCTypeCount - 2);
			if (Pick >= static_cast<int32>(TargetType))
			{
				++Pick;
			}
			Type = static_cast<ENPCType>(Pick);
		}

		FVector SpawnLocation;
		if (!FindSpawnPoint(SpawnLocation))
		{
			continue;
		}

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		ANPCCharacter* NPC = GetWorld()->SpawnActor<ANPCCharacter>(
			ANPCCharacter::StaticClass(),
			SpawnLocation + FVector(0.f, 0.f, 92.f),
			FRotator(0.f, FMath::FRandRange(0.f, 360.f), 0.f),
			Params);

		if (NPC)
		{
			NPC->SetNPCType(Type);
			SpawnedNPCs.Add(NPC);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("Round started: find the %s among %d NPCs"),
		NPCTypeStyles::Get(TargetType).DisplayName, SpawnedNPCs.Num());
}

void ALITSGameMode::ClearRound()
{
	for (ANPCCharacter* NPC : SpawnedNPCs)
	{
		if (IsValid(NPC))
		{
			NPC->Destroy();
		}
	}
	SpawnedNPCs.Empty();
}

bool ALITSGameMode::FindSpawnPoint(FVector& OutLocation) const
{
	UNavigationSystemV1* NavSystem = UNavigationSystemV1::GetCurrent(GetWorld());
	FNavLocation NavLocation;
	if (NavSystem && NavSystem->GetRandomReachablePointInRadius(FVector::ZeroVector, SpawnRadius, NavLocation))
	{
		OutLocation = NavLocation.Location;
		return true;
	}

	// Navmesh missing: scatter on the flat floor so the round still works.
	OutLocation = FVector(FMath::FRandRange(-SpawnRadius, SpawnRadius),
		FMath::FRandRange(-SpawnRadius, SpawnRadius), 0.f);
	return true;
}

void ALITSGameMode::HandleNPCClicked(ANPCCharacter* NPC)
{
	if (bRoundWon || !NPC)
	{
		return;
	}

	if (NPC->GetNPCType() == TargetType)
	{
		bRoundWon = true;
		NPC->CelebrateFound();
	}
	else
	{
		LastWrongClickTime = GetWorld()->GetTimeSeconds();
		NPC->ReactToWrongClick();
	}
}

void ALITSGameMode::RequestRestart()
{
	if (bRoundWon)
	{
		StartRound();
	}
}

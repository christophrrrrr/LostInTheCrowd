#include "LITSGameMode.h"

#include "Camera/CameraActor.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "NavMesh/RecastNavMesh.h"
#include "LITSHUD.h"
#include "LITSPlayerController.h"
#include "Misc/CommandLine.h"
#include "NPCCharacter.h"
#include "NavigationSystem.h"
#include "OrbitCameraPawn.h"
#include "TimerManager.h"
#include "UnrealClient.h"

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

	// Dev loop: -LITSAutoShot lets automation capture a rendered frame plus a
	// diagnostics dump and exit, so visual changes can be verified without a
	// human at the wheel.
	if (FParse::Param(FCommandLine::Get(), TEXT("LITSAutoShot")))
	{
		FTimerHandle DiagHandle;
		GetWorldTimerManager().SetTimer(DiagHandle, this, &ALITSGameMode::DumpDiagnostics, 6.f, false);
		FTimerHandle DiagHandle2;
		GetWorldTimerManager().SetTimer(DiagHandle2, this, &ALITSGameMode::DumpDiagnostics, 10.5f, false);

		// Jump the camera right next to an NPC before the shot so orientation,
		// animation pose, and materials are readable in the capture.
		FTimerHandle CloseupHandle;
		GetWorldTimerManager().SetTimer(CloseupHandle, [this]()
		{
			if (SpawnedNPCs.Num() > 0 && IsValid(SpawnedNPCs[0]))
			{
				const FVector Target = SpawnedNPCs[0]->GetActorLocation();
				const FVector CamLoc = Target + FVector(-260.f, 0.f, 130.f);
				ACameraActor* Cam = GetWorld()->SpawnActor<ACameraActor>(CamLoc, (Target - CamLoc).Rotation());
				if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
				{
					PC->SetViewTargetWithBlend(Cam, 0.f);
				}
			}
		}, 7.5f, false);

		GetWorldTimerManager().SetTimer(AutoShotTimerHandle, []()
		{
			FScreenshotRequest::RequestScreenshot(TEXT("LITSAutoShot"), false, false);
		}, 8.f, false);

		FTimerHandle QuitHandle;
		GetWorldTimerManager().SetTimer(QuitHandle, []()
		{
			FGenericPlatformMisc::RequestExit(false);
		}, 12.f, false);
	}
}

void ALITSGameMode::StartRound()
{
	ClearRound();

	bRoundWon = false;
	TargetType = static_cast<ENPCType>(FMath::RandRange(0, NPCTypeCount - 1));

	// Dev: -LITSForceTarget=<DisplayName> pins the round target (and thereby
	// which character the AutoShot close-up camera inspects).
	FString ForcedTarget;
	if (FParse::Value(FCommandLine::Get(), TEXT("LITSForceTarget="), ForcedTarget))
	{
		for (int32 i = 0; i < NPCTypeCount; ++i)
		{
			if (ForcedTarget.Equals(NPCTypeStyles::Get(static_cast<ENPCType>(i)).DisplayName, ESearchCase::IgnoreCase))
			{
				TargetType = static_cast<ENPCType>(i);
				break;
			}
		}
	}

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

void ALITSGameMode::DumpDiagnostics()
{
	UE_LOG(LogTemp, Log, TEXT("LITS-DIAG ======================================"));

	if (const APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		if (PC->PlayerCameraManager)
		{
			UE_LOG(LogTemp, Log, TEXT("LITS-DIAG camera loc=%s rot=%s"),
				*PC->PlayerCameraManager->GetCameraLocation().ToCompactString(),
				*PC->PlayerCameraManager->GetCameraRotation().ToCompactString());
		}
		if (const APawn* Pawn = PC->GetPawn())
		{
			UE_LOG(LogTemp, Log, TEXT("LITS-DIAG pawn loc=%s rot=%s"),
				*Pawn->GetActorLocation().ToCompactString(), *Pawn->GetActorRotation().ToCompactString());
		}
	}

	for (TActorIterator<ANavMeshBoundsVolume> It(GetWorld()); It; ++It)
	{
		const FBox Box = It->GetComponentsBoundingBox(true);
		UE_LOG(LogTemp, Log, TEXT("LITS-DIAG nav volume bounds size=%s"), *Box.GetSize().ToCompactString());
	}

	for (TActorIterator<ARecastNavMesh> It(GetWorld()); It; ++It)
	{
		UE_LOG(LogTemp, Log, TEXT("LITS-DIAG RecastNavMesh %s runtimeGen=%d tiles=%d"),
			*It->GetName(), static_cast<int32>(It->GetRuntimeGenerationMode()),
			It->GetNavMeshTilesCount());
	}

	UNavigationSystemV1* NavSystem = UNavigationSystemV1::GetCurrent(GetWorld());
	FNavLocation Probe;
	const bool bNavOK = NavSystem && NavSystem->GetRandomReachablePointInRadius(FVector::ZeroVector, 1500.f, Probe);
	UE_LOG(LogTemp, Log, TEXT("LITS-DIAG nav probe at origin: %s"), bNavOK ? TEXT("OK") : TEXT("FAILED"));

	int32 Moving = 0;
	for (const ANPCCharacter* NPC : SpawnedNPCs)
	{
		if (IsValid(NPC) && NPC->GetVelocity().Size2D() > 10.f)
		{
			++Moving;
		}
	}
	UE_LOG(LogTemp, Log, TEXT("LITS-DIAG NPCs: %d spawned, %d moving"), SpawnedNPCs.Num(), Moving);

	if (SpawnedNPCs.Num() > 0 && IsValid(SpawnedNPCs[0]))
	{
		const ANPCCharacter* NPC = SpawnedNPCs[0];
		const FBoxSphereBounds MeshBounds = NPC->GetMesh()->Bounds;
		UE_LOG(LogTemp, Log, TEXT("LITS-DIAG NPC[0] (%s) loc=%s meshScale=%s worldMeshHeight=%.1fcm"),
			NPCTypeStyles::Get(NPC->GetNPCType()).DisplayName,
			*NPC->GetActorLocation().ToCompactString(),
			*NPC->GetMesh()->GetRelativeScale3D().ToCompactString(),
			MeshBounds.BoxExtent.Z * 2.f);

		// Component-space rotations of the first bones reveal how the anim
		// data orients the rig, so the mesh correction can be computed
		// instead of guessed.
		USkeletalMeshComponent* Mesh = NPC->GetMesh();
		UE_LOG(LogTemp, Log, TEXT("LITS-DIAG mesh relRot=%s worldRot=%s actorRot=%s"),
			*Mesh->GetRelativeRotation().ToCompactString(),
			*Mesh->GetComponentRotation().ToCompactString(),
			*NPC->GetActorRotation().ToCompactString());
		for (int32 BoneIndex = 0; BoneIndex < FMath::Min(3, Mesh->GetNumBones()); ++BoneIndex)
		{
			const FName BoneName = Mesh->GetBoneName(BoneIndex);
			const FQuat CompQuat = Mesh->GetBoneQuaternion(BoneName, EBoneSpaces::ComponentSpace);
			const FQuat WorldQuat = Mesh->GetBoneQuaternion(BoneName, EBoneSpaces::WorldSpace);
			UE_LOG(LogTemp, Log, TEXT("LITS-DIAG bone[%d] %s comp-rot=%s world-rot=%s"),
				BoneIndex, *BoneName.ToString(), *CompQuat.Rotator().ToCompactString(),
				*WorldQuat.Rotator().ToCompactString());
		}
	}

	UE_LOG(LogTemp, Log, TEXT("LITS-DIAG ======================================"));
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

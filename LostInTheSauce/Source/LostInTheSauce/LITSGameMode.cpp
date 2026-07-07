#include "LITSGameMode.h"

#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/PointLight.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Sound/SoundBase.h"
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

	// Automation runs skip the start screen (constructor so the flag is set
	// before any actor's BeginPlay ordering can race it). -LITSKeepMenu
	// keeps it for menu screenshots.
	if (FParse::Param(FCommandLine::Get(), TEXT("LITSAutoShot")) &&
		!FParse::Param(FCommandLine::Get(), TEXT("LITSKeepMenu")))
	{
		bMenuPending = false;
	}
}

void ALITSGameMode::BeginPlay()
{
	Super::BeginPlay();

	// Portrait factory: -LITSPortrait=<Type> sets up a lone, front-lit,
	// camera-facing character on a studio backdrop, screenshots it, exits.
	FString PortraitType;
	if (FParse::Value(FCommandLine::Get(), TEXT("LITSPortrait="), PortraitType))
	{
		SetupPortraitCapture(PortraitType);
		GetWorldTimerManager().SetTimer(AutoShotTimerHandle, []()
		{
			FScreenshotRequest::RequestScreenshot(TEXT("LITSAutoShot"), false, false);
		}, 2.5f, false);
		FTimerHandle QuitHandle;
		GetWorldTimerManager().SetTimer(QuitHandle, []()
		{
			FGenericPlatformMisc::RequestExit(false);
		}, 4.f, false);
		return;
	}

	StartAmbientAudio();
	StartRoundTransition();

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
				// Freeze + wave so the capture reliably frames the target
				// (and exercises the celebration animation).
				SpawnedNPCs[0]->CelebrateFound();
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
			// bShowUI=true or Slate widgets (menus) are invisible in captures.
			FScreenshotRequest::RequestScreenshot(TEXT("LITSAutoShot"), true, false);
		}, 8.f, false);

		// Second capture: straight-down aerial for layout QA.
		FTimerHandle TopCamHandle;
		GetWorldTimerManager().SetTimer(TopCamHandle, [this]()
		{
			ACameraActor* TopCam = GetWorld()->SpawnActor<ACameraActor>(
				FVector(0.f, 0.f, 7200.f), FRotator(-89.f, 0.f, 0.f));
			if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
			{
				PC->SetViewTargetWithBlend(TopCam, 0.f);
			}
		}, 11.f, false);
		FTimerHandle TopShotHandle;
		GetWorldTimerManager().SetTimer(TopShotHandle, []()
		{
			FScreenshotRequest::RequestScreenshot(TEXT("LITSTopShot"), true, false);
		}, 11.5f, false);

		FTimerHandle QuitHandle;
		GetWorldTimerManager().SetTimer(QuitHandle, []()
		{
			FGenericPlatformMisc::RequestExit(false);
		}, 14.f, false);
	}
}

void ALITSGameMode::StartRoundTransition()
{
	Flow = ERoundFlow::Transition;
	TransitionStartTime = GetWorld()->GetTimeSeconds();

	// Old crowd goes into the destroy queue; both queues drain in batches.
	PendingDestroy.Append(SpawnedNPCs);
	SpawnedNPCs.Empty();

	WrongGuesses = 0;
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

	// Exactly one NPC of the target type per round. The batch ticker pops
	// from the END of this array, so the target goes last = spawns first
	// (SpawnedNPCs[0] must be the target for the AutoShot close-up).
	PendingSpawnTypes.Reset();
	const int32 CrowdSize = GetCrowdSize();
	for (int32 i = 1; i < CrowdSize; ++i)
	{
		int32 Pick = FMath::RandRange(0, NPCTypeCount - 2);
		if (Pick >= static_cast<int32>(TargetType))
		{
			++Pick;
		}
		PendingSpawnTypes.Add(static_cast<ENPCType>(Pick));
	}
	PendingSpawnTypes.Add(TargetType);

	GetWorldTimerManager().SetTimer(BatchTimerHandle, this, &ALITSGameMode::TransitionBatchTick, 0.05f, true);
}

void ALITSGameMode::TransitionBatchTick()
{
	int32 Budget = BatchSize;

	while (Budget > 0 && PendingDestroy.Num() > 0)
	{
		ANPCCharacter* NPC = PendingDestroy.Pop();
		if (IsValid(NPC))
		{
			NPC->Destroy();
			--Budget;
		}
	}

	while (Budget > 0 && PendingSpawnTypes.Num() > 0)
	{
		SpawnOneNPC(PendingSpawnTypes.Pop());
		--Budget;
	}

	const float Elapsed = GetWorld()->GetTimeSeconds() - TransitionStartTime;
	if (PendingDestroy.Num() == 0 && PendingSpawnTypes.Num() == 0 && Elapsed >= TransitionMinSeconds)
	{
		GetWorldTimerManager().ClearTimer(BatchTimerHandle);
		// Round 1 waits on the start screen; the crowd wanders behind it.
		Flow = bMenuPending ? ERoundFlow::Menu : ERoundFlow::Playing;
		TransitionEndTime = GetWorld()->GetTimeSeconds();
		RoundStartTime = GetWorld()->GetTimeSeconds();
		UE_LOG(LogTemp, Log, TEXT("Round %d started: find the %s among %d NPCs"),
			RoundNumber, NPCTypeStyles::Get(TargetType).DisplayName, SpawnedNPCs.Num());

		// Watchdog: if navigation is somehow dead a few seconds into the
		// round (rebuild race, tile corruption — cause intermittent), force
		// a full rebuild instead of leaving the crowd frozen.
		FTimerHandle NavWatchdogHandle;
		GetWorldTimerManager().SetTimer(NavWatchdogHandle, [this]()
		{
			UNavigationSystemV1* NavSystem = UNavigationSystemV1::GetCurrent(GetWorld());
			FNavLocation Probe;
			if (NavSystem && !NavSystem->GetRandomReachablePointInRadius(FVector::ZeroVector, 1500.f, Probe))
			{
				UE_LOG(LogTemp, Warning, TEXT("LITS: nav probe dead after round start - forcing navigation rebuild"));
				NavSystem->Build();
			}
		}, 3.f, false);
	}
}

void ALITSGameMode::SetupPortraitCapture(const FString& TypeName)
{
	bMenuPending = false;
	bPortraitMode = true;
	Flow = ERoundFlow::Playing; // no transition curtain

	ENPCType Type = ENPCType::Farmer;
	for (int32 i = 0; i < NPCTypeCount; ++i)
	{
		if (TypeName.Equals(NPCTypeStyles::Get(static_cast<ENPCType>(i)).DisplayName, ESearchCase::IgnoreCase))
		{
			Type = static_cast<ENPCType>(i);
			break;
		}
	}

	// A private studio far above the map so no town geometry intrudes.
	const FVector Booth(0.f, 0.f, 20000.f);

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	// Character faces +X at actor yaw 0; the camera sits on the +X side.
	ANPCCharacter* Model = GetWorld()->SpawnActor<ANPCCharacter>(
		ANPCCharacter::StaticClass(), Booth, FRotator::ZeroRotator, Params);
	if (!Model)
	{
		return;
	}
	Model->SetNPCType(Type);
	if (AController* C = Model->GetController())
	{
		C->Destroy();
	}
	Model->GetCharacterMovement()->DisableMovement();

	// Neutral backdrop panel behind the model (-X side, behind its back).
	if (AStaticMeshActor* Panel = GetWorld()->SpawnActor<AStaticMeshActor>(
		Booth + FVector(-220.f, 0.f, 60.f), FRotator::ZeroRotator))
	{
		Panel->SetMobility(EComponentMobility::Movable);
		if (UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")))
		{
			Panel->GetStaticMeshComponent()->SetStaticMesh(Cube);
		}
		Panel->SetActorScale3D(FVector(0.2f, 6.f, 6.f));
		if (UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr,
			TEXT("/Game/LostInTheSauce/Materials/M_NPCColor")))
		{
			UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Base, this);
			MID->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.34f, 0.32f, 0.30f));
			Panel->GetStaticMeshComponent()->SetMaterial(0, MID);
		}
	}

	// Three-point studio lighting so the character is evenly bright, no
	// black side.
	auto AddLight = [this, Booth](const FVector& Offset, float Intensity)
	{
		if (APointLight* L = GetWorld()->SpawnActor<APointLight>(Booth + Offset, FRotator::ZeroRotator))
		{
			L->SetMobility(EComponentMobility::Movable);
			L->SetLightColor(FLinearColor(1.f, 0.98f, 0.95f));
			if (UPointLightComponent* LC = Cast<UPointLightComponent>(L->GetLightComponent()))
			{
				LC->SetIntensity(Intensity);
				LC->SetAttenuationRadius(2000.f);
				LC->SetCastShadows(false);
			}
		}
	};
	AddLight(FVector(260.f, 0.f, 150.f), 300000.f);   // front key
	AddLight(FVector(120.f, -260.f, 90.f), 150000.f); // left fill
	AddLight(FVector(120.f, 260.f, 90.f), 150000.f);  // right fill

	// Camera directly in front (+X), framing head-to-feet. Exposure is set on
	// the camera itself (a bounded volume's runtime bounds are unreliable) so
	// the town's dark clamp doesn't apply to this dim void.
	const FVector CamLoc = Booth + FVector(360.f, 0.f, 45.f);
	const FVector LookAt = Booth + FVector(0.f, 0.f, 15.f);
	ACameraActor* Cam = GetWorld()->SpawnActor<ACameraActor>(CamLoc, (LookAt - CamLoc).Rotation());
	if (Cam)
	{
		UCameraComponent* CamComp = Cam->GetCameraComponent();
		CamComp->SetFieldOfView(42.f);
		FPostProcessSettings& PP = CamComp->PostProcessSettings;
		PP.bOverride_AutoExposureMethod = true;
		PP.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
		PP.bOverride_AutoExposureBias = true;
		PP.AutoExposureBias = 3.5f; // manual EV; higher = brighter here
		if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
		{
			PC->SetViewTargetWithBlend(Cam, 0.f);
		}
	}
}

void ALITSGameMode::SpawnOneNPC(ENPCType Type)
{
	FVector SpawnLocation;
	if (!FindSpawnPoint(SpawnLocation))
	{
		return;
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

bool ALITSGameMode::FindSpawnPoint(FVector& OutLocation) const
{
	UNavigationSystemV1* NavSystem = UNavigationSystemV1::GetCurrent(GetWorld());
	if (NavSystem)
	{
		// Rejection-sample a few candidates and keep the one that is furthest
		// from the nearest already-spawned NPC, so the crowd starts spread
		// out across the streets instead of clumped at a few points.
		FNavLocation Best;
		float BestMinDistSq = -1.f;
		bool bFound = false;
		for (int32 Attempt = 0; Attempt < 6; ++Attempt)
		{
			FNavLocation Candidate;
			if (!NavSystem->GetRandomReachablePointInRadius(FVector::ZeroVector, SpawnRadius, Candidate))
			{
				continue;
			}
			bFound = true;
			float NearestSq = TNumericLimits<float>::Max();
			for (const ANPCCharacter* NPC : SpawnedNPCs)
			{
				if (IsValid(NPC))
				{
					NearestSq = FMath::Min(NearestSq,
						FVector::DistSquared2D(Candidate.Location, NPC->GetActorLocation()));
				}
			}
			if (NearestSq > BestMinDistSq)
			{
				BestMinDistSq = NearestSq;
				Best = Candidate;
			}
			// Good enough spacing — take it without exhausting all attempts.
			if (NearestSq > FMath::Square(MinSpawnSpacing))
			{
				break;
			}
		}
		if (bFound)
		{
			OutLocation = Best.Location;
			return true;
		}
	}

	// Navmesh missing: scatter on the flat floor so the round still works.
	OutLocation = FVector(FMath::FRandRange(-SpawnRadius, SpawnRadius),
		FMath::FRandRange(-SpawnRadius, SpawnRadius), 0.f);
	return true;
}

void ALITSGameMode::DumpDiagnostics()
{
	UE_LOG(LogTemp, Log, TEXT("LITS-DIAG ======================================"));
	extern ENGINE_API float GAverageFPS;
	UE_LOG(LogTemp, Log, TEXT("LITS-DIAG avgFPS=%.1f round=%d similarity=%.2f"),
		GAverageFPS, RoundNumber, GetColorSimilarity());

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

void ALITSGameMode::StartFromMenu()
{
	if (Flow == ERoundFlow::Menu)
	{
		bMenuPending = false;
		Flow = ERoundFlow::Playing;
		TransitionEndTime = GetWorld()->GetTimeSeconds();
		RoundStartTime = GetWorld()->GetTimeSeconds();
		PlayGameSound(TEXT("/Game/LostInTheSauce/Sounds/S_GameStart"));
	}
	else
	{
		// Menu closed before the intro transition finished — just note it;
		// the transition end will flip straight to Playing.
		bMenuPending = false;
	}
}

void ALITSGameMode::RestartFromRoundOne()
{
	RoundNumber = 1;
	StartRoundTransition();
}

void ALITSGameMode::ReturnToTitle()
{
	RoundNumber = 1;
	bMenuPending = true;
	StartRoundTransition();
}

void ALITSGameMode::StartAmbientAudio()
{
	if (USoundBase* Music = LoadObject<USoundBase>(nullptr, TEXT("/Game/LostInTheSauce/Sounds/S_Music")))
	{
		MusicComponent = UGameplayStatics::SpawnSound2D(this, Music, 0.5f, 1.f, 0.f, nullptr, true);
	}
	if (USoundBase* Crowd = LoadObject<USoundBase>(nullptr, TEXT("/Game/LostInTheSauce/Sounds/S_Crowd")))
	{
		CrowdComponent = UGameplayStatics::SpawnSound2D(this, Crowd, 0.1f, 1.f, 0.f, nullptr, true);
	}
}

float ALITSGameMode::GetRoundElapsedSeconds() const
{
	const float End = (Flow == ERoundFlow::Won) ? RoundWonTime : GetWorld()->GetTimeSeconds();
	return FMath::Max(0.f, End - RoundStartTime);
}

void ALITSGameMode::PlayGameSound(const TCHAR* AssetPath) const
{
	if (USoundBase* Sound = LoadObject<USoundBase>(nullptr, AssetPath))
	{
		UGameplayStatics::PlaySound2D(GetWorld(), Sound);
	}
}

void ALITSGameMode::HandleNPCClicked(ANPCCharacter* NPC)
{
	if (Flow != ERoundFlow::Playing || !NPC)
	{
		return;
	}

	if (NPC->GetNPCType() == TargetType)
	{
		Flow = ERoundFlow::Won;
		RoundWonTime = GetWorld()->GetTimeSeconds();
		NPC->CelebrateFound();
		PlayGameSound(TEXT("/Game/LostInTheSauce/Sounds/S_Found"));
		PlayGameSound(TEXT("/Game/LostInTheSauce/Sounds/S_Reward"));
	}
	else
	{
		++WrongGuesses;
		LastWrongClickTime = GetWorld()->GetTimeSeconds();
		NPC->ReactToWrongClick();
		PlayGameSound(TEXT("/Game/LostInTheSauce/Sounds/S_WrongGuess"));
	}
}

void ALITSGameMode::RequestRestart()
{
	if (Flow == ERoundFlow::Won)
	{
		++RoundNumber;
		PlayGameSound(TEXT("/Game/LostInTheSauce/Sounds/S_Transition"));
		StartRoundTransition();
	}
}

float ALITSGameMode::GetColorSimilarity() const
{
	// Round 1 keeps signature colors popping; by round ~9 the crowd has
	// converged into look-alike mud.
	return FMath::Clamp(0.12f + 0.085f * (RoundNumber - 1), 0.12f, 0.80f);
}

int32 ALITSGameMode::GetCrowdSize() const
{
	return FMath::Min(BaseNPCCount + (RoundNumber - 1) * 8, MaxNPCCount);
}

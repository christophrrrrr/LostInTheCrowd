#include "OrbitCameraPawn.h"

#include "Camera/CameraComponent.h"
#include "Components/SphereComponent.h"
#include "EngineUtils.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "GameFramework/PlayerController.h"

AOrbitCameraPawn::AOrbitCameraPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	RootComponent = CollisionSphere;
	CollisionSphere->InitSphereRadius(40.f);
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionSphere->SetCollisionObjectType(ECC_Pawn);
	CollisionSphere->SetCollisionResponseToAllChannels(ECR_Block);
	// Fly through the crowd, never shove it; and never eat the click-trace.
	CollisionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	CollisionSphere->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	CollisionSphere->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(CollisionSphere);

	Movement = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("Movement"));
	Movement->UpdatedComponent = CollisionSphere;
	Movement->MaxSpeed = 2400.f;
	Movement->Acceleration = 8000.f;
	Movement->Deceleration = 9000.f;

	bUseControllerRotationYaw = false;
}

void AOrbitCameraPawn::BeginPlay()
{
	Super::BeginPlay();
	Yaw = GetActorRotation().Yaw;
	SetActorRotation(FRotator(Pitch, Yaw, 0.f));
	ComputeBoundsFromWalls();
}

void AOrbitCameraPawn::ComputeBoundsFromWalls()
{
	FBox Bounds(ForceInit);
	int32 WallCount = 0;
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (It->ActorHasTag(TEXT("CamBoundary")))
		{
			Bounds += It->GetComponentsBoundingBox(true);
			++WallCount;
		}
	}
	if (WallCount > 0 && Bounds.IsValid)
	{
		const float Inset = 80.f;
		BoundsMin = FVector2D(Bounds.Min.X + Inset, Bounds.Min.Y + Inset);
		BoundsMax = FVector2D(Bounds.Max.X - Inset, Bounds.Max.Y - Inset);
		// Invisible ceiling just below the wall tops: the player can never
		// look over the boundary, and it adapts if the walls are re-sized.
		MaxHeight = FMath::Max(800.f, Bounds.Max.Z - 150.f);
	}
}

void AOrbitCameraPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		return;
	}

	// --- Look: hold right mouse, captured-mouse delta (glitch-free) --------
	if (PC->IsInputKeyDown(EKeys::RightMouseButton))
	{
		if (!bLooking)
		{
			bLooking = true;
			PC->SetInputMode(FInputModeGameOnly());
			PC->bShowMouseCursor = false;
		}
		float DeltaX = 0.f, DeltaY = 0.f;
		PC->GetInputMouseDelta(DeltaX, DeltaY);
		Yaw += DeltaX * LookSpeed;
		Pitch = FMath::Clamp(Pitch + DeltaY * LookSpeed, -85.f, 35.f);
		SetActorRotation(FRotator(Pitch, Yaw, 0.f));
	}
	else if (bLooking)
	{
		bLooking = false;
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		PC->SetInputMode(InputMode);
		PC->bShowMouseCursor = true;
	}

	// --- Move: WASD where you look, Space/Ctrl vertical, Shift sprint ------
	const FRotator LookRotation(Pitch, Yaw, 0.f);
	FVector Wish = FVector::ZeroVector;
	if (PC->IsInputKeyDown(EKeys::W)) { Wish += LookRotation.Vector(); }
	if (PC->IsInputKeyDown(EKeys::S)) { Wish -= LookRotation.Vector(); }
	const FVector Right = FRotationMatrix(FRotator(0.f, Yaw, 0.f)).GetUnitAxis(EAxis::Y);
	if (PC->IsInputKeyDown(EKeys::D)) { Wish += Right; }
	if (PC->IsInputKeyDown(EKeys::A)) { Wish -= Right; }
	if (PC->IsInputKeyDown(EKeys::SpaceBar)) { Wish += FVector::UpVector; }
	if (PC->IsInputKeyDown(EKeys::LeftControl)) { Wish -= FVector::UpVector; }

	if (!Wish.IsNearlyZero())
	{
		const float Sprint = PC->IsInputKeyDown(EKeys::LeftShift) ? SprintMultiplier : 1.f;
		AddMovementInput(Wish.GetSafeNormal(), Sprint);
	}

	// --- Belt-and-braces bounds (collision blocks; this stops tunneling) ---
	FVector Location = GetActorLocation();
	Location.X = FMath::Clamp(Location.X, BoundsMin.X, BoundsMax.X);
	Location.Y = FMath::Clamp(Location.Y, BoundsMin.Y, BoundsMax.Y);
	Location.Z = FMath::Clamp(Location.Z, MinHeight, MaxHeight);
	if (!Location.Equals(GetActorLocation()))
	{
		SetActorLocation(Location);
	}
}

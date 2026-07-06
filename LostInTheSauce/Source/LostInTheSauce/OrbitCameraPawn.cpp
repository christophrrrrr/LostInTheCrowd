#include "OrbitCameraPawn.h"

#include "Camera/CameraComponent.h"
#include "EngineUtils.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"

AOrbitCameraPawn::AOrbitCameraPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	Pivot = CreateDefaultSubobject<USceneComponent>(TEXT("Pivot"));
	RootComponent = Pivot;

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(Pivot);
	SpringArm->TargetArmLength = TargetArmLength;
	SpringArm->SetRelativeRotation(FRotator(PitchDegrees, 0.f, 0.f));
	// Slide the camera in when a building or wall is between it and the view
	// center, so it never ends up inside geometry.
	SpringArm->bDoCollisionTest = true;
	SpringArm->ProbeSize = 22.f; // fat probe so it can't slip through thin walls
	SpringArm->ProbeChannel = ECC_Camera;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm);

	bUseControllerRotationYaw = false;
}

void AOrbitCameraPawn::BeginPlay()
{
	Super::BeginPlay();
	ComputePanBoundsFromWalls();
}

void AOrbitCameraPawn::ComputePanBoundsFromWalls()
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
		// Inset a couple of metres so the pivot stays clear of the walls.
		const float Inset = 300.f;
		PanMin = FVector2D(Bounds.Min.X + Inset, Bounds.Min.Y + Inset);
		PanMax = FVector2D(Bounds.Max.X - Inset, Bounds.Max.Y - Inset);
	}
	else
	{
		// No tagged ramparts — fall back to the symmetric limit.
		PanMin = FVector2D(-PanLimit, -PanLimit);
		PanMax = FVector2D(PanLimit, PanLimit);
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

	// Rotation: while the right mouse is held, capture the mouse (GameOnly
	// input mode hides the cursor and locks it) and read the raw device
	// delta. This is the standard, glitch-free approach — no per-frame
	// cursor warping (which desynced against zoom/pan and caused the
	// "bug out"). On release we restore the cursor for clicking NPCs.
	if (PC->IsInputKeyDown(EKeys::RightMouseButton))
	{
		if (!bDragging)
		{
			bDragging = true;
			PC->SetInputMode(FInputModeGameOnly());
			PC->bShowMouseCursor = false;
		}
		float DeltaX = 0.f, DeltaY = 0.f;
		PC->GetInputMouseDelta(DeltaX, DeltaY);
		AddActorWorldRotation(FRotator(0.f, DeltaX * RotateSpeed, 0.f));
		PitchDegrees = FMath::Clamp(PitchDegrees - DeltaY * RotateSpeed, -80.f, -32.f);
		SpringArm->SetRelativeRotation(FRotator(PitchDegrees, 0.f, 0.f));
	}
	else if (bDragging)
	{
		bDragging = false;
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		PC->SetInputMode(InputMode);
		PC->bShowMouseCursor = true;
	}

	// WASD pans camera-relative (W = away from the viewer), faster when
	// zoomed out, clamped so the pivot stays inside the market.
	FVector2D PanInput = FVector2D::ZeroVector;
	if (PC->IsInputKeyDown(EKeys::W)) { PanInput.X += 1.f; }
	if (PC->IsInputKeyDown(EKeys::S)) { PanInput.X -= 1.f; }
	if (PC->IsInputKeyDown(EKeys::D)) { PanInput.Y += 1.f; }
	if (PC->IsInputKeyDown(EKeys::A)) { PanInput.Y -= 1.f; }
	if (!PanInput.IsNearlyZero())
	{
		const float ZoomScale = FMath::Clamp(SpringArm->TargetArmLength / 2600.f, 0.4f, 1.6f);
		const FVector LocalMove(PanInput.X, PanInput.Y, 0.f);
		const FVector WorldMove = FRotator(0.f, GetActorRotation().Yaw, 0.f).RotateVector(LocalMove)
			* PanSpeed * ZoomScale * DeltaSeconds;
		FVector NewLocation = GetActorLocation() + WorldMove;
		NewLocation.X = FMath::Clamp(NewLocation.X, PanMin.X, PanMax.X);
		NewLocation.Y = FMath::Clamp(NewLocation.Y, PanMin.Y, PanMax.Y);
		SetActorLocation(NewLocation);
	}

	if (PC->WasInputKeyJustPressed(EKeys::MouseScrollUp))
	{
		TargetArmLength -= ZoomStep;
	}
	if (PC->WasInputKeyJustPressed(EKeys::MouseScrollDown))
	{
		TargetArmLength += ZoomStep;
	}
	TargetArmLength = FMath::Clamp(TargetArmLength, MinZoom, MaxZoom);

	SpringArm->TargetArmLength = FMath::FInterpTo(SpringArm->TargetArmLength, TargetArmLength, DeltaSeconds, 10.f);
}

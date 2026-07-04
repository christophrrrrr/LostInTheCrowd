#include "OrbitCameraPawn.h"

#include "Camera/CameraComponent.h"
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
	SpringArm->bDoCollisionTest = false; // diorama cam must never bump into stalls

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm);

	bUseControllerRotationYaw = false;
}

void AOrbitCameraPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		return;
	}

	// Orbit by anchoring the OS cursor (Slate reads/writes desktop
	// coordinates directly). This is deliberately independent of viewport
	// capture and input-mode state, which desync after clicks/zooms and
	// killed earlier implementations of this drag.
	if (PC->IsInputKeyDown(EKeys::RightMouseButton) && FSlateApplication::IsInitialized())
	{
		FSlateApplication& Slate = FSlateApplication::Get();
		const FVector2D CursorPos = Slate.GetCursorPos();
		if (!bDragging)
		{
			bDragging = true;
			DragAnchor = CursorPos;
		}
		else
		{
			const FVector2D Delta = CursorPos - DragAnchor;
			AddActorWorldRotation(FRotator(0.f, Delta.X * RotateSpeed, 0.f));
			PitchDegrees = FMath::Clamp(PitchDegrees - Delta.Y * RotateSpeed, -80.f, -20.f);
			SpringArm->SetRelativeRotation(FRotator(PitchDegrees, 0.f, 0.f));
			Slate.SetCursorPos(DragAnchor);
		}
	}
	else
	{
		bDragging = false;
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
		NewLocation.X = FMath::Clamp(NewLocation.X, -PanLimit, PanLimit);
		NewLocation.Y = FMath::Clamp(NewLocation.Y, -PanLimit, PanLimit);
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

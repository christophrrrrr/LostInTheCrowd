#include "OrbitCameraPawn.h"

#include "Camera/CameraComponent.h"
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

	// GetInputMouseDelta reads zero while the cursor is visible, so orbit by
	// anchoring the cursor: measure how far it moved from the anchor each
	// frame, apply that as rotation, then pin it back to the anchor.
	if (PC->IsInputKeyDown(EKeys::RightMouseButton))
	{
		float MouseX = 0.f, MouseY = 0.f;
		if (PC->GetMousePosition(MouseX, MouseY))
		{
			if (!bDragging)
			{
				bDragging = true;
				DragAnchor = FVector2D(MouseX, MouseY);
				PC->bShowMouseCursor = false;
			}
			else
			{
				const FVector2D Delta = FVector2D(MouseX, MouseY) - DragAnchor;
				AddActorWorldRotation(FRotator(0.f, Delta.X * RotateSpeed, 0.f));
				PitchDegrees = FMath::Clamp(PitchDegrees - Delta.Y * RotateSpeed, -80.f, -20.f);
				SpringArm->SetRelativeRotation(FRotator(PitchDegrees, 0.f, 0.f));
				PC->SetMouseLocation(static_cast<int32>(DragAnchor.X), static_cast<int32>(DragAnchor.Y));
			}
		}
	}
	else if (bDragging)
	{
		bDragging = false;
		PC->bShowMouseCursor = true;
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

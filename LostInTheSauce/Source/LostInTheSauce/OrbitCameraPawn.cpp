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

	if (PC->IsInputKeyDown(EKeys::RightMouseButton))
	{
		float DeltaX = 0.f, DeltaY = 0.f;
		PC->GetInputMouseDelta(DeltaX, DeltaY);

		AddActorWorldRotation(FRotator(0.f, DeltaX * RotateSpeed, 0.f));

		PitchDegrees = FMath::Clamp(PitchDegrees + DeltaY * RotateSpeed, -80.f, -20.f);
		SpringArm->SetRelativeRotation(FRotator(PitchDegrees, 0.f, 0.f));
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

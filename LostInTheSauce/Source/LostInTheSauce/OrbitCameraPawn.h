#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "OrbitCameraPawn.generated.h"

class USpringArmComponent;
class UCameraComponent;

// Diorama camera: orbits the market center. Right-drag rotates, wheel zooms.
// Input is polled directly from the player controller — no input assets needed.
UCLASS()
class LOSTINTHESAUCE_API AOrbitCameraPawn : public APawn
{
	GENERATED_BODY()

public:
	AOrbitCameraPawn();

	virtual void Tick(float DeltaSeconds) override;

protected:
	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<USceneComponent> Pivot;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(EditAnywhere, Category = "Orbit")
	float RotateSpeed = 0.35f; // degrees per screen pixel dragged

	UPROPERTY(EditAnywhere, Category = "Orbit")
	float ZoomStep = 250.f;

	UPROPERTY(EditAnywhere, Category = "Orbit")
	float MinZoom = 700.f;

	UPROPERTY(EditAnywhere, Category = "Orbit")
	float MaxZoom = 3400.f; // capped so the view can't peek past the ramparts

	UPROPERTY(EditAnywhere, Category = "Orbit")
	float PanSpeed = 1600.f; // cm/s at default zoom, scales with zoom level

	UPROPERTY(EditAnywhere, Category = "Orbit")
	float PanLimit = 2100.f; // fallback pivot clamp if no walls are tagged

protected:
	virtual void BeginPlay() override;

private:
	// Read the "CameraBound"-tagged rampart actors and inset the pan clamp
	// to their inner extent, so the pivot stays inside whatever walls the
	// level author placed.
	void ComputePanBoundsFromWalls();

	float TargetArmLength = 2600.f;
	float PitchDegrees = -55.f;
	bool bDragging = false;
	FVector2D DragAnchor = FVector2D::ZeroVector;
	FVector2D PanMin = FVector2D(-2100.f, -2100.f);
	FVector2D PanMax = FVector2D(2100.f, 2100.f);
};

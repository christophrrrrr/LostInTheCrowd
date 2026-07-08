#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "OrbitCameraPawn.generated.h"

class UCameraComponent;
class UFloatingPawnMovement;
class USphereComponent;
class USpotLightComponent;

// Free-fly camera: WASD moves, Space/Ctrl up/down, Shift = faster, hold the
// RIGHT mouse button to look around; cursor stays free otherwise for
// hovering/clicking NPCs. A collision sphere + engine pawn movement keep it
// out of buildings and inside the boundary walls — no hand-rolled clamping
// against geometry.
UCLASS()
class LOSTINTHESAUCE_API AOrbitCameraPawn : public APawn
{
	GENERATED_BODY()

public:
	AOrbitCameraPawn();

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<USphereComponent> CollisionSphere;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<UFloatingPawnMovement> Movement;

	// Focused flashlight pointing where the camera looks; toggled with F.
	UPROPERTY(VisibleAnywhere, Category = "Flashlight")
	TObjectPtr<USpotLightComponent> Flashlight;

	UPROPERTY(EditAnywhere, Category = "Fly")
	float LookSpeed = 3.0f; // degrees per unit of captured mouse delta

	UPROPERTY(EditAnywhere, Category = "Fly")
	float SprintMultiplier = 2.2f;

	UPROPERTY(EditAnywhere, Category = "Fly")
	float MinHeight = 180.f;

	UPROPERTY(EditAnywhere, Category = "Fly")
	float MaxHeight = 2600.f;

private:
	// Belt-and-braces XY clamp read from the CamBoundary-tagged walls
	// (collision already blocks; this guards against any physics tunneling).
	void ComputeBoundsFromWalls();

	float Yaw = 0.f;
	float Pitch = -40.f;
	bool bLooking = false;
	bool bFlashlightOn = false;
	FVector2D BoundsMin = FVector2D(-3200.f, -3200.f);
	FVector2D BoundsMax = FVector2D(3200.f, 3200.f);
};

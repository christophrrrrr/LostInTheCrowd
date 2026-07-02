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
	float RotateSpeed = 2.5f; // degrees per mouse delta unit

	UPROPERTY(EditAnywhere, Category = "Orbit")
	float ZoomStep = 250.f;

	UPROPERTY(EditAnywhere, Category = "Orbit")
	float MinZoom = 700.f;

	UPROPERTY(EditAnywhere, Category = "Orbit")
	float MaxZoom = 4200.f;

private:
	float TargetArmLength = 2600.f;
	float PitchDegrees = -55.f;
};

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "NPCTypes.h"
#include "NPCCharacter.generated.h"

class UAnimSequence;

UCLASS()
class LOSTINTHESAUCE_API ANPCCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ANPCCharacter();

	virtual void Tick(float DeltaSeconds) override;

	void SetNPCType(ENPCType InType);
	ENPCType GetNPCType() const { return NPCType; }

	// Hop + hit reaction so a wrong click still feels responsive.
	void ReactToWrongClick();

	// Target was found: stop wandering and wave at the player.
	void CelebrateFound();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "NPC")
	ENPCType NPCType = ENPCType::Farmer;

	// How tall every character stands in cm, regardless of import scale.
	UPROPERTY(EditAnywhere, Category = "NPC")
	float TargetHeight = 175.f;

private:
	void ApplyMesh();
	void PlayLooping(UAnimSequence* Anim);

	UPROPERTY()
	TObjectPtr<UAnimSequence> WalkAnim;
	UPROPERTY()
	TObjectPtr<UAnimSequence> IdleAnim;
	UPROPERTY()
	TObjectPtr<UAnimSequence> WaveAnim;
	UPROPERTY()
	TObjectPtr<UAnimSequence> HitAnim;

	UPROPERTY()
	TObjectPtr<UAnimSequence> CurrentLoop;

	bool bCelebrating = false;
	float OneShotEndTime = 0.f;

	// Import units on these GLBs are unreliable, so the rendered mesh is
	// measured and corrected over the first frames after spawn:
	// 0 = wait for valid bounds, 1 = fix scale, 2 = fix feet position, 3 = done.
	int32 CalibrationPhase = 0;
};

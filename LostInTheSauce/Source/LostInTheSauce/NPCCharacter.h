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

	// Fresnel rim overlay while the cursor is over this NPC.
	void SetHighlighted(bool bHighlighted);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "NPC")
	ENPCType NPCType = ENPCType::Farmer;

	// How tall every character stands in cm, regardless of import scale.
	UPROPERTY(EditAnywhere, Category = "NPC")
	float TargetHeight = 175.f;

private:
	void ApplyMesh();
	void ApplyCrowdColors();
	void PlayLooping(UAnimSequence* Anim);
	void UpdateFootsteps(float Speed, float DeltaSeconds);

	UPROPERTY()
	TObjectPtr<class USoundBase> FootstepSound;

	float StrideAccumulator = 0.f;

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
};

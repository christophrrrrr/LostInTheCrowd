#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "NPCTypes.h"
#include "NPCCharacter.generated.h"

class UMaterialInstanceDynamic;

UCLASS()
class LOSTINTHESAUCE_API ANPCCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ANPCCharacter();

	void SetNPCType(ENPCType InType);
	ENPCType GetNPCType() const { return NPCType; }

	// Brief hop + white flash so a wrong click still feels responsive.
	void ReactToWrongClick();

	// Target was found: stop wandering and hop in place until the round restarts.
	void CelebrateFound();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "NPC")
	TObjectPtr<UStaticMeshComponent> BodyMesh;

	UPROPERTY(VisibleAnywhere, Category = "NPC")
	TObjectPtr<UStaticMeshComponent> HatMesh;

	UPROPERTY(VisibleAnywhere, Category = "NPC")
	ENPCType NPCType = ENPCType::Farmer;

private:
	void ApplyStyle();
	void ScaleMeshTo(UStaticMeshComponent* Mesh, const FVector& TargetSize, float CenterZ) const;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> BodyMID;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> HatMID;

	UPROPERTY()
	TObjectPtr<UStaticMesh> CubeMesh;
	UPROPERTY()
	TObjectPtr<UStaticMesh> SphereMesh;
	UPROPERTY()
	TObjectPtr<UStaticMesh> CylinderMesh;
	UPROPERTY()
	TObjectPtr<UStaticMesh> ConeMesh;

	FTimerHandle FlashTimerHandle;
	FTimerHandle CelebrateTimerHandle;
};

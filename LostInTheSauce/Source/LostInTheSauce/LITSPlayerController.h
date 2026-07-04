#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "LITSPlayerController.generated.h"

UCLASS()
class LOSTINTHESAUCE_API ALITSPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ALITSPlayerController();

protected:
	virtual void BeginPlay() override;
	virtual void PlayerTick(float DeltaTime) override;

private:
	TWeakObjectPtr<class ANPCCharacter> HoveredNPC;
};

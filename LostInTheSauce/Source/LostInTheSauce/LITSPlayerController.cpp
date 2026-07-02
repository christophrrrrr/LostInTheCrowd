#include "LITSPlayerController.h"

#include "Engine/World.h"
#include "LITSGameMode.h"
#include "NPCCharacter.h"

ALITSPlayerController::ALITSPlayerController()
{
	bShowMouseCursor = true;
}

void ALITSPlayerController::BeginPlay()
{
	Super::BeginPlay();
	SetInputMode(FInputModeGameOnly());
}

void ALITSPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	ALITSGameMode* GameMode = GetWorld()->GetAuthGameMode<ALITSGameMode>();
	if (!GameMode)
	{
		return;
	}

	if (WasInputKeyJustPressed(EKeys::LeftMouseButton))
	{
		FHitResult Hit;
		if (GetHitResultUnderCursor(ECC_Visibility, false, Hit))
		{
			if (ANPCCharacter* NPC = Cast<ANPCCharacter>(Hit.GetActor()))
			{
				GameMode->HandleNPCClicked(NPC);
			}
		}
	}

	if (WasInputKeyJustPressed(EKeys::R))
	{
		GameMode->RequestRestart();
	}
}

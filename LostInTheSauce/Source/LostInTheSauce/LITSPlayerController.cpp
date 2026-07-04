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
	// Game-and-UI keeps the cursor free and click/scroll routing stable no
	// matter how often the player clicks; GameOnly mode kept wedging the
	// viewport into a captured state that ate the right-drag.
	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);
}

void ALITSPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	ALITSGameMode* GameMode = GetWorld()->GetAuthGameMode<ALITSGameMode>();
	if (!GameMode)
	{
		return;
	}

	// One cursor trace per frame drives both the hover highlight and clicks.
	ANPCCharacter* UnderCursor = nullptr;
	if (!GameMode->IsRoundWon())
	{
		FHitResult Hit;
		if (GetHitResultUnderCursor(ECC_Visibility, false, Hit))
		{
			UnderCursor = Cast<ANPCCharacter>(Hit.GetActor());
		}
	}

	if (UnderCursor != HoveredNPC.Get())
	{
		if (HoveredNPC.IsValid())
		{
			HoveredNPC->SetHighlighted(false);
		}
		HoveredNPC = UnderCursor;
		if (UnderCursor)
		{
			UnderCursor->SetHighlighted(true);
		}
	}

	if (WasInputKeyJustPressed(EKeys::LeftMouseButton) && UnderCursor)
	{
		GameMode->HandleNPCClicked(UnderCursor);
	}

	if (WasInputKeyJustPressed(EKeys::R))
	{
		GameMode->RequestRestart();
	}
}

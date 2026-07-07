#include "LITSPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "LITSGameMode.h"
#include "LITSMenuWidget.h"
#include "LITSPauseWidget.h"
#include "LITSResultsWidget.h"
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

	if (const ALITSGameMode* GameMode = GetWorld()->GetAuthGameMode<ALITSGameMode>();
		GameMode && GameMode->IsMenuPending())
	{
		if (ULITSMenuWidget* Menu = CreateWidget<ULITSMenuWidget>(this, ULITSMenuWidget::StaticClass()))
		{
			Menu->AddToViewport(10);
		}
	}
}

void ALITSPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	ALITSGameMode* GameMode = GetWorld()->GetAuthGameMode<ALITSGameMode>();
	if (!GameMode)
	{
		return;
	}

	// Time Attack results screen: show/remove the Retry/Menu buttons.
	if (GameMode->GetFlow() == ERoundFlow::Results)
	{
		if (!ResultsWidget.IsValid())
		{
			if (ULITSResultsWidget* W = CreateWidget<ULITSResultsWidget>(this, ULITSResultsWidget::StaticClass()))
			{
				W->AddToViewport(40);
				ResultsWidget = W;
			}
		}
		return; // no hovering/clicking NPCs on the results screen
	}
	if (ResultsWidget.IsValid())
	{
		ResultsWidget->RemoveFromParent();
		ResultsWidget.Reset();
	}

	// ESC pauses (skip while the start menu is still up).
	if (WasInputKeyJustPressed(EKeys::Escape) && !GameMode->IsMenuPending()
		&& !UGameplayStatics::IsGamePaused(GetWorld()))
	{
		if (ULITSPauseWidget* Pause = CreateWidget<ULITSPauseWidget>(this, ULITSPauseWidget::StaticClass()))
		{
			Pause->AddToViewport(50);
			UGameplayStatics::SetGamePaused(GetWorld(), true);
			FInputModeUIOnly InputMode;
			SetInputMode(InputMode);
			bShowMouseCursor = true;
			Pause->SetKeyboardFocus();
		}
	}

	// One cursor trace per frame drives both the hover highlight and clicks.
	ANPCCharacter* UnderCursor = nullptr;
	if (GameMode->IsPlaying())
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

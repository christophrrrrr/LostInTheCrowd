#include "LITSHUD.h"

#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/World.h"
#include "LITSGameMode.h"
#include "NPCTypes.h"

void ALITSHUD::DrawHUD()
{
	Super::DrawHUD();

	const ALITSGameMode* GameMode = GetWorld()->GetAuthGameMode<ALITSGameMode>();
	if (!GameMode || !Canvas)
	{
		return;
	}

	const FNPCTypeStyle& TargetStyle = NPCTypeStyles::Get(GameMode->GetTargetType());
	const float CenterX = Canvas->SizeX * 0.5f;

	// --- Objective banner -------------------------------------------------
	const FString Objective = FString::Printf(TEXT("FIND THE %s"), TargetStyle.DisplayName).ToUpper();
	DrawCenteredText(Objective, FLinearColor::White, CenterX, 34.f, 2.2f);

	DrawText(FString::Printf(TEXT("Round %d"), GameMode->GetRoundNumber()),
		FLinearColor(1.f, 1.f, 1.f, 0.8f), 24.f, 24.f, GEngine->GetLargeFont(), 1.4f);

	// Color swatch beside the banner — tinted the same way the crowd is, so
	// the hint stays honest as rounds get more color-similar.
	const FLinearColor Swatch = FMath::Lerp(TargetStyle.OutfitPrimary,
		NPCTypeStyles::MutedBase, GameMode->GetColorSimilarity());
	UFont* Font = GEngine->GetLargeFont();
	float TextWidth = 0.f, TextHeight = 0.f;
	GetTextSize(Objective, TextWidth, TextHeight, Font, 2.2f);
	const float SwatchX = CenterX + TextWidth * 0.5f + 18.f;
	DrawRect(FLinearColor(Swatch.R, Swatch.G, Swatch.B, 1.f), SwatchX, 40.f, 34.f, 34.f);

	// --- Wrong-click feedback ---------------------------------------------
	const float SinceWrong = GetWorld()->GetTimeSeconds() - GameMode->GetLastWrongClickTime();
	if (!GameMode->IsRoundWon() && SinceWrong < 1.2f)
	{
		const float Alpha = 1.f - (SinceWrong / 1.2f);
		DrawCenteredText(TEXT("Not them - keep looking!"),
			FLinearColor(1.f, 0.45f, 0.35f, Alpha), CenterX, 96.f, 1.4f);
	}

	// --- Win overlay --------------------------------------------------------
	if (GameMode->IsRoundWon())
	{
		DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.55f), 0.f, 0.f, Canvas->SizeX, Canvas->SizeY);

		const FString WinText = FString::Printf(TEXT("YOU FOUND THE %s!"), TargetStyle.DisplayName).ToUpper();
		DrawCenteredText(WinText, FLinearColor(1.f, 0.85f, 0.3f), CenterX, Canvas->SizeY * 0.42f, 3.f);
		DrawCenteredText(FString::Printf(TEXT("Press R for round %d"), GameMode->GetRoundNumber() + 1),
			FLinearColor::White, CenterX, Canvas->SizeY * 0.42f + 70.f, 1.5f);
	}

	// --- Round transition curtain (drawn on top of everything) --------------
	const float Now = GetWorld()->GetTimeSeconds();
	float CurtainAlpha = 0.f;
	if (GameMode->GetFlow() == ERoundFlow::Transition)
	{
		CurtainAlpha = FMath::Min(0.9f, (Now - GameMode->GetTransitionStartTime()) * 3.f);
	}
	else
	{
		CurtainAlpha = FMath::Max(0.f, 0.9f - (Now - GameMode->GetTransitionEndTime()) * 3.f);
	}
	if (CurtainAlpha > 0.01f)
	{
		DrawRect(FLinearColor(0.02f, 0.015f, 0.01f, CurtainAlpha), 0.f, 0.f, Canvas->SizeX, Canvas->SizeY);
		const float TextAlpha = CurtainAlpha / 0.9f;
		DrawCenteredText(FString::Printf(TEXT("ROUND %d"), GameMode->GetRoundNumber()),
			FLinearColor(1.f, 0.85f, 0.3f, TextAlpha), CenterX, Canvas->SizeY * 0.44f, 3.5f);
	}
}

void ALITSHUD::DrawCenteredText(const FString& Text, const FLinearColor& Color, float CenterX, float Y, float Scale)
{
	UFont* Font = GEngine->GetLargeFont();
	float TextWidth = 0.f, TextHeight = 0.f;
	GetTextSize(Text, TextWidth, TextHeight, Font, Scale);

	// Cheap drop shadow for readability over the bright market.
	DrawText(Text, FLinearColor(0.f, 0.f, 0.f, Color.A * 0.8f), CenterX - TextWidth * 0.5f + 2.f, Y + 2.f, Font, Scale);
	DrawText(Text, Color, CenterX - TextWidth * 0.5f, Y, Font, Scale);
}

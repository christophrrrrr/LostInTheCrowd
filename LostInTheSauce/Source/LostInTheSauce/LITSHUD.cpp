#include "LITSHUD.h"

#include "CanvasItem.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/World.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "LITSGameMode.h"
#include "NPCTypes.h"
#include "Styling/CoreStyle.h"

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

	{
		FCanvasTextItem RoundItem(FVector2D(24.f, 24.f),
			FText::FromString(FString::Printf(TEXT("Round %d"), GameMode->GetRoundNumber())),
			GEngine->GetLargeFont(), FLinearColor(1.f, 1.f, 1.f, 0.8f));
		RoundItem.SlateFontInfo = FCoreStyle::GetDefaultFontStyle("Bold", 18);
		RoundItem.EnableShadow(FLinearColor(0.f, 0.f, 0.f, 0.6f));
		Canvas->DrawItem(RoundItem);
	}

	// Color swatch beside the banner — tinted the same way the crowd is, so
	// the hint stays honest as rounds get more color-similar.
	const FLinearColor Swatch = FMath::Lerp(TargetStyle.OutfitPrimary,
		NPCTypeStyles::MutedBase, GameMode->GetColorSimilarity());
	const TSharedRef<FSlateFontMeasure> FontMeasure =
		FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	const FVector2D BannerSize = FontMeasure->Measure(Objective, FCoreStyle::GetDefaultFontStyle("Bold", 29));
	const float SwatchX = CenterX + BannerSize.X * 0.5f + 18.f;
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
		CurtainAlpha = FMath::Min(1.f, (Now - GameMode->GetTransitionStartTime()) * 3.f);
	}
	else
	{
		CurtainAlpha = FMath::Max(0.f, 1.f - (Now - GameMode->GetTransitionEndTime()) * 3.f);
	}
	if (CurtainAlpha > 0.01f)
	{
		DrawRect(FLinearColor(0.02f, 0.015f, 0.01f, CurtainAlpha), 0.f, 0.f, Canvas->SizeX, Canvas->SizeY);
		const float TextAlpha = CurtainAlpha;
		DrawCenteredText(FString::Printf(TEXT("ROUND %d"), GameMode->GetRoundNumber()),
			FLinearColor(1.f, 0.85f, 0.3f, TextAlpha), CenterX, Canvas->SizeY * 0.44f, 3.5f);
	}
}

void ALITSHUD::DrawCenteredText(const FString& Text, const FLinearColor& Color, float CenterX, float Y, float Scale)
{
	// Slate-rendered canvas text: glyphs rasterized at the exact pixel size
	// instead of scaling a small bitmap font (which was blurry at any scale).
	FCanvasTextItem Item(FVector2D(CenterX, Y), FText::FromString(Text), GEngine->GetLargeFont(), Color);
	Item.SlateFontInfo = FCoreStyle::GetDefaultFontStyle("Bold", FMath::RoundToInt(13.f * Scale));
	Item.bCentreX = true;
	Item.EnableShadow(FLinearColor(0.f, 0.f, 0.f, Color.A * 0.8f));
	Canvas->DrawItem(Item);
}

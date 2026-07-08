#include "LITSMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "Kismet/KismetSystemLibrary.h"
#include "LITSGameMode.h"
#include "Styling/CoreStyle.h"

namespace
{
	UTextBlock* MakeText(UWidgetTree* Tree, const FString& Text, int32 Size,
		const FLinearColor& Color, bool bBold = true)
	{
		UTextBlock* Block = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Block->SetText(FText::FromString(Text));
		Block->SetFont(FCoreStyle::GetDefaultFontStyle(bBold ? "Bold" : "Regular", Size));
		Block->SetColorAndOpacity(FSlateColor(Color));
		Block->SetJustification(ETextJustify::Center);
		return Block;
	}
}

bool ULITSMenuWidget::Initialize()
{
	if (!Super::Initialize())
	{
		return false;
	}
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return true;
	}

	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	// Backdrop: pure black while the opening round assembles (all loading
	// hitches invisible), then melts into a warm vignette over the market.
	Backdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Backdrop->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 1.f));
	UOverlaySlot* DimSlot = Root->AddChildToOverlay(Backdrop);
	DimSlot->SetHorizontalAlignment(HAlign_Fill);
	DimSlot->SetVerticalAlignment(VAlign_Fill);

	// Subtle loading hint so slow machines don't look frozen.
	LoadingText = MakeText(WidgetTree, TEXT("Loading..."), 16, FLinearColor(0.55f, 0.52f, 0.45f), false);
	UOverlaySlot* LoadingSlot = Root->AddChildToOverlay(LoadingText);
	LoadingSlot->SetHorizontalAlignment(HAlign_Center);
	LoadingSlot->SetVerticalAlignment(VAlign_Bottom);
	LoadingSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 48.f));

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Content = Column;
	Content->SetRenderOpacity(0.f); // whole title screen appears only when ready
	UOverlaySlot* ColumnSlot = Root->AddChildToOverlay(Column);
	ColumnSlot->SetHorizontalAlignment(HAlign_Center);
	ColumnSlot->SetVerticalAlignment(VAlign_Center);

	auto AddRow = [&Column](UWidget* Widget, float TopPadding)
	{
		UVerticalBoxSlot* Slot = Column->AddChildToVerticalBox(Widget);
		Slot->SetPadding(FMargin(0.f, TopPadding, 0.f, 0.f));
		Slot->SetHorizontalAlignment(HAlign_Center);
		return Slot;
	};

	AddRow(MakeText(WidgetTree, TEXT("LOST IN THE CROWD"), 64, FLinearColor(1.f, 0.85f, 0.3f)), 0.f);
	AddRow(MakeText(WidgetTree, TEXT("One of them is not like the others."), 20,
		FLinearColor(0.9f, 0.88f, 0.82f), false), 8.f);

	// --- Two big mode-select cards side by side ---------------------------
	UHorizontalBox* Cards = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UVerticalBoxSlot* CardsRow = Column->AddChildToVerticalBox(Cards);
	CardsRow->SetHorizontalAlignment(HAlign_Center);
	CardsRow->SetPadding(FMargin(0.f, 40.f, 0.f, 0.f));

	auto MakeCard = [&](const FString& Name, const FString& Tagline, const TCHAR* PortraitPath,
		const FLinearColor& Accent) -> UButton*
	{
		// A fixed-size square-ish card.
		USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		Size->SetWidthOverride(300.f);
		Size->SetHeightOverride(390.f);

		UButton* Card = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
		// Per-state background so the card lifts on hover.
		FButtonStyle Style = Card->GetStyle();
		Style.Normal.TintColor = FSlateColor(FLinearColor(0.11f, 0.09f, 0.07f, 0.92f));
		Style.Hovered.TintColor = FSlateColor(FLinearColor(Accent.R * 0.5f, Accent.G * 0.45f, Accent.B * 0.4f, 0.98f));
		Style.Pressed.TintColor = FSlateColor(FLinearColor(Accent.R * 0.6f, Accent.G * 0.55f, Accent.B * 0.5f, 1.f));
		Card->SetStyle(Style);
		Size->AddChild(Card);

		UVerticalBox* Inner = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		Card->AddChild(Inner);

		auto InnerRow = [&](UWidget* W, const FMargin& Pad, EHorizontalAlignment HA = HAlign_Center)
		{
			UVerticalBoxSlot* S = Inner->AddChildToVerticalBox(W);
			S->SetPadding(Pad);
			S->SetHorizontalAlignment(HA);
			return S;
		};

		// Portrait thumbnail in an accent frame.
		if (UTexture2D* Portrait = LoadObject<UTexture2D>(nullptr, PortraitPath))
		{
			UBorder* Frame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
			Frame->SetBrushColor(Accent);
			Frame->SetPadding(FMargin(3.f));
			UImage* Img = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
			Img->SetBrushFromTexture(Portrait);
			// Force the draw size (SetDesiredSizeOverride renders tiny inside a box).
			FSlateBrush Brush = Img->GetBrush();
			Brush.ImageSize = FVector2D(158.f, 216.f);
			Img->SetBrush(Brush);
			Frame->AddChild(Img);
			InnerRow(Frame, FMargin(0.f, 20.f, 0.f, 0.f));
		}

		InnerRow(MakeText(WidgetTree, Name, 28, FLinearColor(1.f, 0.88f, 0.45f)), FMargin(0.f, 16.f, 0.f, 0.f));
		UTextBlock* Tag = MakeText(WidgetTree, Tagline, 13, FLinearColor(0.82f, 0.8f, 0.74f), false);
		Tag->SetAutoWrapText(true);
		InnerRow(Tag, FMargin(20.f, 8.f, 20.f, 0.f));

		UHorizontalBoxSlot* HSlot = Cards->AddChildToHorizontalBox(Size);
		HSlot->SetPadding(FMargin(18.f, 0.f));
		return Card;
	};

	PlayButton = MakeCard(TEXT("ENDLESS"),
		TEXT("Find the one target. Each win grows the crowd and blurs the colors. Play forever."),
		TEXT("/Game/LostInTheSauce/Portraits/P_King"), FLinearColor(1.f, 0.78f, 0.28f));
	PlayButton->OnClicked.AddDynamic(this, &ULITSMenuWidget::HandleEndlessClicked);

	TimeAttackButton = MakeCard(TEXT("TIME ATTACK"),
		TEXT("Find all 10 before the clock runs out. Each clear cuts 10 seconds. How long can you last?"),
		TEXT("/Game/LostInTheSauce/Portraits/P_Witch"), FLinearColor(0.95f, 0.5f, 0.85f));
	TimeAttackButton->OnClicked.AddDynamic(this, &ULITSMenuWidget::HandleTimeAttackClicked);

	AddRow(MakeText(WidgetTree,
		TEXT("WASD fly   |   Hold right mouse: look   |   Space / Ctrl: up & down\n")
		TEXT("Shift: faster   |   Esc: pause"),
		14, FLinearColor(0.72f, 0.7f, 0.64f), false), 34.f);

	QuitButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	UTextBlock* QuitLabel = MakeText(WidgetTree, TEXT("Quit"), 16, FLinearColor(0.9f, 0.88f, 0.82f));
	QuitButton->AddChild(QuitLabel);
	if (UButtonSlot* QuitSlot = Cast<UButtonSlot>(QuitLabel->Slot))
	{
		QuitSlot->SetPadding(FMargin(24.f, 6.f));
	}
	QuitButton->SetBackgroundColor(FLinearColor(0.25f, 0.22f, 0.18f));
	QuitButton->OnClicked.AddDynamic(this, &ULITSMenuWidget::HandleQuitClicked);
	AddRow(QuitButton, 28.f);

	return true;
}

void ULITSMenuWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!Backdrop || !Content)
	{
		return;
	}
	const ALITSGameMode* GameMode = GetWorld()->GetAuthGameMode<ALITSGameMode>();
	const bool bWorldReady = GameMode && GameMode->GetFlow() != ERoundFlow::Transition;

	// Backdrop: black wall -> warm translucent vignette once ready.
	const float TargetAlpha = bWorldReady ? 0.72f : 1.f;
	if (!FMath::IsNearlyEqual(BackdropAlpha, TargetAlpha, 0.002f))
	{
		BackdropAlpha = FMath::FInterpTo(BackdropAlpha, TargetAlpha, InDeltaTime, 1.6f);
		const FLinearColor Warm(0.03f, 0.02f, 0.012f);
		const float WarmBlend = (1.f - BackdropAlpha) / 0.28f; // 0 at black, 1 at vignette
		Backdrop->SetBrushColor(FLinearColor(
			Warm.R * WarmBlend, Warm.G * WarmBlend, Warm.B * WarmBlend, BackdropAlpha));
	}

	// Title screen content fades in as one piece; loading hint fades out.
	const float TargetContent = bWorldReady ? 1.f : 0.f;
	if (!FMath::IsNearlyEqual(ContentOpacity, TargetContent, 0.002f))
	{
		ContentOpacity = FMath::FInterpTo(ContentOpacity, TargetContent, InDeltaTime, 2.2f);
		Content->SetRenderOpacity(ContentOpacity);
		if (LoadingText)
		{
			LoadingText->SetRenderOpacity(1.f - ContentOpacity);
		}
	}
	if (PlayButton)
	{
		PlayButton->SetIsEnabled(bWorldReady);
	}
	if (TimeAttackButton)
	{
		TimeAttackButton->SetIsEnabled(bWorldReady);
	}
}

void ULITSMenuWidget::HandleEndlessClicked()
{
	if (ALITSGameMode* GameMode = GetWorld()->GetAuthGameMode<ALITSGameMode>())
	{
		GameMode->PlayGameSound(TEXT("/Game/LostInTheSauce/Sounds/S_UIClick"));
		GameMode->StartMode(EGameMode::Endless);
	}
	RemoveFromParent();
}

void ULITSMenuWidget::HandleTimeAttackClicked()
{
	if (ALITSGameMode* GameMode = GetWorld()->GetAuthGameMode<ALITSGameMode>())
	{
		GameMode->PlayGameSound(TEXT("/Game/LostInTheSauce/Sounds/S_UIClick"));
		GameMode->StartMode(EGameMode::TimeAttack);
	}
	RemoveFromParent();
}

void ULITSMenuWidget::HandleQuitClicked()
{
	UKismetSystemLibrary::QuitGame(GetWorld(), GetOwningPlayer(), EQuitPreference::Quit, false);
}

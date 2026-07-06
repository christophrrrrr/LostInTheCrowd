#include "LITSMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
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

	// Backdrop: starts fully OPAQUE (title card) so the first crowd spawn's
	// loading hitches happen invisibly, then NativeTick melts it into a
	// translucent vignette over the live market.
	Backdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Backdrop->SetBrushColor(FLinearColor(0.03f, 0.02f, 0.012f, BackdropAlpha));
	UOverlaySlot* DimSlot = Root->AddChildToOverlay(Backdrop);
	DimSlot->SetHorizontalAlignment(HAlign_Fill);
	DimSlot->SetVerticalAlignment(VAlign_Fill);

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
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

	PlayButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	UTextBlock* PlayLabel = MakeText(WidgetTree, TEXT("PLAY"), 30, FLinearColor(0.05f, 0.04f, 0.02f));
	PlayButton->AddChild(PlayLabel);
	if (UButtonSlot* PlaySlot = Cast<UButtonSlot>(PlayLabel->Slot))
	{
		PlaySlot->SetPadding(FMargin(48.f, 10.f));
	}
	PlayButton->SetBackgroundColor(FLinearColor(1.f, 0.8f, 0.25f));
	PlayButton->OnClicked.AddDynamic(this, &ULITSMenuWidget::HandlePlayClicked);
	AddRow(PlayButton, 42.f);

	AddRow(MakeText(WidgetTree,
		TEXT("Find the character named at the top and click them.\n")
		TEXT("Wrong guesses are free - but the crowd grows every round.\n\n")
		TEXT("WASD fly   |   Hold right mouse: look   |   Space / Ctrl: up & down\n")
		TEXT("Shift: faster   |   R: next round"),
		16, FLinearColor(0.8f, 0.78f, 0.72f), false), 36.f);

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

	if (!Backdrop)
	{
		return;
	}
	const ALITSGameMode* GameMode = GetWorld()->GetAuthGameMode<ALITSGameMode>();
	// Stay opaque while the opening round is still assembling; then reveal.
	const bool bWorldReady = GameMode && GameMode->GetFlow() != ERoundFlow::Transition;
	const float Target = bWorldReady ? 0.72f : 1.f;
	if (!FMath::IsNearlyEqual(BackdropAlpha, Target, 0.002f))
	{
		BackdropAlpha = FMath::FInterpTo(BackdropAlpha, Target, InDeltaTime, 1.8f);
		Backdrop->SetBrushColor(FLinearColor(0.03f, 0.02f, 0.012f, BackdropAlpha));
	}
}

void ULITSMenuWidget::HandlePlayClicked()
{
	if (ALITSGameMode* GameMode = GetWorld()->GetAuthGameMode<ALITSGameMode>())
	{
		GameMode->PlayGameSound(TEXT("/Game/LostInTheSauce/Sounds/S_UIClick"));
		GameMode->StartFromMenu();
	}
	RemoveFromParent();
}

void ULITSMenuWidget::HandleQuitClicked()
{
	UKismetSystemLibrary::QuitGame(GetWorld(), GetOwningPlayer(), EQuitPreference::Quit, false);
}

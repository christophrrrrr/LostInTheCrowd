#include "LITSPauseWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "LITSGameMode.h"
#include "Styling/CoreStyle.h"

namespace
{
	UTextBlock* MakeText(UWidgetTree* Tree, const FString& Text, int32 Size, const FLinearColor& Color)
	{
		UTextBlock* Block = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Block->SetText(FText::FromString(Text));
		Block->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", Size));
		Block->SetColorAndOpacity(FSlateColor(Color));
		Block->SetJustification(ETextJustify::Center);
		return Block;
	}
}

bool ULITSPauseWidget::Initialize()
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

	UBorder* Dim = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Dim->SetBrushColor(FLinearColor(0.01f, 0.008f, 0.006f, 0.82f));
	UOverlaySlot* DimSlot = Root->AddChildToOverlay(Dim);
	DimSlot->SetHorizontalAlignment(HAlign_Fill);
	DimSlot->SetVerticalAlignment(VAlign_Fill);

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	UOverlaySlot* ColSlot = Root->AddChildToOverlay(Column);
	ColSlot->SetHorizontalAlignment(HAlign_Center);
	ColSlot->SetVerticalAlignment(VAlign_Center);

	UVerticalBoxSlot* TitleSlot = Column->AddChildToVerticalBox(
		MakeText(WidgetTree, TEXT("PAUSED"), 48, FLinearColor(1.f, 0.85f, 0.3f)));
	TitleSlot->SetHorizontalAlignment(HAlign_Center);
	TitleSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 24.f));

	AddButton(Column, TEXT("Resume"), FLinearColor(1.f, 0.8f, 0.25f))
		->OnClicked.AddDynamic(this, &ULITSPauseWidget::OnResume);
	AddButton(Column, TEXT("Restart"), FLinearColor(0.75f, 0.7f, 0.6f))
		->OnClicked.AddDynamic(this, &ULITSPauseWidget::OnRestart);
	AddButton(Column, TEXT("Quit to Title"), FLinearColor(0.55f, 0.5f, 0.42f))
		->OnClicked.AddDynamic(this, &ULITSPauseWidget::OnQuitToTitle);
	AddButton(Column, TEXT("Quit Game"), FLinearColor(0.4f, 0.28f, 0.24f))
		->OnClicked.AddDynamic(this, &ULITSPauseWidget::OnQuitGame);

	// Focusable so ESC (NativeOnKeyDown) resumes as well.
	SetIsFocusable(true);
	return true;
}

UButton* ULITSPauseWidget::AddButton(UVerticalBox* Column, const FString& Label, const FLinearColor& Color)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	UTextBlock* Text = MakeText(WidgetTree, Label, 22, FLinearColor(0.05f, 0.04f, 0.02f));
	Button->AddChild(Text);
	if (UButtonSlot* BSlot = Cast<UButtonSlot>(Text->Slot))
	{
		BSlot->SetPadding(FMargin(40.f, 9.f));
	}
	Button->SetBackgroundColor(Color);
	UVerticalBoxSlot* BoxSlot = Column->AddChildToVerticalBox(Button);
	BoxSlot->SetHorizontalAlignment(HAlign_Fill);
	BoxSlot->SetPadding(FMargin(0.f, 6.f));
	return Button;
}

FReply ULITSPauseWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		OnResume();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void ULITSPauseWidget::Dismiss()
{
	UGameplayStatics::SetGamePaused(GetWorld(), false);
	if (APlayerController* PC = GetOwningPlayer())
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		PC->SetInputMode(InputMode);
		PC->bShowMouseCursor = true;
	}
	RemoveFromParent();
}

void ULITSPauseWidget::OnResume()
{
	Dismiss();
}

void ULITSPauseWidget::OnRestart()
{
	if (ALITSGameMode* GameMode = GetWorld()->GetAuthGameMode<ALITSGameMode>())
	{
		GameMode->RestartFromRoundOne();
	}
	Dismiss();
}

void ULITSPauseWidget::OnQuitToTitle()
{
	if (ALITSGameMode* GameMode = GetWorld()->GetAuthGameMode<ALITSGameMode>())
	{
		GameMode->ReturnToTitle();
	}
	Dismiss();
	// Reload the level so the start menu shows again cleanly.
	UGameplayStatics::OpenLevel(GetWorld(), FName(TEXT("MarketMap")));
}

void ULITSPauseWidget::OnQuitGame()
{
	UKismetSystemLibrary::QuitGame(GetWorld(), GetOwningPlayer(), EQuitPreference::Quit, false);
}

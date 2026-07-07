#include "LITSResultsWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "LITSGameMode.h"
#include "Styling/CoreStyle.h"

bool ULITSResultsWidget::Initialize()
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

	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UOverlaySlot* RowSlot = Root->AddChildToOverlay(Row);
	RowSlot->SetHorizontalAlignment(HAlign_Center);
	RowSlot->SetVerticalAlignment(VAlign_Center);
	// Sits below the HUD's results text.
	RowSlot->SetPadding(FMargin(0.f, 200.f, 0.f, 0.f));

	auto MakeButton = [&](const FString& Label, const FLinearColor& Color) -> UButton*
	{
		UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
		UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Text->SetText(FText::FromString(Label));
		Text->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 22));
		Text->SetColorAndOpacity(FSlateColor(FLinearColor(0.05f, 0.04f, 0.02f)));
		Button->AddChild(Text);
		if (UButtonSlot* BSlot = Cast<UButtonSlot>(Text->Slot))
		{
			BSlot->SetPadding(FMargin(42.f, 12.f));
		}
		Button->SetBackgroundColor(Color);
		UHorizontalBoxSlot* HSlot = Row->AddChildToHorizontalBox(Button);
		HSlot->SetPadding(FMargin(16.f, 0.f));
		return Button;
	};

	MakeButton(TEXT("Retry"), FLinearColor(1.f, 0.8f, 0.25f))
		->OnClicked.AddDynamic(this, &ULITSResultsWidget::OnRetry);
	MakeButton(TEXT("Main Menu"), FLinearColor(0.6f, 0.55f, 0.46f))
		->OnClicked.AddDynamic(this, &ULITSResultsWidget::OnMenu);

	return true;
}

void ULITSResultsWidget::OnRetry()
{
	if (ALITSGameMode* GameMode = GetWorld()->GetAuthGameMode<ALITSGameMode>())
	{
		GameMode->RestartFromRoundOne();
	}
	RemoveFromParent();
}

void ULITSResultsWidget::OnMenu()
{
	// Reload the level so the start menu shows cleanly.
	UGameplayStatics::OpenLevel(GetWorld(), FName(TEXT("MarketMap")));
	RemoveFromParent();
}

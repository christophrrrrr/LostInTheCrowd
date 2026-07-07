#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LITSPauseWidget.generated.h"

class UButton;

// ESC pause menu (pure C++): Resume / Restart / Quit to title / Quit game.
// The game is frozen (paused) while this is up.
UCLASS()
class LOSTINTHESAUCE_API ULITSPauseWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual bool Initialize() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	void Dismiss();

private:
	UFUNCTION() void OnResume();
	UFUNCTION() void OnRestart();
	UFUNCTION() void OnQuitToTitle();
	UFUNCTION() void OnQuitGame();

	UButton* AddButton(class UVerticalBox* Column, const FString& Label, const FLinearColor& Color);
};

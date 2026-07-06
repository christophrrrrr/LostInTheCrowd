#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LITSMenuWidget.generated.h"

class UButton;

// Start screen shown over the live market: title, play, how-to, quit.
// Built entirely in C++ (WidgetTree) — no Blueprint assets.
UCLASS()
class LOSTINTHESAUCE_API ULITSMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual bool Initialize() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	UFUNCTION()
	void HandlePlayClicked();

	UFUNCTION()
	void HandleQuitClicked();

	UPROPERTY()
	TObjectPtr<UButton> PlayButton;

	UPROPERTY()
	TObjectPtr<UButton> QuitButton;

	UPROPERTY()
	TObjectPtr<class UBorder> Backdrop;

	UPROPERTY()
	TObjectPtr<class UTextBlock> LoadingText;

	UPROPERTY()
	TObjectPtr<class UVerticalBox> Content;

	// Intro: pure black + "Loading..." while the opening round assembles;
	// when everything is ready, the complete title screen (title, buttons,
	// live-crowd vignette) fades in as one piece.
	float BackdropAlpha = 1.f;
	float ContentOpacity = 0.f;
};

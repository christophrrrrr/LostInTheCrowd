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

	// Title-card intro: the backdrop starts fully opaque (masking the crowd
	// spawn hitches) and melts to a translucent vignette once the round has
	// finished setting up behind the menu.
	float BackdropAlpha = 1.f;
};

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

private:
	UFUNCTION()
	void HandlePlayClicked();

	UFUNCTION()
	void HandleQuitClicked();

	UPROPERTY()
	TObjectPtr<UButton> PlayButton;

	UPROPERTY()
	TObjectPtr<UButton> QuitButton;
};

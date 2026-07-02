#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "LITSHUD.generated.h"

// Graybox HUD drawn straight to the canvas — no widget assets needed.
// Will be replaced with UMG once the game loop is proven.
UCLASS()
class LOSTINTHESAUCE_API ALITSHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

private:
	void DrawCenteredText(const FString& Text, const FLinearColor& Color, float CenterX, float Y, float Scale);
};

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LITSResultsWidget.generated.h"

// Time Attack results buttons (Retry / Main Menu). The HUD draws the stats
// text underneath; this widget just adds the interactive buttons.
UCLASS()
class LOSTINTHESAUCE_API ULITSResultsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual bool Initialize() override;

private:
	UFUNCTION() void OnRetry();
	UFUNCTION() void OnMenu();
};

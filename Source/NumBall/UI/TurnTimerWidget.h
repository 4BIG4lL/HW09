#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TurnTimerWidget.generated.h"

class UProgressBar;
class UTextBlock;
class ANumBallGameState;

UCLASS()
class UTurnTimerWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(meta = (BindWidget))
	UProgressBar* TimeBar;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* TimeText;

private:
	TWeakObjectPtr<ANumBallGameState> CachedGS;

	ANumBallGameState* GetGS();
};

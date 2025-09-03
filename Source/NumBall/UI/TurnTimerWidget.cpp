#include "UI/TurnTimerWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "Game/NumBallGameState.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TurnTimerWidget)

void UTurnTimerWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	ANumBallGameState* GS = GetGS();
	if (GS == nullptr || GS->TurnSeconds <= 0.0f)
	{
		return;
	}
	
	APlayerController* PC = GetOwningPlayer();
	const bool bMyTurn =
		(PC && PC->PlayerState && GS->CurrentTurnPS == PC->PlayerState);

	if (bMyTurn)
	{
		const float Remain = GS->TurnTimeRemaining;
		const float Ratio = FMath::Clamp(Remain / GS->TurnSeconds, 0.0f, 1.0f);

		if (TimeBar)
		{
			TimeBar->SetPercent(Ratio);
		}
		if (TimeText)
		{
			TimeText->SetText(FText::FromString(
				FString::Printf(TEXT("내 턴 • 남은 시간: %.0fs"), Remain)));
		}
	}
	else
	{
		if (TimeBar)
		{
			TimeBar->SetPercent(0.0f);
		}
		if (TimeText)
		{
			TimeText->SetText(FText::FromString(TEXT("상대 턴")));
		}
	}
}

ANumBallGameState* UTurnTimerWidget::GetGS()
{
	if (CachedGS.IsValid())
	{
		return CachedGS.Get();
	}

	if (UWorld* W = GetWorld())
	{
		if (AGameStateBase* GSBase = UGameplayStatics::GetGameState(W))
		{
			CachedGS = Cast<ANumBallGameState>(GSBase);
			return CachedGS.Get();
		}
	}
	return nullptr;
}

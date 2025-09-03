#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "NumBallGameMode.generated.h"

class ANumBallGameState;
class ANumBallPlayerController;
class ANumBallPlayerState;

USTRUCT(BlueprintType)
struct FJudgeResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	int32 Strikes { 0 };

	UPROPERTY(BlueprintReadOnly)
	int32 Balls { 0 };

	UPROPERTY(BlueprintReadOnly)
	bool bOut { false };

	FString ToResultString() const
	{
		if (bOut && Strikes == 0 && Balls == 0)
		{
			return TEXT("OUT");
		}
		return FString::Printf(TEXT("%dS%dB"), Strikes, Balls);
	}
};

UCLASS()
class ANumBallGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ANumBallGameMode();

	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	virtual void BeginPlay() override;
	
	void HandleChatFromPlayer(ANumBallPlayerController* FromPC, const FString& Message);
	
	bool TrySubmitGuess(ANumBallPlayerController* FromPC, const FString& Message, FString& OutSystemReply);
	
	void ResetMatch();
	
	void AdvanceTurn(bool bConsumeByTimeout);

protected:
	void GenerateAnswer3Digits();
	
	bool ExtractValidGuess(const FString& Message, FString& OutDigits) const;
	
	FJudgeResult Judge(const FString& Answer, const FString& Guess) const;
	
	void EvaluateWinConditions(ANumBallPlayerController* LastActorPC, const FJudgeResult& Result);
	
	void StartTurnTick();

private:
	FString AnswerDigits;
};

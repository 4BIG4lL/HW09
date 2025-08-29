#include "Game/NBGameModeBase.h"
#include "NBGameStateBase.h"
#include "Player/NBPlayerController.h"
#include "EngineUtils.h"
#include "Player/NBPlayerState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NBGameModeBase)

void ANBGameModeBase::OnPostLogin(AController* NewPlayer)
{
	Super::OnPostLogin(NewPlayer);

	ANBPlayerController* NBPlayerController = Cast<ANBPlayerController>(NewPlayer);
	if (IsValid(NBPlayerController) == true)
	{
		NBPlayerController->NotificationText = FText::FromString(TEXT("Connected to the game server."));

		AllPlayerControllers.Add(NBPlayerController);

		ANBPlayerState* NBPS = NBPlayerController->GetPlayerState<ANBPlayerState>();
		if (IsValid(NBPS) == true)
		{
			NBPS->PlayerNameString = TEXT ("Player") + FString::FromInt(AllPlayerControllers.Num());
		}

		ANBGameStateBase* NBGS = GetGameState<ANBGameStateBase>();
		if (IsValid(NBGS) == true)
		{
			NBGS->MulticastRPCBroadcastLoginMessage(NBPS->PlayerNameString);
		}
	}
}

FString ANBGameModeBase::GenerateSecretNumber()
{
	TArray<int32> Numbers;
	for (int32 i = 1; i <= 9; ++i)
	{
		Numbers.Add(i);
	}

	FMath::RandInit(FDateTime::Now().GetTicks());
	Numbers = Numbers.FilterByPredicate([](int32 Num) { return Num > 0; });

	FString Result;
	for (int32 i = 0; i < 3; ++i)
	{
		int32 Index = FMath::RandRange(0, Numbers.Num() - 1);
		Result.Append(FString::FromInt(Numbers[Index]));
		Numbers.RemoveAt(Index);
	}

	return Result;
}

bool ANBGameModeBase::IsGuessNumberString(const FString& InNumberString)
{
	bool bCanPlay = false;

	do {

		if (InNumberString.Len() != 3)
		{
			break;
		}

		bool bIsUnique = true;
		TSet<TCHAR> UniqueDigits;
		for (TCHAR C : InNumberString)
		{
			if (FChar::IsDigit(C) == false || C == '0')
			{
				bIsUnique = false;
				break;
			}

			UniqueDigits.Add(C);
		}

		if (bIsUnique == false)
		{
			break;
		}

		bCanPlay = true;

	} while (false);

	return bCanPlay;
}

FString ANBGameModeBase::JudgeResult(const FString& InSecretNumberString, const FString& InGuessNumberString)
{
	int32 StrikeCount = 0, BallCount = 0;

	for (int32 i = 0; i < 3; ++i)
	{
		if (InSecretNumberString[i] == InGuessNumberString[i])
		{
			StrikeCount++;
		}
		else
		{
			FString PlayerGuessChar = FString::Printf(TEXT("%c"), InGuessNumberString[i]);
			if (InSecretNumberString.Contains(PlayerGuessChar))
			{
				BallCount++;
			}
		}
	}

	if (StrikeCount == 0 && BallCount == 0)
	{
		return TEXT("OUT");
	}

	return FString::Printf(TEXT("%dS%dB"), StrikeCount, BallCount);
}

void ANBGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	SecretNumberString = GenerateSecretNumber();
}

void ANBGameModeBase::PrintChatMessageString(ANBPlayerController* InChattingPlayerController, const FString& InChatMessageString)
{
	FString ChatMessageString = InChatMessageString;
	int Index = InChatMessageString.Len() - 3;
	FString GuessNumberString = InChatMessageString.RightChop(Index);
	if (IsGuessNumberString(GuessNumberString) == true)
	{
		FString JudgeResultString = JudgeResult(SecretNumberString, GuessNumberString);

		IncreaseGuessCount(InChattingPlayerController);

		for (TActorIterator<ANBPlayerController> It(GetWorld()); It; ++It)
		{
			ANBPlayerController* NBPlayerController = *It;
			if (IsValid(NBPlayerController) == true)
			{
				FString CombinedMessageString = InChatMessageString + TEXT(" -> ") + JudgeResultString;
				NBPlayerController->ClientRPCPrintChatMessageString(CombinedMessageString);

				int32 StrikeCount = FCString::Atoi(*JudgeResultString.Left(1));
				JudgeGame(InChattingPlayerController, StrikeCount);
			}
		}
	}
	else
	{
		{
			for (TActorIterator<ANBPlayerController> It(GetWorld()); It; ++It)
			{
				ANBPlayerController* NBPC = *It;
				if (IsValid(NBPC) == true)
				{
					NBPC->ClientRPCPrintChatMessageString(InChatMessageString);
				}
			}
		}
	}
}

void ANBGameModeBase::IncreaseGuessCount(ANBPlayerController* InChattingPlayerController)
{
	ANBPlayerState* NBPS = InChattingPlayerController->GetPlayerState<ANBPlayerState>();
	if (IsValid(NBPS) == true)
	{
		NBPS->CurrentGuessCount++;
	}
}

void ANBGameModeBase::ResetGame()
{
	SecretNumberString = GenerateSecretNumber();

	for (const auto& NBPlayerController : AllPlayerControllers)
	{
		ANBPlayerState* NBPS = NBPlayerController->GetPlayerState<ANBPlayerState>();
		if (IsValid(NBPS) == true)
		{
			NBPS->CurrentGuessCount = 0;
		}
	}
}

void ANBGameModeBase::JudgeGame(ANBPlayerController* InChattingPlayerController, int InStrikeCount)
{
	if (3 == InStrikeCount)
	{
		ANBPlayerState* NBPS = InChattingPlayerController->GetPlayerState<ANBPlayerState>();
		for (const auto& NBPlayerController : AllPlayerControllers)
		{
			if (IsValid(NBPS) == true)
			{
				FString CombinedMessageString = NBPS->PlayerNameString + TEXT(" has WON THE GAME !!");
				NBPlayerController->NotificationText = FText::FromString(CombinedMessageString);

				ResetGame();
			}
		}
	}
	else
	{
		bool bIsDraw = true;
		for (const auto& NBPlayerController : AllPlayerControllers)
		{
			ANBPlayerState* NBPS = NBPlayerController->GetPlayerState<ANBPlayerState>();
			if (IsValid(NBPS) == true)
			{
				if (NBPS->CurrentGuessCount < NBPS->MaxGuessCount)
				{
					bIsDraw = false;
					break;
				}
			}
		}

		if (true == bIsDraw)
		{
			for (const auto& NBPlayerController : AllPlayerControllers)
			{
				NBPlayerController->NotificationText = FText::FromString(TEXT("DRAW..."));

				ResetGame();
			}
		}
	}
}

#include "NumBallGameMode.h"
#include "NumBallGameState.h"
#include "Player/NumBallPlayerController.h"
#include "Player/NumBallPlayerState.h"
#include "Engine/World.h"
#include "GameFramework/PlayerState.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NumBallGameMode)

ANumBallGameMode::ANumBallGameMode()
{
    GameStateClass = ANumBallGameState::StaticClass();
    PlayerStateClass = ANumBallPlayerState::StaticClass();
    PlayerControllerClass = ANumBallPlayerController::StaticClass();
}

void ANumBallGameMode::BeginPlay()
{
    Super::BeginPlay();
    ResetMatch();
}

void ANumBallGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);

    if (ANumBallGameState* GS = GetGameState<ANumBallGameState>())
    {
        GS->SyncOnPlayerJoin(Cast<ANumBallPlayerController>(NewPlayer));
        
        if (GS->HasAuthority())
        {
            if (GS->GetConnectedPlayers() >= GS->RequiredPlayers)
            {
                GenerateAnswer3Digits();
                GS->StartMatchOnce();
            }
        }
    }
}

void ANumBallGameMode::Logout(AController* Exiting)
{
    Super::Logout(Exiting);

    if (ANumBallGameState* GS = GetGameState<ANumBallGameState>())
    {
        if (GS->GetNumPlayersAlive() <= 1 && !GS->bMatchFinished)
        {
            ANumBallPlayerController* Winner = GS->FindAnyAlivePlayer();
            if (Winner != nullptr)
            {
                GS->MulticastAnnounce(FText::FromString(TEXT("남은 플레이어 승리")));
                GS->bMatchFinished = true;
                GetWorldTimerManager().SetTimerForNextTick(this, &ANumBallGameMode::ResetMatch);
            }
        }
    }
}

void ANumBallGameMode::HandleChatFromPlayer(ANumBallPlayerController* FromPC, const FString& Message)
{
    if (!HasAuthority() || FromPC == nullptr) { return; }

    ANumBallGameState* GS = GetGameState<ANumBallGameState>();
    if (GS == nullptr) { return; }

    FString SystemReply;
    const bool bWasGuess = TrySubmitGuess(FromPC, Message, SystemReply);
    
    FString AttemptsInfo;
    if (ANumBallPlayerState* PS = FromPC->GetPlayerState<ANumBallPlayerState>())
    {
        AttemptsInfo = FString::Printf(TEXT(" (%d/%d)"), PS->GetCurrentAttempts(), PS->GetMaxAttempts());
    }

    const FString Name = FromPC->PlayerState ? FromPC->PlayerState->GetPlayerName() : TEXT("Player");
    GS->MulticastChat(FString::Printf(TEXT("%s%s: %s"), *Name, *AttemptsInfo, *Message));

    if (!SystemReply.IsEmpty())
    {
        GS->MulticastChat(SystemReply);
    }

    if (bWasGuess && !GS->bMatchFinished)
    {
        AdvanceTurn(false);
    }
}

bool ANumBallGameMode::TrySubmitGuess(ANumBallPlayerController* FromPC, const FString& Message, FString& OutSystemReply)
{
    OutSystemReply.Empty();

    ANumBallGameState* GS = GetGameState<ANumBallGameState>();
    if (GS == nullptr || GS->bMatchFinished || !GS->bMatchActive)
    {
        OutSystemReply = TEXT("매치 준비 중입니다.");
        return false;
    }

    if (!GS->IsPlayersTurn(FromPC))
    {
        OutSystemReply = TEXT("자신의 턴이 아닙니다.");
        return false;
    }

    FString GuessDigits;
    if (!ExtractValidGuess(Message, GuessDigits))
    {
        return false;
    }

    if (ANumBallPlayerState* PS = FromPC->GetPlayerState<ANumBallPlayerState>())
    {
        if (!PS->CanAttempt())
        {
            OutSystemReply = TEXT("기회를 모두 사용했습니다.");
            return false;
        }
        PS->MarkActedThisTurn();
        PS->IncreaseAttempt();
    }

    const FJudgeResult Result = Judge(AnswerDigits, GuessDigits);
    OutSystemReply = FString::Printf(TEXT("▶ %s → %s"), *GuessDigits, *Result.ToResultString());

    EvaluateWinConditions(FromPC, Result);
    return true;
}

void ANumBallGameMode::ResetMatch()
{
    ANumBallGameState* GS = GetGameState<ANumBallGameState>();
    if (GS == nullptr) { return; }

    GenerateAnswer3Digits();
    GS->ResetForNewMatch();
    
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        if (ANumBallPlayerController* PC = Cast<ANumBallPlayerController>(*It))
        {
            if (ANumBallPlayerState* PS = PC->GetPlayerState<ANumBallPlayerState>())
            {
                PS->ResetAttempts();
            }
        }
    }
    
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        if (ANumBallPlayerController* PC = Cast<ANumBallPlayerController>(*It))
        {
            PC->ClientRefreshAttempts();
        }
    }

    GS->MulticastAnnounceTimed(FText::FromString(TEXT("새 게임 시작")), 1.5f);

    if (GS->GetConnectedPlayers() >= GS->RequiredPlayers)
    {
        GS->StartMatchOnce();
    }
}

void ANumBallGameMode::GenerateAnswer3Digits()
{
    TArray<int32> Pool;
    for (int32 i = 1; i <= 9; ++i) { Pool.Add(i); }

    FString NewAnswer;
    for (int32 i = 0; i < 3; ++i)
    {
        const int32 Index = FMath::RandRange(0, Pool.Num() - 1);
        NewAnswer += FString::FromInt(Pool[Index]);
        Pool.RemoveAt(Index);
    }
    AnswerDigits = NewAnswer;

    if (ANumBallGameState* GS = GetGameState<ANumBallGameState>())
    {
        GS->SetServerAnswer(AnswerDigits);
    }
}

bool ANumBallGameMode::ExtractValidGuess(const FString& Message, FString& OutDigits) const
{
    OutDigits.Empty();
    
    TArray<FString> Runs;
    int32 i = 0, N = Message.Len();
    while (i < N)
    {
        while (i < N && !FChar::IsDigit(Message[i])) { ++i; }
        if (i >= N) break;

        int32 j = i;
        while (j < N && FChar::IsDigit(Message[j])) { ++j; }
        Runs.Add(Message.Mid(i, j - i));
        i = j;
    }

    if (Runs.Num() == 0) return false;
    
    for (const FString& R : Runs)
    {
        if (R.Len() > 3)
        {
            return false;
        }
    }
    
    for (const FString& R : Runs)
    {
        if (R.Len() != 3) continue;

        const TCHAR A = R[0], B = R[1], C = R[2];
        
        if (!(A >= '1' && A <= '9' && B >= '1' && B <= '9' && C >= '1' && C <= '9'))
        {
            continue;
        }
        
        if (A == B || A == C || B == C) { continue; }
        
        OutDigits = R;
        return true;
    }

    return false;
}

FJudgeResult ANumBallGameMode::Judge(const FString& Answer, const FString& Guess) const
{
    FJudgeResult Res;

    if (Answer.Len() != 3 || Guess.Len() != 3)
    {
        Res.bOut = true;
        return Res;
    }

    for (int32 i = 0; i < 3; ++i)
    {
        if (Guess[i] == Answer[i])
        {
            ++Res.Strikes;
        }
        else
        {
            int32 DummyIdx = INDEX_NONE;
            if (Answer.FindChar(Guess[i], DummyIdx))
            {
                ++Res.Balls;
            }
        }
    }

    if (Res.Strikes == 0 && Res.Balls == 0)
    {
        Res.bOut = true;
    }
    return Res;
}

void ANumBallGameMode::EvaluateWinConditions(ANumBallPlayerController* LastActorPC, const FJudgeResult& Result)
{
    ANumBallGameState* GS = GetGameState<ANumBallGameState>();
    if (GS == nullptr) { return; }

    if (Result.Strikes == 3)
    {
        const FString WinnerName = LastActorPC && LastActorPC->PlayerState ? LastActorPC->PlayerState->GetPlayerName() : TEXT("Player");
        GS->MulticastAnnounceTimed(FText::FromString(FString::Printf(TEXT("%s 승리 (3S)"), *WinnerName)), 3.0f);
        GS->bMatchFinished = true;
        FTimerHandle Th;
        GetWorldTimerManager().SetTimer(Th, this, &ANumBallGameMode::ResetMatch, 3.0f, false);
        return;
    }

    if (GS->HaveAllPlayersExhausted())
    {
        GS->MulticastAnnounceTimed(FText::FromString(TEXT("무승부")), 3.0f);
        GS->bMatchFinished = true;
        FTimerHandle Th;
        GetWorldTimerManager().SetTimer(Th, this, &ANumBallGameMode::ResetMatch, 3.0f, false);
        return;
    }
}

void ANumBallGameMode::StartTurnTick()
{
    // no-op
}

void ANumBallGameMode::AdvanceTurn(bool bConsumeByTimeout)
{
    ANumBallGameState* GS = GetGameState<ANumBallGameState>();
    if (GS == nullptr || GS->bMatchFinished || !GS->bMatchActive) { return; }

    ANumBallPlayerController* Current = GS->GetCurrentTurnPlayer();
    if (Current != nullptr)
    {
        if (bConsumeByTimeout)
        {
            if (ANumBallPlayerState* PS = Current->GetPlayerState<ANumBallPlayerState>())
            {
                if (!PS->HasActedThisTurn())
                {
                    PS->IncreaseAttempt();
                }
            }
        }

        if (ANumBallPlayerState* PS = Current->GetPlayerState<ANumBallPlayerState>())
        {
            PS->ResetActedFlag();
        }
    }

    GS->MoveToNextTurn();
}

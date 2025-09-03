#include "NumBallGameState.h"

#include "NumBallGameMode.h"
#include "Player/NumBallPlayerController.h"
#include "Player/NumBallPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NumBallGameState)

ANumBallGameState::ANumBallGameState()
{
    bReplicates = true;
}

void ANumBallGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ANumBallGameState, bMatchFinished);
    DOREPLIFETIME(ANumBallGameState, bMatchActive);
    DOREPLIFETIME(ANumBallGameState, TurnTimeRemaining);
    DOREPLIFETIME(ANumBallGameState, CurrentTurnPS);
    DOREPLIFETIME(ANumBallGameState, TurnIndex);
}

void ANumBallGameState::OnRep_TurnTime() {}
void ANumBallGameState::OnRep_CurrentTurn() {}
void ANumBallGameState::OnRep_TurnIndex() {}

void ANumBallGameState::MulticastChat_Implementation(const FString& Line)
{
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        if (ANumBallPlayerController* PC = Cast<ANumBallPlayerController>(*It))
        {
            PC->ClientAppendChat(Line);
        }
    }
}

void ANumBallGameState::MulticastAnnounce_Implementation(const FText& Message)
{
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        if (ANumBallPlayerController* PC = Cast<ANumBallPlayerController>(*It))
        {
            PC->ClientShowNotice(Message);
        }
    }
}

void ANumBallGameState::ResetForNewMatch()
{
    bMatchFinished = false;
    bMatchActive   = false;
    bStartedOnce   = false;

    TurnIndex = 0;
    TurnTimeRemaining = 0.0f;

    GetWorldTimerManager().ClearTimer(TurnTimerHandle);
    RebuildPlayerListCache();
    CurrentTurnPS = nullptr;
}

bool ANumBallGameState::StartMatchOnce()
{
    if (!HasAuthority()) return false;
    if (bStartedOnce)    return false;
    if (GetConnectedPlayers() < RequiredPlayers) return false;

    bStartedOnce   = true;
    bMatchActive   = true;
    bMatchFinished = false;

    RebuildPlayerListCache();
    TurnIndex = 0;

    if (ANumBallPlayerController* PC = GetPCByIndexRoundRobin(TurnIndex))
    {
        CurrentTurnPS = PC->PlayerState;
    }
    else
    {
        CurrentTurnPS = nullptr;
    }

    GetWorldTimerManager().ClearTimer(TurnTimerHandle);
    TurnTimeRemaining = TurnSeconds;
    GetWorldTimerManager().SetTimer(
        TurnTimerHandle, this, &ANumBallGameState::TickTurnTimer, 1.0f, true);

    return true;
}

void ANumBallGameState::StartNewRound()
{
    if (!HasAuthority() || !bMatchActive) return;

    GetWorldTimerManager().ClearTimer(TurnTimerHandle);
    TurnTimeRemaining = TurnSeconds;
    GetWorldTimerManager().SetTimer(
        TurnTimerHandle, this, &ANumBallGameState::TickTurnTimer, 1.0f, true);
}

void ANumBallGameState::MoveToNextTurn()
{
    if (!HasAuthority() || !bMatchActive) return;

    ++TurnIndex;
    RebuildPlayerListCache();

    if (PlayerListCache.Num() == 0)
    {
        CurrentTurnPS = nullptr;
        return;
    }

    TurnIndex = TurnIndex % PlayerListCache.Num();

    if (ANumBallPlayerController* PC = GetPCByIndexRoundRobin(TurnIndex))
    {
        CurrentTurnPS = PC->PlayerState;
    }

    StartNewRound();
}

void ANumBallGameState::TickTurnTimer()
{
    if (!HasAuthority() || bMatchFinished || !bMatchActive)
    {
        GetWorldTimerManager().ClearTimer(TurnTimerHandle);
        return;
    }

    TurnTimeRemaining = FMath::Max(0.0f, TurnTimeRemaining - 1.0f);

    if (TurnTimeRemaining <= 0.0f)
    {
        if (ANumBallGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<ANumBallGameMode>() : nullptr)
        {
            GM->AdvanceTurn(true);
        }
    }
}

void ANumBallGameState::SyncOnPlayerJoin(ANumBallPlayerController* PC)
{
    if (PC == nullptr) { return; }
    PC->ClientSyncAfterJoin();
    // 시작은 GameMode(PostLogin) -> StartMatchOnce 에서만
}

bool ANumBallGameState::IsPlayersTurn(ANumBallPlayerController* PC) const
{
    if (PC == nullptr || CurrentTurnPS == nullptr) return false;
    return (PC->PlayerState == CurrentTurnPS);
}

ANumBallPlayerController* ANumBallGameState::GetCurrentTurnPlayer() const
{
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        if (ANumBallPlayerController* PC = Cast<ANumBallPlayerController>(*It))
        {
            if (PC->PlayerState == CurrentTurnPS) { return PC; }
        }
    }
    return nullptr;
}

int32 ANumBallGameState::GetNumPlayersAlive() const
{
    int32 Count = 0;
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        if (Cast<ANumBallPlayerController>(*It)) { ++Count; }
    }
    return Count;
}

ANumBallPlayerController* ANumBallGameState::FindAnyAlivePlayer() const
{
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        if (ANumBallPlayerController* PC = Cast<ANumBallPlayerController>(*It))
        {
            return PC;
        }
    }
    return nullptr;
}

bool ANumBallGameState::HaveAllPlayersExhausted() const
{
    int32 Players = 0;
    int32 Exhausted = 0;

    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        if (ANumBallPlayerController* PC = Cast<ANumBallPlayerController>(*It))
        {
            ++Players;
            if (ANumBallPlayerState* PS = PC->GetPlayerState<ANumBallPlayerState>())
            {
                if (!PS->CanAttempt()) { ++Exhausted; }
            }
        }
    }
    return (Players > 0 && Players == Exhausted);
}

void ANumBallGameState::SetServerAnswer(const FString& InAnswer)
{
    ServerAnswerHidden = InAnswer;
}

void ANumBallGameState::RebuildPlayerListCache()
{
    PlayerListCache.Empty();
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        if (ANumBallPlayerController* PC = Cast<ANumBallPlayerController>(*It))
        {
            PlayerListCache.Add(PC);
        }
    }
}

ANumBallPlayerController* ANumBallGameState::GetPCByIndexRoundRobin(int32 Index) const
{
    if (PlayerListCache.Num() == 0) { return nullptr; }
    const int32 SafeIndex = FMath::Clamp(Index, 0, PlayerListCache.Num() - 1);
    return PlayerListCache[SafeIndex].Get();
}

int32 ANumBallGameState::GetConnectedPlayers() const
{
    int32 Count = 0;
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        if (Cast<ANumBallPlayerController>(*It)) { ++Count; }
    }
    return Count;
}

void ANumBallGameState::MulticastAnnounceTimed_Implementation(const FText& Message, float DurationSeconds)
{
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        if (ANumBallPlayerController* PC = Cast<ANumBallPlayerController>(*It))
        {
            PC->ClientShowNoticeTimed(Message, DurationSeconds);
        }
    }
}
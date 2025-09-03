#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "NumBallGameState.generated.h"

class ANumBallPlayerController;

UCLASS()
class ANumBallGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	ANumBallGameState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	UPROPERTY(Replicated)
	bool bMatchFinished { false };
	
	UPROPERTY(Replicated)
	bool bMatchActive { false };

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Match")
	int32 RequiredPlayers { 2 };
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Turn")
	float TurnSeconds { 20.0f };

	UPROPERTY(ReplicatedUsing = OnRep_TurnTime)
	float TurnTimeRemaining { 0.0f };

	UFUNCTION()
	void OnRep_TurnTime();

	UPROPERTY(ReplicatedUsing = OnRep_CurrentTurn)
	APlayerState* CurrentTurnPS { nullptr };

	UFUNCTION()
	void OnRep_CurrentTurn();
	
	UFUNCTION(NetMulticast, Reliable)
	void MulticastChat(const FString& Line);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastAnnounce(const FText& Message);
	
	void ResetForNewMatch();
	
	bool StartMatchOnce();
	
	void MoveToNextTurn();
	
	void SyncOnPlayerJoin(ANumBallPlayerController* PC);
	
	bool IsPlayersTurn(ANumBallPlayerController* PC) const;
	ANumBallPlayerController* GetCurrentTurnPlayer() const;
	int32 GetNumPlayersAlive() const;
	ANumBallPlayerController* FindAnyAlivePlayer() const;
	bool HaveAllPlayersExhausted() const;
	
	void SetServerAnswer(const FString& InAnswer);
	
	int32 GetConnectedPlayers() const;

	UFUNCTION(NetMulticast, Reliable)
	void MulticastAnnounceTimed(const FText& Message, float DurationSeconds); 

private:
	FTimerHandle TurnTimerHandle;
	
	FString ServerAnswerHidden;

	void TickTurnTimer();
	ANumBallPlayerController* GetPCByIndexRoundRobin(int32 Index) const;
	
	UPROPERTY(ReplicatedUsing = OnRep_TurnIndex)
	int32 TurnIndex { 0 };

	UFUNCTION()
	void OnRep_TurnIndex();
	
	TArray<TWeakObjectPtr<ANumBallPlayerController>> PlayerListCache;

	void RebuildPlayerListCache();
	
	bool bStartedOnce { false };
	
	void StartNewRound();
};

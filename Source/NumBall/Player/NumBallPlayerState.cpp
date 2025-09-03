#include "NumBallPlayerState.h"
#include "Player/NumBallPlayerController.h"
#include "GameFramework/PlayerController.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NumBallPlayerState)

ANumBallPlayerState::ANumBallPlayerState()
{
	bReplicates = true;
}

void ANumBallPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANumBallPlayerState, MaxAttempts);
	DOREPLIFETIME(ANumBallPlayerState, CurrentAttempts);
	DOREPLIFETIME(ANumBallPlayerState, bActedThisTurn);
}

void ANumBallPlayerState::IncreaseAttempt()
{
	if (CurrentAttempts < MaxAttempts)
	{
		++CurrentAttempts;
	}
}

void ANumBallPlayerState::ResetAttempts()
{
	CurrentAttempts = 0;
	bActedThisTurn = false;
}

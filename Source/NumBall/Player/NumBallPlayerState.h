#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "NumBallPlayerState.generated.h"

UCLASS()
class ANumBallPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	ANumBallPlayerState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, Category = "Attempts")
	int32 GetMaxAttempts() const { return MaxAttempts; }

	UFUNCTION(BlueprintCallable, Category = "Attempts")
	int32 GetCurrentAttempts() const { return CurrentAttempts; }

	UFUNCTION(BlueprintCallable, Category = "Attempts")
	bool CanAttempt() const { return CurrentAttempts < MaxAttempts; }

	void IncreaseAttempt();
	void ResetAttempts();

	void MarkActedThisTurn() { bActedThisTurn = true; }
	void ResetActedFlag() { bActedThisTurn = false; }
	bool HasActedThisTurn() const { return bActedThisTurn; }

private:
	UPROPERTY(Replicated)
	int32 MaxAttempts { 3 };

	UPROPERTY(Replicated)
	int32 CurrentAttempts { 0 };

	UPROPERTY(Replicated)
	bool bActedThisTurn { false };
};

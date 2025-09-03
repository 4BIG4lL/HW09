#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "NumBallPlayerController.generated.h"

class UChatWidget;
class UNoticeWidget;
class UTurnTimerWidget;

UCLASS()
class ANumBallPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ANumBallPlayerController();

	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	
	UFUNCTION(Server, Reliable)
	void ServerSendChat(const FString& Message);
	
	UFUNCTION(Client, Reliable)
	void ClientAppendChat(const FString& Line);
	
	UFUNCTION(Client, Reliable)
	void ClientShowNotice(const FText& Message);
	
	UFUNCTION(Client, Reliable)
	void ClientSyncAfterJoin();

	UFUNCTION(Client, Reliable)
	void ClientShowNoticeTimed(const FText& Message, float DurationSeconds);
	
	UFUNCTION(BlueprintCallable)
	void SubmitChatFromUI(const FString& Message);

	UFUNCTION(Client, Reliable)
	void ClientRefreshAttempts();
	
	UFUNCTION(Client, Reliable)
	void ClientSetRemainingAttempts(int32 Remaining);
	
	UFUNCTION()
	void ForceUpdateAttemptsLocal();

protected:
	void EnsureWidgets();

	UPROPERTY(EditDefaultsOnly, Category="UI")
	TSubclassOf<class UChatWidget> ChatWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category="UI")
	TSubclassOf<class UNoticeWidget> NoticeWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category="UI")
	TSubclassOf<class UTurnTimerWidget> TurnWidgetClass;

private:
	UPROPERTY()
	UChatWidget* ChatWidget { nullptr };

	UPROPERTY()
	UNoticeWidget* NoticeWidget { nullptr };

	UPROPERTY()
	UTurnTimerWidget* TurnWidget { nullptr };

	void UpdateAttemptsText();

	FTimerHandle NoticeTimerHandle;
	
	void HideNotice();
	
	FString LastAppendedChatLine;
};

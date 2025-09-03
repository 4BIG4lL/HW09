#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ChatWidget.generated.h"

class UScrollBox;
class UEditableTextBox;
class UTextBlock;
class ANumBallPlayerController;

UCLASS(BlueprintType)
class UChatWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UChatWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	void InitForPC(ANumBallPlayerController* InPC);

	UFUNCTION(BlueprintCallable)
	void AppendLine(const FString& Line);

	UFUNCTION(BlueprintCallable)
	void SetAttempts(const FString& InText);

	UFUNCTION(BlueprintCallable)
	void FocusInput();

protected:
	virtual void NativeConstruct() override;

	UFUNCTION()
	void OnTextCommitted(const FText& Text, ETextCommit::Type CommitMethod);

protected:
	UPROPERTY(meta = (BindWidget))
	UScrollBox* MessagesBox;

	UPROPERTY(meta = (BindWidget))
	UEditableTextBox* InputBox;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* AttemptsText;

private:
	UPROPERTY()
	ANumBallPlayerController* OwningPC { nullptr };

	void AddMessageText(const FString& Line);
};

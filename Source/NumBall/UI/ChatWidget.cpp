#include "UI/ChatWidget.h"
#include "Components/ScrollBox.h"
#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"
#include "Player/NumBallPlayerController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ChatWidget)

UChatWidget::UChatWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UChatWidget::InitForPC(ANumBallPlayerController* InPC)
{
	OwningPC = InPC;
}

void UChatWidget::FocusInput()
{
	if (InputBox)
	{
		InputBox->SetUserFocus(OwningPC);
		InputBox->SetKeyboardFocus();
	}
}

void UChatWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (InputBox)
	{
		InputBox->OnTextCommitted.RemoveAll(this);
		InputBox->OnTextCommitted.AddDynamic(this, &UChatWidget::OnTextCommitted);
	}

	if (AttemptsText)
	{
		AttemptsText->SetText(FText::FromString(TEXT("남은 기회: 3")));
	}
}

void UChatWidget::OnTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	if (CommitMethod == ETextCommit::OnEnter)
	{
		const FString Msg = Text.ToString();
		if (OwningPC != nullptr)
		{
			OwningPC->SubmitChatFromUI(Msg);
		}
		if (InputBox)
		{
			InputBox->SetText(FText::GetEmpty());
		}
	}
}

void UChatWidget::AppendLine(const FString& Line)
{
	AddMessageText(Line);
}

void UChatWidget::SetAttempts(const FString& InText)
{
	if (AttemptsText)
	{
		AttemptsText->SetText(FText::FromString(InText));
	}
}

void UChatWidget::AddMessageText(const FString& Line)
{
	if (MessagesBox == nullptr)
	{
		return;
	}

	UTextBlock* NewText = NewObject<UTextBlock>(this);
	NewText->SetText(FText::FromString(Line));
	MessagesBox->AddChild(NewText);
	MessagesBox->ScrollToEnd();
}

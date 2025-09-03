#include "NumBallPlayerController.h"
#include "UI/ChatWidget.h"
#include "UI/NoticeWidget.h"
#include "UI/TurnTimerWidget.h"
#include "Blueprint/UserWidget.h"
#include "Player/NumBallPlayerState.h"
#include "Game/NumBallGameMode.h"
#include "Game/NumBallGameState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NumBallPlayerController)

ANumBallPlayerController::ANumBallPlayerController()
{
    bReplicates = true;
}

void ANumBallPlayerController::BeginPlay()
{
    Super::BeginPlay();

    bShowMouseCursor = true;
    DefaultMouseCursor = EMouseCursor::Default;

    FInputModeGameAndUI Mode;
    Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    Mode.SetHideCursorDuringCapture(false);
    SetInputMode(Mode);

    if (IsLocalController())
    {
        EnsureWidgets();
    }
}

void ANumBallPlayerController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);
    if (IsLocalController())
    {
        EnsureWidgets();
    }
}

void ANumBallPlayerController::EnsureWidgets()
{
    if (ChatWidget == nullptr && ChatWidgetClass)
    {
        ChatWidget = CreateWidget<UChatWidget>(this, ChatWidgetClass);
        if (ChatWidget)
        {
            ChatWidget->AddToViewport(0);
            ChatWidget->InitForPC(this);
            ChatWidget->FocusInput();
        }
    }

    if (NoticeWidget == nullptr && NoticeWidgetClass)
    {
        NoticeWidget = CreateWidget<UNoticeWidget>(this, NoticeWidgetClass);
        if (NoticeWidget)
        {
            NoticeWidget->AddToViewport(1);
        }
    }

    if (TurnWidget == nullptr && TurnWidgetClass)
    {
        TurnWidget = CreateWidget<UTurnTimerWidget>(this, TurnWidgetClass);
        if (TurnWidget)
        {
            TurnWidget->AddToViewport(2);
        }
    }

    UpdateAttemptsText();
}

void ANumBallPlayerController::ServerSendChat_Implementation(const FString& Message)
{
    ANumBallGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<ANumBallGameMode>() : nullptr;
    if (!GM) { return; }

    GM->HandleChatFromPlayer(this, Message);
}

void ANumBallPlayerController::ClientAppendChat_Implementation(const FString& Line)
{
    if (Line == LastAppendedChatLine)
    {
        return;
    }
    LastAppendedChatLine = Line;

    if (ChatWidget != nullptr)
    {
        ChatWidget->AppendLine(Line);
    }
    UpdateAttemptsText();
}

void ANumBallPlayerController::ClientShowNotice_Implementation(const FText& Message)
{
    if (NoticeWidget != nullptr)
    {
        NoticeWidget->ShowMessage(Message);
    }
}

void ANumBallPlayerController::ClientSyncAfterJoin_Implementation()
{
    EnsureWidgets();
}

void ANumBallPlayerController::SubmitChatFromUI(const FString& Message)
{
    if (Message.IsEmpty())
    {
        return;
    }
    ServerSendChat(Message);
}

void ANumBallPlayerController::UpdateAttemptsText()
{
    ANumBallPlayerState* PS = GetPlayerState<ANumBallPlayerState>();
    if (PS && ChatWidget)
    {
        const int32 Remaining = PS->GetMaxAttempts() - PS->GetCurrentAttempts();
        const FString Text = FString::Printf(TEXT("남은 기회: %d"), Remaining);
        ChatWidget->SetAttempts(Text);
    }
}

void ANumBallPlayerController::ClientRefreshAttempts_Implementation()
{
    UpdateAttemptsText();
}

void ANumBallPlayerController::ClientShowNoticeTimed_Implementation(const FText& Message, float DurationSeconds)
{
    if (!NoticeWidget) return;
    NoticeWidget->ShowMessage(Message);
    
    GetWorldTimerManager().ClearTimer(NoticeTimerHandle);
    if (DurationSeconds > 0.f)
    {
        GetWorldTimerManager().SetTimer(NoticeTimerHandle, this, &ANumBallPlayerController::HideNotice, DurationSeconds, false);
    }
}

void ANumBallPlayerController::ClientSetRemainingAttempts_Implementation(int32 Remaining)
{
    if (ChatWidget)
    {
        const FString Text = FString::Printf(TEXT("남은 기회: %d"), Remaining);
        ChatWidget->SetAttempts(Text);
    }
}

void ANumBallPlayerController::ForceUpdateAttemptsLocal()
{
    UpdateAttemptsText();
}

void ANumBallPlayerController::HideNotice()
{
    if (NoticeWidget)
    {
        NoticeWidget->ShowMessage(FText::GetEmpty());
    }
}
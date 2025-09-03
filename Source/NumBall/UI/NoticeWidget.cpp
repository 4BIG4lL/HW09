#include "UI/NoticeWidget.h"
#include "Components/TextBlock.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NoticeWidget)

void UNoticeWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (NoticeText)
	{
		NoticeText->SetText(FText::FromString(TEXT("")));
	}
}

void UNoticeWidget::ShowMessage(const FText& InMessage)
{
	if (NoticeText)
	{
		NoticeText->SetText(InMessage);
	}
}

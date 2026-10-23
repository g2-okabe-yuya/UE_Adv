#include "UGameOverWidget.h"
#include "InGameHUD.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"

void UUGameOverWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ButtonContinue)
	{
		ButtonContinue->OnClicked.AddUniqueDynamic(this, &UUGameOverWidget::OnButtonContinueClicked);
	}

	if (ButtonTitle)
	{
		ButtonTitle->OnClicked.AddUniqueDynamic(this, &UUGameOverWidget::OnButtonTitleClicked);
	}
}

// Continueボタンが押されたときの処理
void UUGameOverWidget::OnButtonContinueClicked()
{
	// 一時停止を解除
	UGameplayStatics::SetGamePaused(GetWorld(), false);

	// 現在のレベル名をを取得してリロード
	FString CurrentLevelName = UGameplayStatics::GetCurrentLevelName(GetWorld());
	UGameplayStatics::OpenLevel(GetWorld(), FName(*CurrentLevelName));
}

// Titleに戻る
void UUGameOverWidget::OnButtonTitleClicked()
{
	UGameplayStatics::SetGamePaused(GetWorld(), false);

	// タイトルレベルを開く
	UGameplayStatics::OpenLevel(GetWorld(), FName(TEXT("Title")));
}
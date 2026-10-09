#include "UGameOverWidget.h"
#include "Kismet/GameplayStatics.h"
#include "InGameHUD.h"
#include "Components/Button.h"

void UUGameOverWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	// ButtonContinueのOnClickedに「OnButtonContinueClicked」を関連づける
	//ButtonContinue->OnClicked.AddUniqueDynamic(this, &UUGameOverWidget::OnButtonContinueClicked);
	// ButtonTitleのOnClickedに「OnButtonTitleClicked」を関連づける
}

// Continueボタンが押されたときの処理
void UUGameOverWidget::OnButtonContinueClicked()
{
	// PlayerControllerを取得する
	// const APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	//
	// // InGameHUDクラスを取得する
	// UInGameHUD* HUD = Cast<UInGameHUD>(PlayerController->GetHUD());
	//
	// // Gameを再開する
	// HUD->ContinueGame();
}
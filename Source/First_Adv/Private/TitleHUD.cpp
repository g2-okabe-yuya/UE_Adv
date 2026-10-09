#include "TitleHUD.h" 
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

void UTitleHUD::NativeConstruct()
{
	Super::NativeConstruct();
	
	// ButtonPlayのOnClickedに「OnButtonPlayClicked」を関連づける
	ButtonPlay->OnClicked.AddUniqueDynamic(this,&UTitleHUD::OnButtonPlayClicked);
	
	// ButtonのQuitのOnClickedに「OnButtonQuitClicked」を関連づける
	ButtonQuit->OnClicked.AddUniqueDynamic(this,&UTitleHUD::OnButtonQuitClicked);
}

void UTitleHUD::OnButtonPlayClicked()
{
	// ボタンを押したらシーンを遷移させる
	UGameplayStatics::OpenLevel(GetWorld(),FName(TEXT("InGame")));
}

void UTitleHUD::OnButtonQuitClicked()
{
	// PlayerControllerを取得する
	if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(),0))
	{
		// ゲームを終了する
		UKismetSystemLibrary::QuitGame(GetWorld(), PlayerController, EQuitPreference::Quit, false);
	}
}
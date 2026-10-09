
#include "Kismet/GameplayStatics.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "TitleMenuHUD.h"
#include "Blueprint/UserWidget.h"

void ATitleMenuHUD::BeginPlay()
{
	// WidgetBlueprintのClassを取得する
	FString Path = TEXT("/Game/BluePrints/UI/BP_Title.BP_Title_C");
	TSubclassOf<UUserWidget> WidgetClass = TSoftClassPtr<UUserWidget>(FSoftObjectPath(*Path)).LoadSynchronous();
	
	// PlayerControllerを取得する
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	
	// WidgetClassとPlayerControllerが取得できたか判定する
	if (WidgetClass && PlayerController)
	{
		// Widgetを作成する
		UUserWidget* UserWidget = UWidgetBlueprintLibrary::Create(GetWorld(),WidgetClass,PlayerController);
		
		// Viewportに追加する
		UserWidget->AddToViewport(0);
		
		// MouseCursorを表示する
		UWidgetBlueprintLibrary::SetInputMode_GameAndUIEx(PlayerController,UserWidget,EMouseLockMode::DoNotLock,true
			,false);
		PlayerController->SetShowMouseCursor(true);
	}
	
	if (WidgetClass)
	{
		// 成功確認: WidgetClass が正常に取得できているか
		UE_LOG(LogTemp, Log, TEXT("WidgetClass の取得に成功しました: %s"), *WidgetClass->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("WidgetClass の取得に失敗しました"));
	}
}

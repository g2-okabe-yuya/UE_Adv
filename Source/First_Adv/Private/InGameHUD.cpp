#include "InGameHUD.h"

#include "GameManager.h"
#include "Components/Button.h"
#include "InputCoreTypes.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"

// スコープ解決演算子
// クラスの定義と実体を結びつける
void UInGameHUD::NativeConstruct()
{
	// Super = base
	Super::NativeConstruct();
	// キー入力を受け付けるようにする
	//SetIsFocusable(true);
	
	// Button_AdvのOnClickedにOnButtonClickedを結びつける
	//Adv_Button->OnClicked.AddUniqueDynamic(this, &UAdvHUD::OnAdvButtonClicked);
	
	// if (UGameInstance* GameInstance = GetGameInstance())
	// {
	// 	// nullチェックを行い、公開関数を呼び出す
	// 	if (UAdvSubSystem* SubSystem = GameInstance->GetSubsystem<UAdvSubSystem>())
	// 	{
	// 		// シナリオを呼び出す
	// 		if (SubSystem->IsDataLoaded())
	// 		{
	// 			OnScenarioDataLoadSuccess();
	// 		}
	// 		else
	// 		{
	// 			// 通知が飛んで来たら処理を実行する
	// 			SubSystem->OnDataLoaded.AddUObject(this,&UAdvHUD::OnScenarioDataLoadSuccess);
	// 		}
	// 	}
	// }
}

void UInGameHUD::ShowGameOverPanel()
{
	// WidgetBlueprintのClassを取得する
	FString GameOverWidgetPath = TEXT("Game/BluePrints/UI/BPW_GameOver.BPW_GameOver");
	TSubclassOf<UUserWidget> GameOverWidgetClass = TSoftClassPtr<UUserWidget>(FSoftObjectPath(*GameOverWidgetPath)).LoadSynchronous();
	
	// PlayerControllerを取得する
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(),0);
	
	if (GameOverWidgetClass && PlayerController)
	{
		// GameOver用のWidgetを作成する
		GameOverWidget = UWidgetBlueprintLibrary::Create(GetWorld(),GameOverWidgetClass,PlayerController);
		
		// GameOverメニューを折りたたみ状態にする
		GameOverWidget->SetVisibility(ESlateVisibility::Collapsed);
		
		// ViewPortに追加する
		GameOverWidget->AddToViewport(0);
	}
}

void UInGameHUD::DispGameOver()
{
	// GameOverWidgetを表示する
	GameOverWidget->SetVisibility(ESlateVisibility::Visible);
	
	// PlayerControllerを取得する
	APlayerController* PlayerController = GetOwningPlayer();
	
	// UIモードに設定する
	UWidgetBlueprintLibrary::SetInputMode_UIOnlyEx(PlayerController,GameOverWidget,EMouseLockMode::DoNotLock,false);
	
	// GameをPause状態にする
	UGameplayStatics::SetGamePaused(GetWorld(),true);
	
	// Mouseカーソルを表示する
	PlayerController->SetShowMouseCursor(true);
}

// コンティニュー処理
void UInGameHUD::ContinueGame()
{
	// AGameManager* GameManager = Cast<AGameManager>(AGameManager::GetGameInstance());
	//
	// GameManager->Initialize();
	//
	// // 現在のレベルネームを取得する
	// const FString CurrentLevelName = UGameplayStatics::GetCurrentLevelName(GetWorld());
	//
	// // 現在のレベルを開きなおす
	// UGameplayStatics::OpenLevel(GetWorld(),FName(*CurrentLevelName));
}

// SubSystemから通知が飛んで来たらよびだされる関数
void UInGameHUD::OnScenarioDataLoadSuccess()
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UAdvSubSystem* SubSystem = GameInstance->GetSubsystem<UAdvSubSystem>())
		{
			ScenarioData = SubSystem->GetAllScenarioData();
			UE_LOG(LogTemp, Log, TEXT("[AdvHUD] 受け取ったシナリオデータ数: %d 件"), ScenarioData.Num());
			// シナリオの呼び出し
			ShowScenario("SO001_01");
		}
	}
}

// 次のシナリオを表示する
void UInGameHUD::ShowScenario(FName ScenarioID)
{
	// 該当する行のポインタを保持する変数
	const FScenarioRowType* FoundRow = nullptr;
	int32 FoundIndex = INDEX_NONE;
	
	if (!ScenarioID.IsNone())
	{
		for (int32 i = 0; i < ScenarioData.Num(); ++i)
		{
			if (ScenarioData[i].TextID == ScenarioID.ToString())
			{
				FoundRow = &ScenarioData[i]; 
				FoundIndex = i;              
				break;               
			}
		}
	}
	
	// ScenarioIDから該当する行を検索する
	// iのDialogueがほしい
	/*
	for (const FScenarioRowType& Row : ScenarioData)
	{
			*Row.TextID.ToString(), *ScenarioID.ToString());
		if (Row.TextID == ScenarioID.ToString())
		{
			// 対象の行を見つけたらアドレスを保存しておく
			FoundRow = &Row;
			break;
		}
	}
	*/
	
	// NextIDの記載(ジャンプ先がなければ次のダイアログを表示する)
	if (FoundRow == nullptr)
	{
		int32 NextIndex = CurrentScenarioIndex + 1;
		if (ScenarioData.IsValidIndex(NextIndex))
		{
			FoundRow = &ScenarioData[NextIndex];
			FoundIndex = NextIndex;
			UE_LOG(LogTemp, Log, TEXT("[AdvHUD] NextID未記載、次の行 (Index: %d) を表示"), NextIndex);
		}
	}
	
	// UI更新
	if (FoundRow != nullptr)
	{
		CurrentScenarioIndex = FoundIndex;
		CurrentScenarioID = FName(*FoundRow->NextID.ToString());
		
		if (Dialogue != nullptr)
		{
			// 本文を表示
			Dialogue->SetText(FoundRow->Dialogue);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[AdvHUD] 本文を取得できませんでした"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[AdvHUD] シナリオの最後です"));
		CurrentScenarioID = NAME_None;
	}
}

//　テキスト送りを検知する
FReply UInGameHUD::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() ==EKeys::LeftMouseButton)
	{
		OnAdvMouseClicked();
		return FReply::Handled();
	}
	
	return FReply::Unhandled();
}

// 左マウスクリック時に次のシナリオを表示する
void UInGameHUD::OnAdvMouseClicked()
{
	if (!CurrentScenarioID.IsNone() || ScenarioData.IsValidIndex(CurrentScenarioIndex + 1))
	{
		ShowScenario(CurrentScenarioID);
	}
}
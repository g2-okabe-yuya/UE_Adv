#include "AdvHUD.h"
#include "Components/Button.h"
#include "InputCoreTypes.h"
#include "Components/TextBlock.h"

// スコープ解決演算子
// クラスの定義と実体を結びつける
void UAdvHUD::NativeConstruct()
{
	// Super = base
	Super::NativeConstruct();
	// キー入力を受け付けるようにする
	SetIsFocusable(true);
	
	// Button_AdvのOnClickedにOnButtonClickedを結びつける
	//Adv_Button->OnClicked.AddUniqueDynamic(this, &UAdvHUD::OnAdvButtonClicked);
	
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		// nullチェックを行い、公開関数を呼び出す
		if (UAdvSubSystem* SubSystem = GameInstance->GetSubsystem<UAdvSubSystem>())
		{
			// シナリオを呼び出す
			if (SubSystem->IsDataLoaded())
			{
				OnScenarioDataLoadSuccess();
			}
			else
			{
				// 通知が飛んで来たら処理を実行する
				SubSystem->OnDataLoaded.AddUObject(this,&UAdvHUD::OnScenarioDataLoadSuccess);
			}
		}
	}
}

// SubSystemから通知が飛んで来たらよびだされる関数
void UAdvHUD::OnScenarioDataLoadSuccess()
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
void UAdvHUD::ShowScenario(FName ScenarioID)
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
FReply UAdvHUD::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() ==EKeys::LeftMouseButton)
	{
		OnAdvMouseClicked();
		return FReply::Handled();
	}
	
	return FReply::Unhandled();
}

// 左マウスクリック時に次のシナリオを表示する
void UAdvHUD::OnAdvMouseClicked()
{
	if (!CurrentScenarioID.IsNone() || ScenarioData.IsValidIndex(CurrentScenarioIndex + 1))
	{
		ShowScenario(CurrentScenarioID);
	}
}
#include "AdvSubSystem.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Kismet/KismetSystemLibrary.h"

// 初期化時にDataTableを読み込む
void UAdvSubSystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	// 指定したパスをアセットとしてロードするための処理
	FString strPath = TEXT("/Game/DataTable/ScenarioData.ScenarioData");
	// スマートポインタ
	// アドレスだけを保持し、使用時までメモリへのロードをおこなわない
	// DataTableをPathで指定して取得する
	ScenarioDataTable = TSoftObjectPtr<UDataTable>(FSoftObjectPath(strPath));
	
	// --- デバッグログ①：設定したパスの確認 ---
	FSoftObjectPath SoftPath = ScenarioDataTable.ToSoftObjectPath();
	FString PathString = SoftPath.ToString();
	FString AssetName = SoftPath.GetAssetName();
	
	// 非同期ロードのリクエストを開始する
	RequestAsyncLoadDataTable();
}

// データテーブルの非同期ロードをリクエストする関数
void UAdvSubSystem::RequestAsyncLoadDataTable()
{
	// DataTableが有効であるか確認
	if (!ScenarioDataTable.IsValid())
	{
		// 無効
		UE_LOG(LogTemp,Warning,TEXT("DataTableの読み込みに失敗しました"))
		// 画面およびログ出力ウィンドウに赤い警告を10秒間表示する
		UKismetSystemLibrary::PrintString(this,TEXT("Error:DataTableのセットがされていません"),true,true,FLinearColor::Red,10.f);
		return;
	}
	
	// すでにロード済みであるかをチェックする
	if (ScenarioDataTable.Get() != nullptr)
	{
		UE_LOG(LogTemp,Log,TEXT("DataTableのLoadが完了しています"))
		OnDataTableLoaded();
		return;
	}
	
	// UEのアセット管理を一括管理しているシステム
	FStreamableManager& StreamableManager = UAssetManager::GetStreamableManager();
	
	// 非同期でのロードをリクエストする
	// AssetのPath情報を取得する
	StreamableManager.RequestAsyncLoad(ScenarioDataTable.ToSoftObjectPath(),
		// デリゲートの作成
		// 引数に呼び出したいコールバック関数を登録
		FStreamableDelegate::CreateUObject(this,&UAdvSubSystem::OnDataTableLoaded));
	
	FString Msg = FString::Printf(TEXT("DataTableの読み込みを開始します(%s)"), *ScenarioDataTable.ToString());
	
	UKismetSystemLibrary::PrintString(this,Msg,true,true,FLinearColor::Yellow,5.f);
}

// DataTableのロードが完了した際にStreamingManagerによって呼び出される関数
void UAdvSubSystem::OnDataTableLoaded() const
{
	UE_LOG(LogTemp,Log,TEXT("DataTableのロードが完了しました"));
	
	// UObjectでポインタを取得
	UDataTable* LoadedDataTable = ScenarioDataTable.Get();
	
	// ポインタチェック
	if (LoadedDataTable == nullptr)
	{
		// 無効時
		UE_LOG(LogTemp,Error,TEXT("ポインタが無効です"))
		UKismetSystemLibrary::PrintString(this,TEXT("Error:非同期でのデータテーブルの取得に失敗しました"),true
			,true,FLinearColor::Red,10.f);
		return;
	}
	
	const FString ContextString = TEXT("DataTableのロードに成功しました");
	
	// 型チェック
	if (ensureMsgf(LoadedDataTable->GetRowStruct() == FScenarioRowType::StaticStruct()
		,TEXT("DataTableの型が一致していません")))
	{
		// すべての行名を取得(今回の場合はTextID)
		TArray<FName> RowNames = LoadedDataTable->GetRowNames();
		
		for (const FName RowName : RowNames)
		{
			// IDでFindをかけて列を取得してくる
			const FScenarioRowType* RowData = LoadedDataTable-> FindRow<FScenarioRowType>(RowName,ContextString,false);

			if (!RowData)
			{
				continue;
			}
			
			// シナリオの列すべてを出力する
			const FString DebugString = FString::Printf(
				TEXT("[Load成功] RowName: %s |SpeakerName:%s | Dialogue: %s | NextID: %s" ),
			*RowName.ToString(),*RowData->SpeakerName.ToString(),*RowData->Dialogue.ToString(),*RowData->NextID.ToString());
			
			UKismetSystemLibrary::PrintString(this,DebugString,true,true,FLinearColor::Green,15.f);
		}
	}
}
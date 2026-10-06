#include "AdvSubSystem.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Kismet/KismetSystemLibrary.h"

// 初期化時にDataTableを読み込む
// void UAdvSubSystem::Initialize(FSubsystemCollectionBase& Collection)
// {
// 	Super::Initialize(Collection);
// 	bIsDataLoaded  = false;
// 	// 指定したパスをアセットとしてロードするための処理
// 	FString strPath = TEXT("/Game/DataTable/ScenarioData.ScenarioData");
// 	// スマートポインタ
// 	// アドレスだけを保持し、使用時までメモリへのロードをおこなわない
// 	// DataTableをPathで指定して取得する
// 	ScenarioDataTable = TSoftObjectPtr<UDataTable>(FSoftObjectPath(strPath));
// 	// 非同期ロードのリクエストを開始する
// 	RequestAsyncLoadDataTable();
// }

// データテーブルの非同期ロードをリクエストする関数
void UAdvSubSystem::RequestAsyncLoadDataTable()
{
	// すでにロード済みであるかをチェックする
	if (ScenarioDataTable.Get() != nullptr)
	{
		UE_LOG(LogTemp,Log,TEXT("DataTableのLoadが完了しています"))
		OnDataTableLoaded();
		return;
	}
	
	if (ScenarioDataTable.ToSoftObjectPath().IsNull())
	{
		UE_LOG(LogTemp, Warning, TEXT("ScenarioDataTable のパスが設定されていません"));
		UKismetSystemLibrary::PrintString(this, TEXT("Error: DataTableのパスが不適切です"), true, true, FLinearColor::Red, 10.f);
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
void UAdvSubSystem::OnDataTableLoaded()
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

			if (RowData != nullptr)
			{
				FScenarioRowType TempRow = *RowData;
				TempRow.TextID = RowName;
				// 配列に格納する
				// ポインタを配列にAddするとポインタ型が格納される
				CachedScenarioData.Add(TempRow);
			}
		}
		
		bIsDataLoaded = true;
		// データ読み込みが終了したことを通知する
		OnDataLoaded.Broadcast();
	}
}
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "ScenarioRowType.h"
#include "Components/Image.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AdvSubSystem.generated.h"

// デリゲート型の宣言
// マルチキャスト = 複数のクラスが同時にBindできる
DECLARE_MULTICAST_DELEGATE(FOnScenarioDataLoaded);

// Scenario管理を行うサブシステム
// 今回はレベル単位ではなくGame中に常駐してほしいのでSubSystemを継承する
UCLASS()
class FIRST_ADV_API UAdvSubSystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	// 外部航海用の公開関数
	// UIにデータを流し込むなどの用途
	UFUNCTION(BlueprintCallable,Category = "AdvSystem")
	TArray<FScenarioRowType> GetAllScenarioData() const
	{
		return CachedScenarioData;
	}
	
	// 外部公開用の通知変数
	FOnScenarioDataLoaded OnDataLoaded;
	
	// シナリオデータのロードが終了したかどうかを判定する
	bool IsDataLoaded() const
	{
		return bIsDataLoaded;
	}
	
	// 初期化
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	
	// シナリオ読み込みをおこなう
private:
	EScenarioRowType RowType;
	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<UDataTable> ScenarioDataTable;
	UPROPERTY()
	TObjectPtr<UImage> FadeImage = nullptr;
	// Cacheしたデータを保持しておく変数
	UPROPERTY(EditAnywhere)
	TArray<FScenarioRowType> CachedScenarioData;
	
	bool bIsDataLoaded = false;
	// データテーブルの非同期ロードをリクエストする関数
	void RequestAsyncLoadDataTable();
	
	// 非同期ロードが完了した際にStreamableManagerによって呼び出されるコールバック関数
	void OnDataTableLoaded();
};

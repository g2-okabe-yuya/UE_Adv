#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "ScenarioRowType.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AdvSubSystem.generated.h"

// Scenario管理を行うサブシステム
// 今回はレベル単位ではなくGame中に常駐してほしいのでSubSystemを継承する
UCLASS()
class FIRST_ADV_API UAdvSubSystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	// 初期化
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	
	// シナリオ読み込みをおこなう
private:
	EScenarioRowType RowType;
	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<UDataTable> ScenarioDataTable;
	
	// データテーブルの非同期ロードをリクエストする関数
	void RequestAsyncLoadDataTable();
	
	// 非同期ロードが完了した際にStreamableManagerによって呼び出されるコールバック関数
	void OnDataTableLoaded() const;
};

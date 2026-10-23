#pragma once

#include "CoreMinimal.h"
#include "AGoalTrigger.h"
#include "SplineMapCreator.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerStart.h"
#include "GameManager.generated.h"

class UInGameHUD;
class UFadeAnimation;
// ゲームの進行を管理する
UCLASS()
class FIRST_ADV_API AGameManager : public AGameModeBase
{
	GENERATED_BODY()
	
public:
	AGameManager();
	// 初期化
	void Initialize();
protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
private:
	// 生成したバイク
	UPROPERTY(Transient)
	APawn* SpawnedPawn = nullptr;
	UPROPERTY()
	ASplineMapCreator* MapCreator = nullptr;
	UPROPERTY()
	APlayerStart* PlayerStart = nullptr;
	UPROPERTY()
	UInGameHUD* InGameHUD = nullptr;
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UInGameHUD> InGameHUDClass;
	UPROPERTY()
	TObjectPtr<AGoalTrigger> GoalTrigger;
	FTimerHandle CountdownDelayTimerHandle;
	// 自作タイマー用変数
	bool IsTimerRunning = false;
	float ElapsedTime = 0.0f;
	// ゲーム開始時の実行順序を操作する
	// マップ生成
	// PlayerStartの配置
	// Playerを配置
	// UI配置
	// カウントダウン
	void GameStart();
	void GameClear();
	void OnFinishCountdown();
	void OnStartTimer();
	void OnContinueRequested();
	void OnTitleRequested();
	APawn* SpawnAndGetPlayer();
	void OnFadeInComplete();
	void StartCountdownWithDelay();
};

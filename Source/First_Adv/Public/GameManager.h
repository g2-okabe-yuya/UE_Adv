#pragma once

#include "CoreMinimal.h"
#include "BikeMovement.h"
#include "SplineMapCreator.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerStart.h"
#include "GameManager.generated.h"

// ゲームの進行を管理する
UCLASS()
class FIRST_ADV_API AGameManager : public AGameModeBase
{
	GENERATED_BODY()
	
public:
	virtual void BeginPlay() override;
	// 初期化
	void Initialize();

private:
	// 生成したバイク
	UPROPERTY(Transient)
	APawn* SpawnedPawn;
	UPROPERTY()
	ASplineMapCreator* MapCreator= nullptr;
	UPROPERTY()
	APlayerStart* PlayerStart = nullptr;
	// ゲーム開始時の実行順序を操作する
	// マップ生成
	// PlayerStartの配置
	// Playerを配置
	// UI配置
	// カウントダウン
	void GameStart();
	APawn* SpawnAndGetPlayer();
};

#pragma once

#include "AdvSubsystem.h"
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InGameHUD.generated.h"

class UTextBlock;
// Widgetを実行時に操作するクラス 
UCLASS()
class FIRST_ADV_API UInGameHUD : public UUserWidget
{
	GENERATED_BODY()
	
public:
	// Widgetの初期化処理を行うライフサイクル関数
	// UIが画面に生成されたときに一回呼び出される
	virtual void NativeConstruct() override;
	void ShowGameOverPanel();
	void DispGameOver();
	void ContinueGame();
	
	// マウスクリックを検知する
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Dialogue;

	
private:
	//UFUNCTION()
	//void OnAdvButtonClicked();
	// ロードしたシナリオを表示する
	void ShowScenario(FName ScenarioID);
	UPROPERTY()
	TArray<FScenarioRowType> ScenarioData;
	// 次に表示するシナリオの番号
	FName* NextID;
	// Scenarioのロードが完了した通知がきたときに実行する
	void OnScenarioDataLoadSuccess();
	// 現在表示しているシナリオID
	FName CurrentScenarioID;
	int32 CurrentScenarioIndex = -1;
	// テキスト送りが実行されたときに呼び出される
	UFUNCTION()
	void OnAdvMouseClicked();
	
	// GameOverWidgetを保持する変数
	UPROPERTY()
	UUserWidget* GameOverWidget;
	
	// WBP上に配置した「画面全体を覆うボタン」または
	
	// WBPに配置したButtonを完全一致で検索しポインタ変数に結びつける
	// UPROPERTYとは →　プロパティ指定子とメタデータ指定子を定義することができる
	// UPROPERTY(プロパティ指定子、メタデータ)
	//UPROPERTY(EditAnywhere, meta = (BindWidget))
	//class UButton* Adv_Button = nullptr;
};

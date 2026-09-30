#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AdvHUD.generated.h"

// Widgetを実行時に操作するクラス
UCLASS()
class FIRST_ADV_API UAdvHUD : public UUserWidget
{
	GENERATED_BODY()
	
public:
	// Widgetの初期化処理を行うライフサイクル関数
	// UIが画面に生成されたときに一回呼び出される
	virtual void NativeConstruct() override;
	
private:
	UFUNCTION()
	void OnAdvButtonClicked();
	
	// WBPに配置したButtonを完全一致で検索しポインタ変数に結びつける
	// UPROPERTYとは →　プロパティ指定子とメタデータ指定子を定義することができる
	// UPROPERTY(プロパティ指定子、メタデータ)
	UPROPERTY(EditAnywhere, meta = (BindWidget))
	class UButton* Adv_Button = nullptr;
};

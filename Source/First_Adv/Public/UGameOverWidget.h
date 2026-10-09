
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UGameOverWidget.generated.h"

class UButton;
UCLASS()
class FIRST_ADV_API UUGameOverWidget : public UUserWidget
{
	GENERATED_BODY()
	
protected:
	// NativeConstruct
	void NativeConstruct() override;
	
private:
	// ゲームをもう一度遊ぶ
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ButtonContinue;
	// タイトルに戻る
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ButtonTitle;
	
	UFUNCTION()
	// ContinueボタンにOnClickedのイベントをバインド
	void OnButtonContinueClicked();
	
	// ButtonTitleのOnClickedイベントに関連づける
	//UFUNCTION()
	//void OnButtonTitleClicked();
};


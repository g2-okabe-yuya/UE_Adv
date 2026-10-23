#pragma once

#include "CoreMinimal.h"
#include "BikeMovement.h"
#include "FadeAnimation.h"
#include "Blueprint/UserWidget.h"
#include "Components/BackgroundBlur.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "InGameHUD.generated.h"

// 前方宣言
class AGameManager;
class UTextBlock;
class UUGameOverWidget;

// 相互参照解消のためデリゲートで依存性逆転
DECLARE_MULTICAST_DELEGATE(FOnCountdownFinishedDelegate);
DECLARE_MULTICAST_DELEGATE(FOnContinueRequestedDelegate);
DECLARE_MULTICAST_DELEGATE(FOnTimerStartedDelegate);
DECLARE_MULTICAST_DELEGATE(FOnTitleRequestedDelegate);
// Widgetを実行時に操作するクラス 
UCLASS()
class FIRST_ADV_API UInGameHUD : public UUserWidget
{
	GENERATED_BODY()
	
public:
	void UpdateCurrentSpeedMeter(float CurrentSpeed) const;
	void UpdateCurrentGear(int32 CurrentGear) const;
	void UpdateCurrentRpm(float CurrentRpm) const;
	void DisplayGameClear(float ClearTime);
	void DisplayGameOver();
	void StartCountdown();
	
	FOnCountdownFinishedDelegate OnCountdownFinished;
	FOnTimerStartedDelegate OnTimerStarted;
	FOnContinueRequestedDelegate OnContinueRequested;
	FOnTitleRequestedDelegate OnTitleRequested;
	
	UPROPERTY(EditAnywhere, Category = "FadeAnimation")
	TSubclassOf<UFadeAnimation> FadeAnimationClass;
	
protected:
	// Widgetの初期化処理を行うライフサイクル関数
	// UIが画面に生成されたときに一回呼び出される
	virtual void NativeConstruct() override;
	void NativeTick(const FGeometry& MyGeometry, float InDeltaTime);

private:
	// ゲームをもう一度遊ぶ
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> GameOverContinueButton;
	// タイトルに戻る
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> GameOverTitleButton;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> GameClearContinueButton;
	// タイトルに戻る
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> GameClearTitleButton;
	int32 CountdownTimer;
	UFUNCTION()
	// ContinueボタンにOnClickedのイベントをバインド
	void OnButtonContinueClicked();
	
	// ButtonTitleのOnClickedイベントに関連づける
	UFUNCTION()
	void OnButtonTitleClicked();
	// バイクのスピード通知を購読する
	void BindToBikeMovement();
	void UpdateCountDown();
	void OnBikeDead(bool IsDead);
	
	// GameOverWidgetを保持する変数
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBackgroundBlur> GameResultBlur;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> GameClearBorder;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> GameOverBorder;
	// スピードUI
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CurrentSpeedText;
	ABikeMovement* TargetBike;
	// 回転数UI
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> NeedleImage;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CurrentGearText;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CountdownText;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ClearTimeText;
	UPROPERTY(Transient, meta = (BindWidgetAnim))
	TObjectPtr<UWidgetAnimation> PopAnimation;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UOverlay> Tachometer;
	FTimerHandle CountdownTimerHandle;
	UPROPERTY(EditAnywhere,Category="Countdown")
	int32 CountdownValue = 3.0;
	
	// 回転数用の針を動かすためのパラメーター
	UPROPERTY(EditAnywhere, Category = "TachoMeter")
	float MinRPM = 0.0f;
	UPROPERTY(EditAnywhere, Category = "TachoMeter")
	float MaxRPM = 18000.0f;
	UPROPERTY(EditAnywhere, Category = "TachoMeter")
	float MinAngle = -120.0f;
	UPROPERTY(EditAnywhere, Category = "TachoMeter")
	float MaxAngle = 120.0f;
	UPROPERTY(EditAnywhere,Category="TachoMeter")
	float NeedleInterpSpeed = 10.0f;
	UPROPERTY(EditAnywhere, Category = "TachoMeter")
	float VibrationIntensity = 1.5f;
	// 針の現在の角度
	float CurrentNeedleAngle = 0.0f;
	// 針の目指す目標角度
	float TargetNeedleAngle = 0.0f;
};

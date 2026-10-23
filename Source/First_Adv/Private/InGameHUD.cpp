#include "InGameHUD.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"

// スコープ解決演算子
// クラスの定義と実体を結びつける
void UInGameHUD::NativeConstruct()
{
	Super::NativeConstruct();
	// GameOverPanelを非アクティブ化
	if (GameResultBlur)
	{
		GameResultBlur->SetVisibility(ESlateVisibility::Collapsed);
	}
	
	// ボタンに遷移処理をバインド
	GameClearTitleButton->OnClicked.AddUniqueDynamic(this, &UInGameHUD::OnButtonTitleClicked);
	GameClearContinueButton->OnClicked.AddUniqueDynamic(this, &UInGameHUD::OnButtonContinueClicked);
	GameOverContinueButton->OnClicked.AddUniqueDynamic(this, &UInGameHUD::OnButtonContinueClicked);
	GameOverTitleButton->OnClicked.AddUniqueDynamic(this, &UInGameHUD::OnButtonTitleClicked);
	CountdownTimer = CountdownValue;
	Tachometer->SetVisibility(ESlateVisibility::Visible);
	BindToBikeMovement();
}

void UInGameHUD::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (NeedleImage)
	{
		// 現在の角度から目標角度へ滑らかに補間 (FInterpTo)
		CurrentNeedleAngle = FMath::FInterpTo(CurrentNeedleAngle, TargetNeedleAngle, InDeltaTime, NeedleInterpSpeed);
		// 画像の回転角を更新
		NeedleImage->SetRenderTransformAngle(CurrentNeedleAngle);
	}
}

// Continueボタンが押されたときの処理
void UInGameHUD::OnButtonContinueClicked()
{
	if (OnContinueRequested.IsBound())
	{
		OnContinueRequested.Broadcast();
	}
}

// Titleに戻る
void UInGameHUD::OnButtonTitleClicked()
{
	if (OnTitleRequested.IsBound())
	{
		OnTitleRequested.Broadcast();
	}
}

// 1秒ごとにカウントダウンを更新する
void UInGameHUD::UpdateCountDown()
{
	if (!CountdownText || !PopAnimation)
	{
		return;
	}
	
	if (CountdownTimer > 0)
	{
		// 数字テキストを設定
		CountdownText->SetText(FText::AsNumber(CountdownTimer));
		
		// Animation再生
		PlayAnimation(PopAnimation);
		
		CountdownTimer--;
		
		GetWorld()->GetTimerManager().SetTimer(CountdownTimerHandle,this,&UInGameHUD::UpdateCountDown,1.0f, false);
	}
	else if (CountdownTimer == 0)
	{
		// 「GO!」の表示
		CountdownText->SetText(FText::FromString(TEXT("GO!")));
		PlayAnimation(PopAnimation);
		
		// リザルト用のタイマーをスタートさせる
		if (OnTimerStarted.IsBound())
		{
			OnTimerStarted.Broadcast();
		}

		CountdownTimer--;

		// バイクの操作ロックを解除する
		if (OnCountdownFinished.IsBound())
		{
			OnCountdownFinished.Broadcast();
		}
		
		// 1.0秒後にカウントダウンテキストを非表示にする
		GetWorld()->GetTimerManager().SetTimer(CountdownTimerHandle,FTimerDelegate::CreateLambda([this]
			{
				if (CountdownText)
				{
					CountdownText->SetVisibility(ESlateVisibility::Collapsed);
				}
			}),1.0f,false);
	}
}

// カウントダウンと開始
void UInGameHUD::StartCountdown()
{
	// タイマー数値を設定値に設定する
	CountdownTimer = CountdownValue;
	if (CountdownText)
	{
		CountdownText->SetVisibility(ESlateVisibility::Visible);
	}
	
	// カウントダウンを再生する
	UpdateCountDown();
}

// GameClear時にUIを表示する
void UInGameHUD::DisplayGameClear(float ClearTime)
{
	if (!GameOverBorder)
	{
		return;
	}
	Tachometer->SetVisibility(ESlateVisibility::Collapsed);
	// GameOver用のBorderを表示、GameClear用のBorderを非表示
	if (GameClearBorder)
	{
		GameClearBorder->SetVisibility(ESlateVisibility::Visible);
	}
	if (GameOverBorder)
	{
		// GameOverのBorderは非表示にする
		GameOverBorder->SetVisibility(ESlateVisibility::Collapsed);
	}
	
	// タイムテキストを非表示
	if (ClearTimeText)
	{
		// タイムテキストを表示する
		int32 Minutes = FMath::FloorToInt(ClearTime / 60.0f);
		int32 Seconds = FMath::FloorToInt(FMath::Fmod(ClearTime, 60.0f));
		int32 Millis  = FMath::FloorToInt(FMath::Fmod(ClearTime * 100.0f, 100.0f));

		FString FormattedTime = FString::Printf(TEXT("Time : %02d:%02d.%02d"), Minutes, Seconds, Millis);
		ClearTimeText->SetText(FText::FromString(FormattedTime));
		ClearTimeText->SetVisibility(ESlateVisibility::Visible);
	}
	
	GameResultBlur->SetVisibility(ESlateVisibility::Visible);
	
	APlayerController* PlayerController = GetOwningPlayer();
	if (!PlayerController)
	{
		PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	}

	if (PlayerController)
	{
		UWidgetBlueprintLibrary::SetInputMode_UIOnlyEx(PlayerController, nullptr, EMouseLockMode::DoNotLock, false);
		UGameplayStatics::SetGamePaused(GetWorld(), true);
		PlayerController->SetShowMouseCursor(true);
	}
}

// GameOver時にUIを表示する
void UInGameHUD::DisplayGameOver()
{
	if (GameResultBlur)
	{
		// 非表示だったGameOverBlurを表示状態にする
		Tachometer->SetVisibility(ESlateVisibility::Collapsed);
		GameResultBlur->SetVisibility(ESlateVisibility::Visible);
		GameOverBorder->SetVisibility(ESlateVisibility::Visible);
		GameClearBorder->SetVisibility(ESlateVisibility::Collapsed);

		APlayerController* PlayerController = GetOwningPlayer();
		if (!PlayerController)
		{
			PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
		}

		if (PlayerController)
		{
			// UI操作モードに変更し、ポーズ＆マウスカーソル表示
			UWidgetBlueprintLibrary::SetInputMode_UIOnlyEx(PlayerController, nullptr, EMouseLockMode::DoNotLock, false);
			UGameplayStatics::SetGamePaused(GetWorld(), true);
			PlayerController->SetShowMouseCursor(true);
		}
	}
}

// バイクのイベントをバインドする
void UInGameHUD::BindToBikeMovement()
{
	// PlayerControllerを取得
	APlayerController* PlayerController = GetOwningPlayer();
	if (!PlayerController)
	{
		PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	}

	// Levelに配置されているバイクを取得する
	ABikeMovement* Bike = nullptr;
	// Playerが操作しているBikeを指定
	if (PlayerController)
	{
		Bike = Cast<ABikeMovement>(PlayerController->GetPawn());
	}
	
	// Playerがバイクを操作していなかった場合はWorldにあるバイクを取得する
	if (!Bike)
	{
		Bike = Cast<ABikeMovement>(UGameplayStatics::GetActorOfClass(GetWorld(), ABikeMovement::StaticClass()));
	}
	
	if (Bike)
	{
		// バイクを取得することができたらターゲットに設定する
		TargetBike = Bike;
		// 購読処理
		// スピードが変化したときにUIを更新する
		Bike->OnSpeedChanged.AddUObject(this, &UInGameHUD::UpdateCurrentSpeedMeter);
		// エンジン回転数が変化したときUIを更新する
		Bike->OnRpmChanged.AddUObject(this, &UInGameHUD::UpdateCurrentRpm);
		// ギアが変化したときにギア更新処理を呼び出す
		Bike->OnGearTextChanged.AddUObject(this, &UInGameHUD::UpdateCurrentGear);
		// バイクが落下したらゲームオーバーを呼び出す
		Bike->OnDead.AddUObject(this, &UInGameHUD::OnBikeDead);
	}
	else
	{
		FTimerHandle RetryTimerHandle;
		GetWorld()->GetTimerManager().SetTimer(RetryTimerHandle, this, &UInGameHUD::BindToBikeMovement, 0.1f, false);
	}
}

// バイクが死んだら呼び出す関数
void UInGameHUD::OnBikeDead(bool IsDead)
{
	if (IsDead)
	{
		DisplayGameOver();
	}
}

// 回転数の表示
// 回転数に比例してギアのスライダーも変化させる
void UInGameHUD::UpdateCurrentRpm(float CurrentRpm) const
{
	if (!NeedleImage)
	{
		return;
	}
    
	// 回転数を0.0～1.0に正規化
	float Alpha = FMath::Clamp((CurrentRpm - MinRPM) / (MaxRPM - MinRPM), 0.0f, 1.0f);
    
	// 基本の目標角度を計算
	float BaseTargetAngle = FMath::Lerp(MinAngle, MaxAngle, Alpha);

	// 回転数が高いほど震えを大きくする
	float Jitter = FMath::RandRange(-VibrationIntensity, VibrationIntensity) * Alpha;

	// 震えを加算した最終的な目標角度を設定
	const_cast<UInGameHUD*>(this)->TargetNeedleAngle = BaseTargetAngle + Jitter;
}

// 現在のSpeedをUIに更新する
void UInGameHUD::UpdateCurrentSpeedMeter(float CurrentSpeed) const
{
	// floatをテキスト型にして更新
	CurrentSpeedText->SetText(FText::AsNumber(CurrentSpeed));
}

// 現在のギアを更新する
void UInGameHUD::UpdateCurrentGear(int32 CurrentGear) const
{
	CurrentGearText->SetText(FText::Format(FText::FromString(TEXT("{0}")),FText::AsNumber(CurrentGear)));
}
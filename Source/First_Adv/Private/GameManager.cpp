#include "GameManager.h"
#include "BikeMovement.h"
#include "InGameHUD.h"
#include "FadeAnimation.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Kismet/GameplayStatics.h"

AGameManager::AGameManager()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AGameManager::BeginPlay()
{
	UGameplayStatics::SetGamePaused(GetWorld(), false);
	Initialize();
	GameStart();
}

// ゲーム実行時の初期化はここにまとめる
void AGameManager::Initialize()
{
	DefaultPawnClass = nullptr;
	IsTimerRunning = false;
	ElapsedTime = 0.0f;
	
	// リトライ時のポーズ解除と入力モード復元
	UGameplayStatics::SetGamePaused(GetWorld(), false);
	
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (PlayerController)
	{
		// 入力モードを切り替える
		// カーソル戻す
		UWidgetBlueprintLibrary::SetInputMode_GameOnly(PlayerController);
		PlayerController->SetShowMouseCursor(false);
	}
	// UEの標準機能でPlayerを生成する仕組みをOffにする
	if (!MapCreator)
	{
		MapCreator = Cast<ASplineMapCreator>(UGameplayStatics::GetActorOfClass(GetWorld(), ASplineMapCreator::StaticClass()));
	}
	
	// Bikeが実行されるたびに生成されるのを防ぐ
	TArray<AActor*> ExistingBikes;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABikeMovement::StaticClass(), ExistingBikes);

	for (AActor* BikeActor : ExistingBikes)
	{
		if (IsValid(BikeActor))
		{
			BikeActor->Destroy();
		}
	}
}

void AGameManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	// タイマーの計算
	if (IsTimerRunning)
	{
		ElapsedTime += DeltaTime;
	}
}

// 実行時のゲーム準備
void AGameManager::GameStart()
{
	// ゲーム開始時の実行順序を操作する
	// マップ生成
	MapCreator->CreateSplineLongMap();
	PlayerStart = Cast<APlayerStart>(UGameplayStatics::GetActorOfClass(GetWorld(), APlayerStart::StaticClass()));
	// Playerを配置
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (PlayerController)
	{
		PlayerController->Possess(SpawnAndGetPlayer());
	}
	
	InGameHUD = CreateWidget<UInGameHUD>(GetWorld(), InGameHUDClass);
	if (InGameHUD)
	{
		InGameHUD->AddToViewport();
		
		// 各種イベントへのバインド
		InGameHUD->OnCountdownFinished.AddUObject(this, &AGameManager::OnFinishCountdown);
		InGameHUD->OnContinueRequested.AddUObject(this, &AGameManager::OnContinueRequested);
		InGameHUD->OnTitleRequested.AddUObject(this, &AGameManager::OnTitleRequested);
		InGameHUD->OnTimerStarted.AddUObject(this, &AGameManager::OnStartTimer);
	}
	
	// FadeからCountDown
	if (InGameHUD->FadeAnimationClass)
	{
		TObjectPtr<UFadeAnimation> FadeWidget = CreateWidget<UFadeAnimation>(GetWorld(), InGameHUD->FadeAnimationClass);
		if (FadeWidget)
		{
			FadeWidget->AddToViewport(10);
			FadeWidget->OnFadeInComplete.AddUObject(this,&AGameManager::OnFadeInComplete);
			FadeWidget->StartFadeAnimation();
		}
	}
	else
	{
		InGameHUD->StartCountdown();
	}
	
	// ワールド上に置かれているGoalTriggerアクターを取得
	GoalTrigger = Cast<AGoalTrigger>(UGameplayStatics::GetActorOfClass(GetWorld(), AGoalTrigger::StaticClass()));
	
	if (GoalTrigger)
	{
		// ゴール到達イベントにGameManagerのOnGameClearを登録
		GoalTrigger->OnGoalReached.AddUObject(this,&AGameManager::GameClear);
	}
}

// カウントダウンが完了したのでバイク操作を可能にする
void AGameManager::OnFinishCountdown()
{
	if (ABikeMovement* Bike = Cast<ABikeMovement>(SpawnedPawn))
	{
		Bike->IsCountdown = false;
	}
}

// レーススタート時にリザルト用タイマーの計算をはじめる
void AGameManager::OnStartTimer()
{
	ElapsedTime = 0.0f;
	IsTimerRunning = true;
}

// HUDからのコンティニューを呼び出す
void AGameManager::OnContinueRequested()
{
	UGameplayStatics::SetGamePaused(GetWorld(), false);
	
	// ポーズ解除やれbる再読み込みなどのゲーム進行を管理
	const FString CurrentLevelName = UGameplayStatics::GetCurrentLevelName(GetWorld());
	UGameplayStatics::OpenLevel(GetWorld(), FName(*CurrentLevelName));
}

// HUDからのタイトル遷移要求を処理
void AGameManager::OnTitleRequested()
{
	UGameplayStatics::SetGamePaused(GetWorld(), false);
	UGameplayStatics::OpenLevel(GetWorld(), FName(TEXT("Title")));
}

void AGameManager::GameClear()
{
	// タイマーの測定を停止
	IsTimerRunning = false;
	// バイクの走行・アクセル入力をロックする
	if (ABikeMovement* Bike = Cast<ABikeMovement>(SpawnedPawn))
	{
		// 物理入力を無効化
		Bike->IsCountdown = true;
	}
	
	if (InGameHUD)
	{
		// クリアタイムをリザルトに渡す
		InGameHUD->DisplayGameClear(ElapsedTime);
	}
}

APawn* AGameManager::SpawnAndGetPlayer()
{
    FVector SpawnLocation = PlayerStart->GetActorLocation();
	SpawnLocation.X += 10.0f;
	SpawnLocation.Y += 1000.0f;
	SpawnLocation.Z += 50.0f;
    FRotator RawRotation = PlayerStart->GetActorRotation();

    FActorSpawnParameters SpawnParams;
    // 重なりによるキャンセルを防ぐために AlwaysSpawn を指定
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	
    AActor* SpawnedActor = GetWorld()->SpawnActor<AActor>(
        MapCreator->BikeActor,
        SpawnLocation,
        RawRotation,
        SpawnParams
    );

    // APawn へのキャスト確認
    SpawnedPawn = Cast<APawn>(SpawnedActor);
	
    return SpawnedPawn;
}

// Fadeが完了したことを通知する関数
void AGameManager::OnFadeInComplete()
{
	GetWorld()->GetTimerManager().SetTimer(CountdownDelayTimerHandle,this,&AGameManager::StartCountdownWithDelay,0.2f,false);
}

void AGameManager::StartCountdownWithDelay()
{
	if (InGameHUD)
	{
		InGameHUD->StartCountdown();
	}
}
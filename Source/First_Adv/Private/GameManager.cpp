#include "GameManager.h"

#include "Kismet/GameplayStatics.h"

void AGameManager::BeginPlay()
{
	Initialize();
	GameStart();
}

// ゲーム実行時の初期化はここにまとめる
void AGameManager::Initialize()
{
	DefaultPawnClass = nullptr;
	// UEの標準機能でPlayerを生成する仕組みをOffにする
	if (!MapCreator)
	{
		MapCreator = Cast<ASplineMapCreator>(UGameplayStatics::GetActorOfClass(GetWorld(), ASplineMapCreator::StaticClass()));
	}
	
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

// 実行時のゲーム準備
void AGameManager::GameStart()
{
	// ゲーム開始時の実行順序を操作する
	// マップ生成
	//MapCreator->GenerateSplineMap();
	PlayerStart = Cast<APlayerStart>(UGameplayStatics::GetActorOfClass(GetWorld(), APlayerStart::StaticClass()));
	// Playerを配置
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (PlayerController)
	{
		PlayerController->Possess(SpawnAndGetPlayer());
	}
	// UI配置
	// カウントダウン
}

APawn* AGameManager::SpawnAndGetPlayer()
{
    // 4. Transform と Collision 設定
    FVector SpawnLocation = PlayerStart->GetActorLocation();
	SpawnLocation.Z += 50.0f;
    FRotator SpawnRotation = PlayerStart->GetActorRotation();
	UE_LOG(LogTemp, Warning, TEXT("SpawnRotation : %s"), *SpawnRotation.ToString());

    FActorSpawnParameters SpawnParams;
    // 重なりによるキャンセルを防ぐために AlwaysSpawn を指定
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    // 5. スポーン実行 (AActor で受けて型変換)
    AActor* SpawnedActor = GetWorld()->SpawnActor<AActor>(
        MapCreator->BikeActor,
        SpawnLocation,
        SpawnRotation,
        SpawnParams
    );

    // APawn へのキャスト確認
    SpawnedPawn = Cast<APawn>(SpawnedActor);
	
    return SpawnedPawn;
}
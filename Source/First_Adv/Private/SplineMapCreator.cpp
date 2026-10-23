#include "SplineMapCreator.h"
#include "MyVector3D.h"
#include "Components/SplineMeshComponent.h"
#include "GameFramework/PlayerStart.h"

#include "CoreMinimal.h"
#include "Kismet/GameplayStatics.h"

// Editor設定をしてくれる
ASplineMapCreator::ASplineMapCreator()
{
	PrimaryActorTick.bCanEverTick = true;
	SplineComponent = CreateDefaultSubobject<USplineComponent>(TEXT("SplineComponent"));
	SplineComponent->SetupAttachment(RootComponent);

}

void ASplineMapCreator::BeginPlay()
{
	Super::BeginPlay();
}

void ASplineMapCreator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// 長方形に長い道を作成する
void ASplineMapCreator::CreateSplineLongMap()
{
	if (!SplineComponent || !RoadMesh || RoadMeshLength <= 0.0f)
	{
		return;
	}

	// Splineの長さを取得する
	const float SplineLength = SplineComponent->GetSplineLength();
	// Local座標に配置をおこなう
	const ESplineCoordinateSpace::Type CoordSpace = ESplineCoordinateSpace::Local;
	// RoadLength を 0 からスタートし、Spline の全体長さに達するまでループ
	float CurrentDistance = 0.0f;

	// 生成したMeshの長さがSplineの長さを超えるまでループする
	while (CurrentDistance < SplineLength)
	{
		// 始点と終点の距離計算
		float StartDistance = CurrentDistance;
		// スプラインの終点を超えないように調整
		float EndDistance = FMath::Min(CurrentDistance + RoadMeshLength, SplineLength);
		// このセグメントの実際の長さ(最後の端数区間にも対応)
		float SegmentLength = EndDistance - StartDistance;
		
		if (SegmentLength <= SMALL_NUMBER)
		{
			break;
		}
		
		// 新しいActorを宣言
		USplineMeshComponent* SplineMeshComponent = NewObject<USplineMeshComponent>(this);
		if (!SplineMeshComponent)
		{
			UE_LOG(LogTemp, Warning, TEXT("SplineMeshComponent is NULL"));
			break;
		}
		// UEに認識・描画してもらう
		SplineMeshComponent->SetupAttachment(SplineComponent);
		SplineMeshComponent->SetStaticMesh(RoadMesh);
		// メッシュの伸びる軸を指定
		SplineMeshComponent->SetForwardAxis(ESplineMeshAxis::X);
		SplineMeshComponent->SetMobility(EComponentMobility::Movable);
		// ワールド登録を行う
		SplineMeshComponent->RegisterComponentWithWorld(GetWorld());
		
		// 進んだ位置の3次元座標を計算
		FVector StartPos = SplineComponent->GetLocationAtDistanceAlongSpline(StartDistance, CoordSpace);
		// 方向ベクトルに対してセグメントの長さを乗算した接線を作成
		FVector StartTangent = SplineComponent->GetDirectionAtDistanceAlongSpline(StartDistance, CoordSpace) * SegmentLength;
		FVector EndPos = SplineComponent->GetLocationAtDistanceAlongSpline(EndDistance, CoordSpace);
		FVector EndTangent = SplineComponent->GetDirectionAtDistanceAlongSpline(EndDistance, CoordSpace) * SegmentLength;
		
		// マップのねじれを防止する
		FVector StartUp = SplineComponent->GetUpVectorAtDistanceAlongSpline(StartDistance,CoordSpace);
		FVector EndUp = SplineComponent->GetUpVectorAtDistanceAlongSpline(EndDistance, CoordSpace);
		SplineMeshComponent->SetSplineUpDir(StartUp, false);
		
		// 横幅などの設定
		SplineMeshComponent->SetStartScale(RoadScale,false);
		SplineMeshComponent->SetEndScale(RoadScale, false);
		// Posの決定
		SplineMeshComponent->SetStartAndEnd(StartPos, StartTangent, EndPos, EndTangent, true);
		// Collisionを有効化する
		SplineMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		// Collisionのレイヤーを決定する
		// 一旦普通の設定ゲームのボリュームが増えてきたらEditorで触るから書くことなさそう
		SplineMeshComponent->SetCollisionObjectType(ECC_WorldStatic);
		SplineMeshComponent->SetCollisionResponseToAllChannels(ECR_Block);
		
		SplineMeshComponent->RegisterComponentWithWorld(GetWorld());
		CurrentDistance += RoadMeshLength;
	}
}

// 最初の点にPlayerStartをおいてゲーム開始時にマップが生成されたタイミングで一緒に生成
// そのPlayerStartを置かれた後にPlayerを置きたい
// そこをスタート位置にして一周したらゴールの仕組みにしたい
// バイクの配置みたいにPlayerを一番前にして敵AIを斜め後ろに配置して自動でPlayerの後ろに追従してもらうようにしたい
FMyVector3D ASplineMapCreator::GetStartPos() const
{
	return *GameStartPos;
}

// SplineMeshComponentを使用した簡単な道路の生成
// 当たり判定うやUEに認識してもらうためのお作法もある
void ASplineMapCreator::CreateStaticMeshComponent(const FVector& StartPos, const FVector& EndPos, const FVector& StartTangent, const FVector& EndTangent)
{
	// SplineMeshComponentの生成
	if (USplineMeshComponent* SplineMesh = NewObject<USplineMeshComponent>(this))
	{
		// 生成するMeshをmovableに設定する
		SplineMesh->SetMobility(EComponentMobility::Movable);
		// UEに認識してもらう(物理挙動や描画の有効化)
		// これないとなにもしてくれない
		SplineMesh->RegisterComponent();
		//SplineMesh->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
		// メッシュと曲げ変形の設定
		SplineMesh->SetStaticMesh(RoadMesh);
		//SplineMesh->SetForwardAxis(ESplineMeshAxis::X);
		// 始点と終点をと接線を渡す
		SplineMesh->SetStartAndEnd(StartPos, StartTangent, EndPos, EndTangent, true);

		// 道幅
		SplineMesh->SetStartScale(RoadScale);
		SplineMesh->SetEndScale(RoadScale);
		// 衝突判定
		SplineMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}
}

// 指定した点を打つための関数
// 点の座標が格納された配列を返す
TArray<FMyVector3D> ASplineMapCreator::AddControlPoint() const
{
	// 制御点の配列を作成する
	TArray<FMyVector3D> Points;
	// 制御点を作成し配列に格納する
	for (int32 i = 0; i < ControlPoint; ++i)
	{
		// 分割角度の計算
		// θ = i * 2π / N(制御点の総数)
		float Angle = i * (2.0f * 3.14159265358979323846f / ControlPoint);
		// 直交座標の計算
		float X = Radius * FMath::Cos(Angle);
		float Y = Radius * FMath::Sin(Angle);
		Points.Add(FMyVector3D(X, Y, 0.0f));
	}
	
	return Points;
}

// Start地点にStartPointを作成する
void ASplineMapCreator::CreateStartPoint(FMyVector3D StartPosition,FMyVector3D NextPosition)
{
	TArray<AActor*> StartPoints;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), StartPoints);

	for (AActor* StartPoint : StartPoints)
	{
		if (IsValid(StartPoint))
		{
			StartPoint->Destroy();
		}
	}
	// スタート地点を決定
	// 中心から上面の高さまでを取得
	float MeshHalf = RoadMesh->GetBounds().BoxExtent.Z;
	GameStartPos = new FMyVector3D(StartPosition.X,StartPosition.Y,StartPosition.Z + MeshHalf*RoadScale.Y + 50);
	// 点の進行方向ベクトルを取得し回転に変換する
	FMyVector3D Direction = (NextPosition - StartPosition).Normalize();
	// 方向ベクトルを計算
	float YawRad =  std::atan2(Direction.X, Direction.Z);
	constexpr float RadToDeg = 180.0f / 3.14159265358979323846f;
	float YawDeg = YawRad * RadToDeg;
	// 進行方向向きにむかせる
	FMyRotator PlayerStartRotation = FMyRotator(0.0f, 0.0f, 0.0f);
	
	// Spawn時に障害物があっても無視して生成する
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	UWorld* World = GetWorld();
	// APlayerStartをワールド座標に動的生成
	APlayerStart* NewPlayerStartActor = World->SpawnActor<APlayerStart>(
		APlayerStart::StaticClass(),
		GameStartPos->ToFVector(),
		PlayerStartRotation.ToFRotator(),
		SpawnParams);
	
	if (NewPlayerStartActor)
	{
		// 1. 生成位置に赤い球体を描画（半径50cm、10秒間表示）
		DrawDebugSphere(GetWorld(), GameStartPos->ToFVector(), 50.0f, 12, FColor::Red, false, 10.0f, 0, 2.0f);

		// 2. 向き（トランスフォーム）を示す矢印を描画
		DrawDebugCoordinateSystem(GetWorld(), GameStartPos->ToFVector(), PlayerStartRotation.ToFRotator(), 100.0f, false, 10.0f, 0, 3.0f);
	}
}

// Editorで生成されるMeshを確認する
void ASplineMapCreator::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	
	// 1. エディタ上で再生成された時に古くなった既存の SplineMeshComponent を削除
	TArray<USplineMeshComponent*> ExistingMeshes;
	GetComponents<USplineMeshComponent>(ExistingMeshes);
	for (USplineMeshComponent* MeshComp : ExistingMeshes)
	{
		if (MeshComp)
		{
			MeshComp->DestroyComponent(); // 古いメッシュを消去
		}
	}
	
	// 2. プレビュー用のメッシュを生成
	CreateSplineLongMap();
}
#include "SplineMapCreator.h"
#include "MyVector3D.h"
#include "Components/SplineMeshComponent.h"
#include "GameFramework/PlayerStart.h"

#include "CoreMinimal.h"

ASplineMapCreator::ASplineMapCreator()
{
	PrimaryActorTick.bCanEverTick = true;

}

void ASplineMapCreator::BeginPlay()
{
	Super::BeginPlay();
}

void ASplineMapCreator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// SplineMeshComponentを使用して円形のマップを作成する
void ASplineMapCreator::GenerateSplineMap()
{
	if (!RoadMesh)
	{
		return;
	}

	TArray<FMyVector3D> ControlPoints = AddControlPoint();

	// 制御点同士の間を SplineMesh で接続（接線は前後の差分で自前計算）
	for (int32 i = 0; i < ControlPoints.Num(); ++i)
	{
		// 次の点を取得
		int32 NextIndex = i + 1;
		// 最後の点であれば最初の点とつなげて円にする
		if (NextIndex >= ControlPoints.Num())
		{
			NextIndex = 0;
		}
		
		int32 PrevIndex = i - 1;
		// 最初の点なら最後に戻す
		if (PrevIndex < 0)
		{
			PrevIndex = ControlPoints.Num() - 1;
		}

		// 現在の点を格納
		FMyVector3D StartPos = ControlPoints[i];
		// 次の点を格納
		FMyVector3D EndPos = ControlPoints[NextIndex];

		// 接線の計算
		// T(i) = T(dir) * D
		// D = |P(i+1)-P(i)| 2点間の距離
		// 向き＊隣り合う点どうしの距離 = カーブの強さ
		FMyVector3D StartTangent = (EndPos - ControlPoints[PrevIndex]).Normalize() * FMyVector3D::Distance(StartPos, EndPos);
		FMyVector3D EndTangent = (ControlPoints[(NextIndex + 1) % ControlPoints.Num()] - StartPos).Normalize() * FMyVector3D::Distance(StartPos, EndPos);

		CreateStaticMeshComponent(StartPos.ToFVector(),EndPos.ToFVector(),StartTangent.ToFVector(),EndTangent.ToFVector());
	}
	
	// スタート地点を決定
	// 中心から上面の高さまでを取得
	float MeshHalf = RoadMesh->GetBounds().BoxExtent.Z;
	GameStartPos = new FMyVector3D(ControlPoints[0].X,ControlPoints[0].Y,ControlPoints[0].Z + MeshHalf*RoadScale.Y + 50);
	// 点の進行方向ベクトルを取得し回転に変換する
	FMyVector3D Direction = (ControlPoints[1] - ControlPoints[0]).Normalize();
	// 方向ベクトルを計算
	float YawRad =  std::atan2(Direction.X, Direction.Z);
	constexpr float RadToDeg = 180.0f / 3.14159265358979323846f;
	float YawDeg = YawRad * RadToDeg;
	// 進行方向向きにむかせる
	FMyRotator PlayerStartRotation = FMyRotator(0.0f, YawDeg, 0.0f);
	
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

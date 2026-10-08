#include "SplineMapCreator.h"

#include "MyVector3D.h"
#include "Components/SplineMeshComponent.h"

ASplineMapCreator::ASplineMapCreator()
{
	PrimaryActorTick.bCanEverTick = true;

}

void ASplineMapCreator::BeginPlay()
{
	Super::BeginPlay();
	GenerateSplineMap();
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

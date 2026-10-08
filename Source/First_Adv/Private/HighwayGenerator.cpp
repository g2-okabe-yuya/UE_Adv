#include "HighwayGenerator.h"

#include "MyVector3D.h"

AHighwayGenerator::AHighwayGenerator()
{
	PrimaryActorTick.bCanEverTick = true;

}

void AHighwayGenerator::BeginPlay()
{
	Super::BeginPlay();
	GenerateMap();
}


void AHighwayGenerator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// ゲーム上にマップを生成する
void AHighwayGenerator::GenerateMap()
{
	// 配列の初期化
	ControlPoints.Init(FMyVector3D(0,0,0),ControlPoint);
	// 設定数だけ点を作成する
	for (int32 i = 0; i < ControlPoint; i++)
	{
		// 作成した円の半径を取得する
		float r = CalculateTurnRadius(i);
		// 座標に変換する
		FMyVector3D PointVector = ConvertRadiusToPosition(r,i);
		ControlPoints[i] = PointVector;
	}
	
	// 格納した点の隣り合う2点を取り出してCubeを配置していく
	CreateActorOnRoad();
}

// CubeをSpline曲線の上からはりつけていく
// ActorPos = P(s) + (d(side)*N(s))+(d(height)*Up)
void AHighwayGenerator::CreateActorOnRoad()
{
	UWorld* World = GetWorld();
	if (!World || ControlPoints.Num() < 2 || !RoadCubeClass)
	{
		return;
	}
	
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	
	// 定義した一定の高さ(Up方向のオフセット)
	FMyVector3D UpVector(0.0f,0.0f,1.0f);
	
	// 道路の右側に配置したいオフセット距離
	float DSide = 400.0f;

	// 現在の点と次の点を取得する
	for (int32 i = 0; i < ControlPoints.Num(); ++i)
	{
		// P(s) : 現在の制御点P1と次の制御点P2
		FMyVector3D P1 = ControlPoints[i];
		FMyVector3D P2 = ControlPoints[ (i + 1) % ControlPoints.Num()];
		
		// P(s)とする中心線上の位置(2点の中間点)
		//FMyVector3D Ps = (P1 + P2) * 0.5f;
		// 接線(T(s)) : 進行方向の単位ベクトル
		FMyVector3D Tangent = (P2 - P1).Normalize();
		// N(s) : コースの横方向ベクトル(UP * Tangent)
		FMyVector3D Ns = UpVector.Cross(Tangent).Normalize();
		
		// 位置計算: ActorPos = P(s) + (Dside * N(s) + (dheight * UP))
		FMyVector3D ActorPos = P2 + (DSide * Ns) + UpVector;
		
		// Actorの向き(コースの進行方向へ向ける回転)
		float YawAngleRad = std::atan2(Tangent.Y,Tangent.X);
		float YawAngleDeg = YawAngleRad * (180.0f / 3.14159265358979323846f);
		FMyRotator SpawnRotation = FMyRotator(0.0f,0.0f,YawAngleDeg);
		
		// UEへのスポーン処理(World)
		SpawnRoadSegment(ActorPos,P2,SpawnRotation.ToFRotator());
	}
}

void AHighwayGenerator::SpawnRoadSegment(const FMyVector3D& StartPos,const FMyVector3D& EndPos,const FRotator& SegmentRotation)
{
	if (!RoadCubeClass)
	{
		return;
	}
	
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	
	// 道路アクターを生成
	AActor* NewRoadSegment = GetWorld()->SpawnActor<AActor>(RoadCubeClass,StartPos.ToFVector(),SegmentRotation,SpawnParams);
	
	if (!NewRoadSegment)
	{
		return;
	}
	
	// 2点間の実際の距離を算出
	float Distance = FVector::Distance(StartPos.ToFVector(),EndPos.ToFVector());
	
	// アクターないのメッシュの元々のX軸長を自動取得
	// デフォルト
	float BaseMeshLength = 100.0f;
	
	UStaticMeshComponent* MeshComp = NewRoadSegment->FindComponentByClass<UStaticMeshComponent>();
	if (MeshComp && MeshComp->GetStaticMesh())
	{
		// メッシュのローカルサイズを取得
		FBoxSphereBounds MeshBounds = MeshComp->GetStaticMesh()->GetBounds();
		float MeshXSize = MeshBounds.BoxExtent.X * 2.0f;
		if (MeshXSize > 0.0f)
		{
			BaseMeshLength = MeshXSize;
		}
	}
	
	// 道と道の隙間なくつながるX軸のスケール倍率を計算
	float RequiredScaleX = Distance / BaseMeshLength;
	
	// Y Zは設定値のまま、Xスケールのみ距離を合わせて補正をかける
	FMyVector3D FinalScale(RequiredScaleX,CubeScale.Y,CubeScale.Z);
	NewRoadSegment->SetActorScale3D(FinalScale.ToFVector());
}

// 作成した半径を座標に変換する
FMyVector3D AHighwayGenerator::ConvertRadiusToPosition(float Radius, const float &CurrentPointIndex) const
{
	float X = Radius * cos(ParametricVariable(CurrentPointIndex));
	float Y = Radius * sin(ParametricVariable(CurrentPointIndex));
	
	return FMyVector3D(X,Y,0);
}

// 乱数で作成した点をつなげた円の半径を返す
// 半径(r) = R(base) + A1Sin(2t + x1) + A2Sin(3t + x2) + A3Sin(5t + x3)
// t = 媒介変数(乱数)
// x = オフセット
float AHighwayGenerator::CalculateTurnRadius(const float &CurrentPointIndex) const
{
	// 媒介変数
	float t = ParametricVariable(CurrentPointIndex);
	float A1Sin = Swing * std::sin(2 * t + CreateRandOffset());
	float A2Sin = Swing * std::sin(3 * t + CreateRandOffset());
	float A3Sin = Swing * std::sin(5 * t + CreateRandOffset());
	
	return BaseMapSize + A1Sin + A2Sin + A3Sin;
}

// 媒介変数を返す関数
float AHighwayGenerator::ParametricVariable(const float &CurrentPointIndex) const
{
	float i = CurrentPointIndex;
	return i * (2 * 3.14159265358979323846f/ControlPoint);
}

// ランダムなオフセットを返す
float AHighwayGenerator::CreateRandOffset() const
{
	return 0 + (std::rand() % (MaxOffset - 0 + 1));
}

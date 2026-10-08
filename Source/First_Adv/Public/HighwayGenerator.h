#pragma once

#include "CoreMinimal.h"
#include "MyVector3D.h"
#include "GameFramework/Actor.h"
#include "HighwayGenerator.generated.h"

UCLASS()
// パラトメック方程式を使って乱数で作成した点をつなげて円を作成し、その上にActorを配置して道を作成する
class FIRST_ADV_API AHighwayGenerator : public AActor
{
	GENERATED_BODY()
	
public:	
	AHighwayGenerator();
	virtual void Tick(float DeltaTime) override;
	void GenerateMap();
	
	// 配置するCubeアクターのBlueprintクラス（エディタから指定）
	UPROPERTY(EditAnywhere, Category = "Highway")
	TSubclassOf<AActor> RoadCubeClass;
	// Cubeのサイズ設定
	UPROPERTY(EditAnywhere, Category = "Highway")
	FVector CubeScale = FVector(2.0f, 2.0f, 0.5f);
	UPROPERTY(EditAnywhere,Category="Highway")
	// マップの大きさ
	float BaseMapSize = 300.0f;
	// ふり幅　-1に近いほど急発進する
	UPROPERTY(EditAnywhere, Category = "Highway")
	float Swing = 0.4f;
	// 制御点の数
	UPROPERTY(EditAnywhere, Category = "Highway")
	int32 ControlPoint = 10;
	// Offsetの乱数を作成するときのマックス値
	UPROPERTY(EditAnywhere, Category = "Highway")
	int32 MaxOffset = 10;

protected:
	virtual void BeginPlay() override;
	
private:
	TArray<FMyVector3D> ControlPoints;
	// 作成した円の半径を返す
	FMyVector3D ConvertRadiusToPosition(float Radius, const float &CurrentPointIndex) const;
	float CalculateTurnRadius(const float &CurrentPointIndex) const;
	float ParametricVariable(const float &CurrentPointIndex) const;
	// ランダムなオフセットを返す
	float CreateRandOffset() const;
	// Splineの上に道を配置する
	void CreateActorOnRoad();
	void SpawnRoadSegment(const FMyVector3D& StartPos, const FMyVector3D& EndPos, const FRotator& SegmentRotation);
};

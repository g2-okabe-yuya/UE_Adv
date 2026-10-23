#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MyVector3D.h"
#include "Components/SplineComponent.h"
#include "SplineMapCreator.generated.h"

UCLASS()
class FIRST_ADV_API ASplineMapCreator : public AActor
{
	GENERATED_BODY()
	
public:	
	ASplineMapCreator();
	// 長方形に長いマップを作成する
	void CreateSplineLongMap();
	// スタート地点(Playerが生成される位置を返す)
	FMyVector3D GetStartPos() const;
	void CreateStaticMeshComponent(const FVector& StartPos, const FVector& EndPos, const FVector& StartTangent,
		const FVector& EndTangent);
	// スタート地点を保持する
	FMyVector3D* GameStartPos;
	
	// EditorでどんなMapが生成されるかを確認するための関数
	virtual void OnConstruction(const FTransform& Transform) override;

	virtual void Tick(float DeltaTime) override;
	
	// 道路に使用するMesh
	UPROPERTY(EditAnywhere, Category = "Map Settings")
	UStaticMesh* RoadMesh;
	UPROPERTY(EditAnywhere, Category = "Map Settings")
	int32 ControlPoint = 10.0;
	UPROPERTY(EditAnywhere, Category = "Map Settings")
	FVector2D RoadScale;
	UPROPERTY(EditAnywhere, Category = "Map Settings")
	float Radius = 100.0f;
	UPROPERTY(EditAnywhere, Category = "Map Settings")
	UClass* BikeActor = nullptr;
	// 道路の長さ
	UPROPERTY(EditAnywhere, Category = "Map Settings")
	float RoadLength = 100.0f;
	// 道路メッシュ一つ分の長さ
	UPROPERTY(EditAnywhere,Category = "Map Settings")
	float RoadMeshLength = 100.0f;
	UPROPERTY(EditAnywhere, Category = "Map Settings")
	USplineComponent* SplineComponent;
protected:
	virtual void BeginPlay() override;
	
private:
	TArray<FMyVector3D> AddControlPoint() const;
	void CreateStartPoint(FMyVector3D StartPosition, FMyVector3D NextPosition);
};

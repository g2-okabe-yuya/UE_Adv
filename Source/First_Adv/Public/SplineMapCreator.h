#pragma once

#include "CoreMinimal.h"
#include "MyVector3D.h"
#include "Components/SplineMeshComponent.h"
#include "GameFramework/Actor.h"
#include "SplineMapCreator.generated.h"

UCLASS()
class FIRST_ADV_API ASplineMapCreator : public AActor
{
	GENERATED_BODY()
	
public:	
	ASplineMapCreator();
	void GenerateSplineMap();
	void CreateStaticMeshComponent(const FVector& StartPos, const FVector& EndPos, const FVector& StartTangent,
	                               const FVector& EndTangent);

	virtual void Tick(float DeltaTime) override;
	
	UPROPERTY(EditAnywhere, Category = "Map Settings")
	int32 ControlPoint = 10.0;
	UPROPERTY(EditAnywhere, Category = "Map Settings")
	UStaticMesh* RoadMesh;
	UPROPERTY(EditAnywhere, Category = "Map Settings")
	FVector2D RoadScale;
	UPROPERTY(EditAnywhere, Category = "Map Settings")
	float Radius = 100.0f;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
private:
	TArray<FMyVector3D> AddControlPoint() const;
};

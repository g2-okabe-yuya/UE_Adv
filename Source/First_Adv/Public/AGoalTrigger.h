// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AGoalTrigger.generated.h"

class UBoxComponent;

// ゴール到達を通知するイベント関数
DECLARE_MULTICAST_DELEGATE(FOnGoalReachedDelegate);
UCLASS()
class FIRST_ADV_API AGoalTrigger : public AActor
{
	GENERATED_BODY()
	
public:	
	AGoalTrigger();

	// 外部からゴールイベントを購読するためのデリゲート
	FOnGoalReachedDelegate OnGoalReached;
protected:
	virtual void BeginPlay() override;
private:
	// ゴールを通貨したときに呼ぶ
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent,AActor* OtherActor,UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep,const FHitResult& SweepResult);
	// 二重ゴール防止
	bool HasTriggered = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBoxComponent> GoalBox;
};

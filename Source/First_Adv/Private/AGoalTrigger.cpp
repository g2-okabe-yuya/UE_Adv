// Fill out your copyright notice in the Description page of Project Settings.


#include "AGoalTrigger.h"

#include "BikeMovement.h"
#include "Components/BoxComponent.h"

// Sets default values
AGoalTrigger::AGoalTrigger()
{
	PrimaryActorTick.bCanEverTick = false;
	
	// 当たり判定用ボックスの生成とアタッチ
	GoalBox = CreateDefaultSubobject<UBoxComponent>(TEXT("GoalBox"));
	RootComponent = GoalBox;
	
	GoalBox->SetCollisionProfileName(TEXT("Trigger"));
	GoalBox->SetBoxExtent(FVector(100.0f,500.0f,300.0f));
	
	// コリジョン設定
}

// Called when the game starts or when spawned
void AGoalTrigger::BeginPlay()
{
	Super::BeginPlay();
	
	// Bind
	if (GoalBox)
	{
		GoalBox->OnComponentBeginOverlap.AddDynamic(this,&AGoalTrigger::OnOverlapBegin);
	}
}

void AGoalTrigger::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, 
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (HasTriggered)
	{
		return;
	}
	
	// 触れたActorがプレイヤーのバイクかチェック
	if (ABikeMovement* Bike = Cast<ABikeMovement>(OtherActor))
	{
		HasTriggered = true;

		// ゴール到達を通知（Broadcast）
		if (OnGoalReached.IsBound())
		{
			OnGoalReached.Broadcast();
		}
	}
}
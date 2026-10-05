#pragma once

#include "CoreMinimal.h"
#include "MyVector3D.h"
#include "InputMappingContext.h"
#include "Camera/CameraComponent.h"
#include  "InputAction.h"
#include "InputActionValue.h"
#include "GameFramework/Pawn.h"
#include "BikeMovement.generated.h"


// 前方宣言
class UBoxComponent;
class UStaticMeshComponent;
class USpringArmComponent;
class UCameraComponent;
class UEnhancedInputComponent;

// Bikeの物理計算用構造体
struct FBikePhysicsState
{
	// 位置
	FMyVector3D Position = FMyVector3D::ZeroVector();
	// 速度
	FMyVector3D Velocity = FMyVector3D::ZeroVector();
	// 角速度
	FMyVector3D AngularVelocity = FMyVector3D::ZeroVector();
	// 回転
	FMyQuat Rotation = FMyQuat::IdentityQuat();
	
	// エンジンの計算
	float EngineRPM = 1000.0f;
	// バンク角
	float LeanAngle = 0.0f;
	// ステアリング角
	float SteeringAngle = 0.0f;
};

UCLASS()
class FIRST_ADV_API ABikeMovement : public APawn
{
	GENERATED_BODY()

public:
	ABikeMovement();
	virtual void Tick(float DeltaTime) override;
	virtual void BeginPlay() override;
	
protected:
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	
	// 生ポインタというものがある
	//UPROPERTY(EditAnywhere, Category="ACamera")
	UCameraComponent* Camera;
	
	UPROPERTY(EditAnywhere,Category="Input")
	UInputMappingContext* DefaultMappingContext;
	UPROPERTY(EditAnywhere,Category="Input")
	UInputAction* ActionInput;
	UPROPERTY(EditAnywhere,Category="Input")
	UInputAction* IA_Throttle;
	UPROPERTY(EditAnywhere,Category="Input")
	UInputAction* IA_Brake;
	UPROPERTY(EditAnywhere,Category="Input")
	UInputAction* IA_Steer;

private:
	// ToDo : Editorで設定できるようにする
	// 質量
	float BikeMass = 200.0f;
	float MaxForce = 3000.f;
	float MaxBrakeForce = 5000.f;
	float AirResistanceCoefficient = 0.35f;
	
	// 自然減速
	float RollingResistance = 400.0f;
	
	// 入力
	float ThrottleInput = 0.0f;
	float BrakeInput = 0.0f;
	float SteeringInput = 0.0f;
	float LearnInput = 0.0f;
	
	// 状態管理用ステート
	FBikePhysicsState PhysicsState;
	
	void UpdateCustomPhysics(float DeltaTime);
	void UpdateAngularVelocity(float DeltaTime);
	void Initialize();
	
	void OnThrottleInput(const FInputActionValue& Value);
	void OnThrottleCompleted(const FInputActionValue& value);
	void OnSteeringInput(const FInputActionValue& Value);
	void OnSteeringInputCompleted(const FInputActionValue& value);
	// カメラの向きに合わせてバイクの物理状態(Rotation)の向きを直接補正・同期する
	void AlignForwardToCamera();
};
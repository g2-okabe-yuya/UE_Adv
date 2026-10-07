#pragma once

#include "CoreMinimal.h"
#include "MyVector3D.h"
#include "InputMappingContext.h"
#include "FBikeRigidBodyDynamics.h"
#include "Camera/CameraComponent.h"
#include  "InputAction.h"
#include "InputActionValue.h"
#include "GameFramework/Pawn.h"
#include "BikeMovement.generated.h"


// 前方宣言
class USpringArmComponent;
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
	
	// エンジンの回転数
	float EngineRPM =0.0f;
	// 現在のギア
	int32 CurrentGear = 1;
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
	
	UPROPERTY()
	TObjectPtr<UCameraComponent> CameraComponent;
	UPROPERTY()
	TObjectPtr<USpringArmComponent> SpringArmComponent;
	
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
	// 車体の質量
	float BikeMass = 200.0f;
	float Pi = 3.14159265358979323846f;
	float MaxBrakeForce = 5000.f;
	// コーナリング剛性
	float CorneringStiffness = 15.0f;
	// 空気抵抗係数
	float AirResistanceCoefficient = 0.35f;
	// 最大トルク量
	float MaxTorque = 110.0f;
	// トルクカーブの滑らかさを決定する
	float TorqueCurve = 0.4f;
	// 最大旋回角速度
	float MaxYawRate = 1.2f;
	// 最大バンク角
	float MaxLeanAngleRad = 0.87f;
	// 傾き(ロール)の追従スピード(補完スピード)
	float LeanResponseSpeed = 5.0f;
	// エンジン用のパラメータ
	float IdleRPM = 1000.f;
	float MaxRPM = 12000.f;
	// リアタイヤの半径
	float RearWheelRadius = 0.31f;
	// 一次減速比(エンジンからクラッチ)
	float PrimaryReductionRatio = 1.75f;
	// ファイナルドライブ比(スプロケット・チェーン)
	float FinalDriveRatio = 2.80f;
	// 水平方向(Yaw)の回転状態を保持するQuaternion
	FMyQuat YawRotation = FMyQuat::IdentityQuat();
	// 1速から6速の各ギア比
	TArray<float> GearRatios = {2.60f,1.95f,1.55f,1.30f,1.12f,0.98f};
	
	// キャンバー剛性(N/rad)
	float CamberStiffness = 800.0f;
	// 自然減速
	float RollingResistance = 400.0f;
	
	// 入力
	float ThrottleInput = 0.0f;
	float BrakeInput = 0.0f;
	float SteeringInput = 0.0f;
	float LearnInput = 0.0f;
	
	// 状態管理用ステート
	FBikePhysicsState PhysicsState;
	
	// 6Dof 剛体物理エンジン
	FBikeRigidBodyDynamics DynamicsEngine;
	FBikeRigidBodyState RigidBodyState;
	
	void Initialize();
	// 前に力を与える計算
	FMyVector3D ForwardVector(float CurrentSpeed);
	void UpdateCustomPhysics(float DeltaTime);
	void UpdateAngularVelocity(float DeltaTime);
	void OnThrottleInput(const FInputActionValue& Value);
	void OnThrottleCompleted(const FInputActionValue& value);
	void OnSteeringInput(const FInputActionValue& Value);
	void OnSteeringInputCompleted(const FInputActionValue& value);
	float CalculateEngineRPM(float SpeedMPS);
	float GetEngineTorqueAtRPM(float RPM) const;
	float CalculateDriveForce(float SpeedMPS);
	// バンク角を計算して車体を傾ける
	void BankAngle(float DeltaTime); 
	float TotalReductionRatio();
	float CalculateCamberThrust(float LeanAngleRad) const;
	void UpdateLeanAngle(float deltaTime);
	void OnShiftUp(const FInputActionValue& Value);
	void OnShiftDown(const FInputActionValue& Value);
	float LateralAcceleration() const;
	float CalculateSlipAngle(float SteeringAngleRad) const;
	float CalculatePacejkaLateralForce(float SlipAngle) const;
};
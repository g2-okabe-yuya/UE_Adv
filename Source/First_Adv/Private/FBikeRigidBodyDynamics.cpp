#pragma once

#include "FBikeRigidBodyDynamics.h"

#include "CoreMinimal.h"

FBikeRigidBodyDynamics::FBikeRigidBodyDynamics()
{
	// デフォルト慣性テンソルの初期化(Ix, Iy, Iz
	FMyVector3D DefaultInertia = FMyVector3D(35.0f, 75.0f, 45.0f);
	InitializeParam(200.f, DefaultInertia);
}

FBikeRigidBodyDynamics::~FBikeRigidBodyDynamics(){}

void FBikeRigidBodyDynamics::InitializeParam(float Mass, const FMyVector3D& InertiaDiagonal)
{
	BikeMass = Mass;
	
	// 対角成分に慣性モーメントを初期化
	InertiaTensor.M[0][0] = InertiaDiagonal.X;
	InertiaTensor.M[0][1] = 0.0f;
	InertiaTensor.M[0][2] = 0.0f;
	InertiaTensor.M[1][0] = 0.0f;
	InertiaTensor.M[1][1] = InertiaDiagonal.Y;
	InertiaTensor.M[1][2] = 0.0f;
	InertiaTensor.M[2][0] = 0.0f;
	InertiaTensor.M[2][1] = 0.0f;         
	InertiaTensor.M[2][2] = InertiaDiagonal.Z;
	
	InverseInertiaTensor = InertiaTensor.Inverse();
}

// 前後ホイールの高速回転によるジャイロプレセッションモーメント計算
FMyVector3D FBikeRigidBodyDynamics::CalculateGyroscopicTorque(const FBikeRigidBodyState& State) const
{
	// 前輪・後輪の角運動量ベクトル L = I * omega
	// 車体ローカル座標系におけるホイール回転軸はY軸(Pitch軸)
	float FrontAngularMoment = WheelInertia * State.FrontWheelAngularVelocity;
	// 後輪
	float RearAngularMoment = WheelInertia * State.RearWheelAngularVelocity;
	float TotalWheelAngularMoment = FrontAngularMoment + RearAngularMoment;
	
	// ジャイロトルク
	FMyVector3D WheelAngularMomentVector(0.0f,TotalWheelAngularMoment,0.0f);
	
	return State.AngularVelocity.Cross(WheelAngularMomentVector);
}

// キャスター角　トレール長　タイヤ横力から発生するセルフステア復元トルク計算
float FBikeRigidBodyDynamics::CalculateSelfSteeringTorque(const FBikeRigidBodyState& State, float FrontLateralForce) const
{
	// トレール長と前輪横力によるセルフステア復元モーメント
	float TrailTorque = -FrontLateralForce * TrailDistance * std::cos(CasterAngleRad);
	
	// キャンバー傾き(Roll角)によるフロント周りの自重倒れこみトルク
	FMyVector3D ForwardDir = State.Orientation.GetForwardVector();
	float CurrentRollRed = std::asin(std::clamp(ForwardDir.Z,-0.99f,0.99f));
	float GravityTorque = BikeMass * 9.81f * TrailDistance * std::sin(CasterAngleRad) * std::sin(CurrentRollRed);
	
	// ステアリングダンバー減衰力
	float DamperTorque = -SteeringDamping * State.SteeringAngularVelocity;
	
	return  TrailTorque + GravityTorque + DamperTorque;
}

void FBikeRigidBodyDynamics::UpdateDynamics(FBikeRigidBodyState& InOutState, 
	const FMyVector3D& WorldDriveForce, const FMyVector3D& WorldFrontTireForce, const FMyVector3D& WorldRearTireForce, float SteeringTorqueInput, float DeltaTime) const
{
	if (DeltaTime <= 0.0f)
		return;
	
	// 並進運動方程式(F = m * a)
	FMyVector3D GravityForce(0.0f,0.0f,-9.81f * BikeMass);
	FMyVector3D TotalWorldForce = WorldDriveForce + WorldFrontTireForce + WorldRearTireForce + GravityForce;
	
	InOutState.LinearAcceleration = TotalWorldForce * (1.0f / BikeMass);
	InOutState.LinearVelocity += InOutState.LinearAcceleration * DeltaTime;
	InOutState.Position += InOutState.LinearVelocity * DeltaTime;
	
	// 回転運動方程式
	// ホイールジャイロ効果トルクの効果
	FMyVector3D GyroTorque = CalculateGyroscopicTorque(InOutState);
	
	// 外力による合計モーメント
	// タイヤ接地荷重モーメント等を合算可能
	FMyVector3D ExternalTorque = GyroTorque;
	
	// 慣性項　w * (I * W)の計算
	FMyVector3D Iw = InertiaTensor.TransformVector(InOutState.AngularVelocity);
	FMyVector3D GyroscopicInternalTorque = InOutState.AngularVelocity.Cross(Iw);
	
	// 角加速度の導出　alpha = I^-1 * (T_ext - w * (I * W))
	FMyVector3D NetTorque = ExternalTorque - GyroscopicInternalTorque;
	InOutState.AngularAcceleration = InverseInertiaTensor.TransformVector(NetTorque);
	
	// 角速度更新
	InOutState.AngularVelocity += InOutState.AngularAcceleration * DeltaTime;
	
	// 姿勢Quaternionの数値積分(dq/dt = 0.5 * q * w)
	FMyQuat AngularVelocityQuat(InOutState.AngularVelocity.X,InOutState.AngularVelocity.Y,InOutState.AngularVelocity.Z,0.0f);
	FMyQuat QuatDerivative = InOutState.Orientation * AngularVelocityQuat * 0.5f;
	
	InOutState.Orientation.X += QuatDerivative.X * DeltaTime;
	InOutState.Orientation.Y += QuatDerivative.Y * DeltaTime;
	InOutState.Orientation.Z += QuatDerivative.Z * DeltaTime;
	InOutState.Orientation.W += QuatDerivative.W * DeltaTime;
	
	// フロントステアリング運動方程式
	// 前輪横力抽出
	float FrontLateralForce = WorldFrontTireForce.Size();
	float NetSteeringTorque = SteeringTorqueInput + CalculateSelfSteeringTorque(InOutState, FrontLateralForce);
	
	float SteeringAngularAcceleration = NetSteeringTorque / SteeringInertia;
	InOutState.SteeringAngularVelocity += SteeringAngularAcceleration * DeltaTime;
	InOutState.SteeringAngularVelocity += InOutState.SteeringAngularVelocity * DeltaTime;
	
	// ハンドル切れの角の上限クランプ
	InOutState.SteeringAngle = std::clamp(InOutState.SteeringAngle,-0.61f,0.61f);
}

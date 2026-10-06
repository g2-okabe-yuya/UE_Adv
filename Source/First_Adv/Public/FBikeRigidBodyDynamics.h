#pragma once

#include "CoreMinimal.h"
#include "FMyMatrix.h"

class FIRST_ADV_API FBikeRigidBodyDynamics
{
public:
	FBikeRigidBodyDynamics();
	// デストラクタ
	~FBikeRigidBodyDynamics();
	
	void InitializeParam(float Mass, const FMyVector3D& InertiaDiagonal);
	
	// マイフレームの力学計算
	void UpdateDynamics(FBikeRigidBodyState& InOutState,
		const FMyVector3D& WorldDriveForce,
		const FMyVector3D& WorldFrontTireForce,
		const FMyVector3D& WorldRearTireForce,
		float SteeringTorqueInput,
		float DeltaTime
		) const;
	
	// ジャイロモーメントの算出
	FMyVector3D CalculateGyroscopicTorque(const FBikeRigidBodyState& State) const;
	
	// ステアリングセルフステア復元のトルクの計算
	float CalculateSelfSteeringTorque(const FBikeRigidBodyState& State,float FrontLateralForce) const;
	
private:
	// 総質量
	float BikeMass = 200.0f;
	// 慣性テンソル
	FMyMatrix InertiaTensor;
	// 逆慣性テンソル
	FMyMatrix InverseInertiaTensor;
	
	// ホイール慣性定数
	// ホイール重量(kg)
	float WheelMass = 12.0f;
	// ホイール半径
	float WheelRadius = 0.31f;
	// ホイール回転慣性モーメント(kg・m2)
	float WheelInertia = 0.57f;
	
	// ジオメトリ定数
	// キャスター角
	float CasterAngleRad = 0.418f;
	// トレール長
	float TrailDistance = 0.10f;
	// フロント周りステアリング慣性
	float SteeringInertia = 0.25f;
	// ステアリングダンバー減衰定数
	float SteeringDamping = 8.5f;
};

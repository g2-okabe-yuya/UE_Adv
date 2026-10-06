#pragma once

#include "CoreMinimal.h"
#include "MyVector3D.h"

// 3*3 行列構造体
struct  FMyMatrix
{
	float M[3][3];
	
	FMyMatrix()
	{
		// 単位行列
		for (int i = 0; i < 3; ++i)
		{
			for (int j = 0; j < 3; ++j)
			{
				if (i == j)
				{
					// 行番号=列番号なら(対角線上)なら1.0
					M[i][j] = 1.0f;
				}
				else
				{
					// それ以外なら0.0
					M[i][j] = 0.0f;
				}
			}
		}
	}
	
	static FMyMatrix Identity()
	{
		return  FMyMatrix();
	}
	
	// ベクトルとの乗算(M * V)
	FMyVector3D TransformVector(const FMyVector3D& Vector) const
	{
		return FMyVector3D(
			M[0][0] * Vector.X + M[0][1] * Vector.Y + M[0][2] * Vector.Z,
			M[1][0] * Vector.X + M[1][1] * Vector.Y + M[1][2] * Vector.Z,
			M[2][0] * Vector.X + M[2][1] * Vector.Y + M[2][2] * Vector.Z
			);
	}
	
	// 逆行列の計算(3 * 3 行列式による厳密解)
	// 余因子行列
	FMyMatrix Inverse() const
	{
		// 左上から右下にかけて足す - 右上から左下にかけて足す
		// ラプラス展開
		// 体積の変化率を表す変化式
		float Det =
			M[0][0] * (M[1][1] * M[2][2] - M[1][2] * M[2][1])-
				M[0][1] * (M[1][0] * M[2][2] - M[1][2] * M[2][0])+
					M[0][2] * (M[1][0] * M[2][1] - M[1][1] * M[2][0]);
		
		if (std::abs(Det) < 0.00001f)
		{
			return Identity();
		}
		
		float InverseDet = 1.0f / Det;
		FMyMatrix Result;
		
		Result.M[0][0] = (M[1][1] * M[2][2] - M[1][2] * M[2][1]) * InverseDet;
		Result.M[0][1] = (M[0][2] * M[2][1] - M[0][1] * M[2][2]) * InverseDet;
		Result.M[0][2] = (M[0][1] * M[1][2] - M[0][2] * M[1][1]) * InverseDet;
		
		Result.M[1][0] = (M[1][2] * M[2][0] - M[1][0] * M[2][2]) * InverseDet;
		Result.M[1][1] = (M[0][0] * M[2][2] - M[0][2] * M[2][0]) * InverseDet;
		Result.M[1][2] = (M[0][2] * M[1][0] - M[0][0] * M[1][2]) * InverseDet;
		
		Result.M[2][0] = (M[1][0] * M[2][1] - M[1][1] * M[2][0]) * InverseDet;
		Result.M[2][1] = (M[0][1] * M[2][0] - M[0][0] * M[2][1]) * InverseDet;
		Result.M[2][2] = (M[0][0] * M[1][1] - M[0][1] * M[1][0]) * InverseDet;
		
		return Result;
	}
};

// バイク車体の剛体物理ステート構造体
struct FBikeRigidBodyState
{
	FMyVector3D Position = FMyVector3D::ZeroVector();
	FMyVector3D LinearVelocity = FMyVector3D::ZeroVector();
	FMyVector3D LinearAcceleration = FMyVector3D::ZeroVector();
	
	FMyQuat Orientation = FMyQuat::IdentityQuat();
	FMyVector3D AngularVelocity = FMyVector3D::ZeroVector();
	FMyVector3D AngularAcceleration = FMyVector3D::ZeroVector();
	
	// ホイールの回転ステート
	// 前輪角速度(rad/s)
	float FrontWheelAngularVelocity = 0.0f;
	// 後輪角速度(rad/s)
	float RearWheelAngularVelocity = 0.0f;
	// ステアリング角
	float SteeringAngle = 0.0f;
	// ステアリング角速度(rad/s)
	float SteeringAngularVelocity = 0.0f;
};
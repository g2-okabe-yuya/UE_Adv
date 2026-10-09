#pragma once

#include "CoreMinimal.h"
#include "MyVector3D.h"

// 3次元ベクトル
// ToDo : cppわけしてデストラクタでメモリを解放する
struct FMyVector3D
{
	float X = 0.0f;
	float Y = 0.0f;
	float Z = 0.0f;
	
	// コンストラクタ
	FMyVector3D() : X(0.0f), Y(0.0f), Z(0.0f){}
	FMyVector3D(float InX, float InY, float InZ) : X(InX), Y(InY), Z(InZ){}
	
	// FVectorに渡す用の関数
	FVector ToFVector() const
	{
		return FVector(X, Y, Z);
	}
	
	// FVectorを自作Vectorに変換するための関数
	static FMyVector3D FromFVector(const FVector& InVector)
	{
		return FMyVector3D(InVector.X, InVector.Y, InVector.Z);
	}
	
	// ベクトルの足し算
	FMyVector3D operator+(const FMyVector3D& InVector) const
	{
		return FMyVector3D(X + InVector.X, Y + InVector.Y, Z + InVector.Z);
	}
	
	// ベクトルを直接足し算する
	FMyVector3D& operator+=(const FMyVector3D& InVector)
	{
		X += InVector.X;
		Y += InVector.Y;
		Z += InVector.Z;
		return *this;
	}
	
	// ベクトルの引き算
	FMyVector3D operator-(const FMyVector3D& InVector) const
	{
		return FMyVector3D(X - InVector.X, Y - InVector.Y, Z - InVector.Z);
	}
	
	// ベクトルを直接引き算する
	FMyVector3D& operator-=(const FMyVector3D& InVector)
	{
		X -= InVector.X;
		Y -= InVector.Y;
		Z -= InVector.Z;
		return *this;
	}
	
	// スカラー倍(Vector*float)
	FMyVector3D operator*(float Scalar) const
	{
		return FMyVector3D(X * Scalar, Y * Scalar, Z * Scalar);
	}
	
	friend FMyVector3D operator*(float Scalar, const FMyVector3D& InVector)
	{
		return FMyVector3D(InVector.X * Scalar,InVector.Y * Scalar,InVector.Z * Scalar);
	}
	
	FMyVector3D Normalize() const
	{
		float VectorSize = Size();
		
		// 0で割り算しないようにする
		if (VectorSize > 0.00001f)
		{
			return FMyVector3D(X / VectorSize, Y / VectorSize, Z / VectorSize);
		}
		
		return ZeroVector();
	}
	
	// 初期化
	static FMyVector3D ZeroVector()
	{
		return FMyVector3D(0.0f, 0.0f, 0.0f);
	}
	
	// ベクトルの外積
	FMyVector3D Cross(const FMyVector3D& InVector) const
	{
		return FMyVector3D(
			Y * InVector.Z - Z * InVector.Y,
			Z * InVector.X - X * InVector.Z,
			X * InVector.Y - Y * InVector.X
			);
	}
	
	// 2点間の直線距離を求める
	static float Distance(const FMyVector3D& Start,const FMyVector3D& End)
	{
		// Start(X1,Y1,Z1),End(X2,Y2,Z3)
		// ^(x2-x1)*(x2-x1) + (y2-y1)*(y2-y1) + (z2-z1)*(z2-z1)
		return std::sqrt((End.X - Start.X) * (End.X - Start.X) + (End.Y - Start.Y)*(End.Y - Start.Y) + (End.Z - Start.Z)*(End.Z - Start.Z));
	}
	
	// ベクトルの内積 (Dot Product) A・B = AxBx + AyBy + AzBz
	float Dot(const FMyVector3D& Other) const
	{
		return X * Other.X + Y * Other.Y + Z * Other.Z;
	}
	
	// ベクトルの大きさを返す
	float Size() const
	{
		return std::sqrt(X * X + Y * Y + Z * Z);
	}
};

// Quaternion
struct FMyQuat
{
	float X = 0.0f;
	float Y = 0.0f;
	float Z = 0.0f;
	float W = 1.0f;
	
	// コンストラクタ
	FMyQuat() : X(0.0f), Y(0.0f), Z(0.0f), W(1.0f){}
	FMyQuat(float InX,float InY,float InZ,float InW) : X(InX), Y(InY), Z(InZ), W(InW){}

	static FMyQuat FromFQuat(const FQuat& InVector)
	{
		return FMyQuat(InVector.X, InVector.Y, InVector.Z, InVector.W);
	}
	
	// FVectorに渡す用の関数
	FQuat ToFQuat() const
	{
		return FQuat(X, Y, Z, W);
	}
	
	// 回転を考慮しない
	static FMyQuat IdentityQuat()
	{
		return FMyQuat(0.0f, 0.0f, 0.0f, 1.0f);
	}
	
	// Quaternionの生成
	static FMyQuat FromAxisAngle(const FMyVector3D& Axis, float AngleInRadians)
	{
		float AxisSize = Axis.Size();
		if (AxisSize < 0.00001f)
		{
			return IdentityQuat();
		}
		
		const float HalfAngle = AngleInRadians * 0.5f;
		// 半角に対するSinΘとCosΘを計算
		const float SinHalf = std::sin(HalfAngle);
		const float CosHalf = std::cos(HalfAngle);
		
		const FMyVector3D NormalizedAxis = Axis * (1.0f /AxisSize);
		
		return FMyQuat(NormalizedAxis.X*SinHalf,NormalizedAxis.Y*SinHalf,NormalizedAxis.Z*SinHalf,CosHalf);
	}
	
	// かけ算
	FMyQuat operator*(const FMyQuat& Quat) const
	{
		float OutX = W * Quat.X + X * Quat.W + Y * Quat.Z - Z * Quat.Y;
		float OutY = W * Quat.Y - X * Quat.Z + Y * Quat.W + Z * Quat.X;
		float OutZ = W * Quat.Z + X * Quat.Y - Y * Quat.X + Z * Quat.W;
		float OutW = W * Quat.W - X * Quat.X - Y * Quat.Y - Z * Quat.Z;
		
		return FMyQuat(OutX, OutY, OutZ, OutW);
	}
	
	// スカラー倍の実装
	FMyQuat operator*(float Scalar) const
	{
		return FMyQuat(
			X * Scalar,
			Y * Scalar,
			Z * Scalar,
			W * Scalar
		);
	}
	
	// 回転後のForwardVectorを直接計算
	FMyVector3D GetForwardVector() const
	{
		// 単位ベクトル(1,0,0)をQuaternionで回転させた結果を展開した式
		return FMyVector3D(
			1.0f - 2.0f * (Y * Y + Z * Z),
			2.0f * (X * Y + W * Z),
			2.0f * (X * Z + W * Y));
	}
	
	FMyVector3D GetUpVector() const
	{
		return FMyVector3D(
			2.0f * (X * Z - W * Y),
			2.0f * (Y * Z + W * X),
			1.0f - 2.0f * (X * X + Y * Y)
		);
	}
	
	FMyVector3D GetRightVector() const
	{
		return FMyVector3D(
			2.0f * (X * Y + W * Z),
			1.0f - 2.0f * (X * X + Z * Z),
			2.0f * (Y * Z - W * X)
		);
	}
};


// オイラー角
struct FMyRotator
{
	// X軸回転
	float Roll = 0.0f;
	// Y軸回転
	float Pitch = 0.0f;
	// Z軸回転
	float Yaw = 0.0f;
	
	FMyRotator() : Roll(0.0f), Pitch(0.0f), Yaw(0.0f) {}
	FMyRotator(float InPitch,float InYaw,float InRoll) : Roll(InPitch),Pitch(InYaw),Yaw(InRoll){}
	
	// DegをRadに変換
	static constexpr float DegToRad()
	{
		return 3.14159265358979323846f / 180.0f;
	}
	
	// オイラー角からForward Vectorを直接算出
	FMyVector3D GetForwardVector() const
	{
		// ラジアンに変換
		float RadPitch = Pitch * DegToRad();
		float RadYaw = Yaw * DegToRad();
		
		// 度数
		float CP = std::cos(RadPitch);
		float SP = std::sin(RadPitch);
		float CY = std::cos(RadYaw);
		float SY = std::sin(RadYaw);
		
		return FMyVector3D(CP * CY, CP * SY, SP);
	}
	
	// オイラー角をQuaternion
	FMyQuat ToQuat() const
	{
		float HalfRoll = Roll * DegToRad() * 0.5f;
		float HalfPitch = Pitch * DegToRad() * 0.5f;
		float HalfYaw = Yaw * DegToRad() * 0.5f;
		
		float SR = std::sin(HalfRoll);
		float CR = std::cos(HalfRoll);
		float SP = std::sin(HalfPitch);
		float CP = std::cos(HalfPitch);
		float SY = std::sin(HalfYaw);
		float CY = std::cos(HalfYaw);
		
		float OutX = SR * CP * CY - CR * SP * SY; 
		float OutY = CR * SP * CY + SR * CP * SY; 
		float OutZ = CR * CP * SY - SR * SP * CY; 
		float OutW = CR * CP * CY + SR * SP * SY; 
		
		return FMyQuat(OutX, OutY, OutZ, OutW);
	}
	
	FRotator ToFRotator() const
	{
		return FRotator(Pitch,Yaw,Roll);
	}
	
	static FMyRotator FromFRotator(const FRotator& Rotator)
	{
		return FMyRotator(Rotator.Roll,Rotator.Pitch,Rotator.Yaw);
	}
};

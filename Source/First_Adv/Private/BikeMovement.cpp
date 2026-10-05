#include "BikeMovement.h"
#include "InputMappingContext.h"
#include "Camera/CameraComponent.h"
#include  "InputAction.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

ABikeMovement::ABikeMovement()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ABikeMovement::BeginPlay()
{
	Super::BeginPlay();
	Initialize();
}

// 自身の初期位置を保存する
void ABikeMovement::Initialize()
{
	PhysicsState.Position = FMyVector3D::FromFVector(GetActorLocation());
	PhysicsState.Rotation = FMyQuat::FromFQuat(GetActorQuat());
}

void ABikeMovement::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	// 位置と傾きの更新
	UpdateCustomPhysics(DeltaTime);
	// UEのオブジェクトに更新を反映する
	SetActorLocationAndRotation(PhysicsState.Position.ToFVector(),PhysicsState.Rotation.ToFQuat());
}

// InputSystem
void ABikeMovement::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	
	// Inputの有効化
	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			if (DefaultMappingContext)
			{
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
			}
		}
	}
	
	// Inputのバインド処理
	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (IA_Throttle)
		{
			EnhancedInputComponent->BindAction(IA_Throttle, ETriggerEvent::Triggered, this, &ABikeMovement::OnThrottleInput);
			EnhancedInputComponent->BindAction(IA_Throttle, ETriggerEvent::Completed, this, &ABikeMovement::OnThrottleCompleted);
		}

		if (IA_Steer)
		{
			EnhancedInputComponent->BindAction(IA_Steer, ETriggerEvent::Triggered, this, &ABikeMovement::OnSteeringInput);
			EnhancedInputComponent->BindAction(IA_Steer, ETriggerEvent::Completed, this, &ABikeMovement::OnSteeringInputCompleted);
		}
	}
}

// ===================================
// 力を計算
// 1次元ベクトルの計算(アクセル・ブレーキ)
// 前方力 = スロットル*最大駆動力
// ブレーキ = 空気抵抗係数 * 現在の速度2乗
// 現在の合力 F=F(drive)-F(drag)-F(brake)
//　速度を計算
// F = ma(力=質量*加速度)　加速度 = F/a
//　位置と速度を更新する(数値積分)
// 前フレームからの経過時間をDeltaTimeとします
// 新しい速度 = 今の速度+(a*DeltaTime)
// 新しい位置 = 現在の速度+(速度*DeltaTime)
// ====================================

// アクセルとブレーキの力を計算する
void ABikeMovement::UpdateCustomPhysics(float DeltaTime)
{
	if (DeltaTime <= 0.0f)
		return;
	
	AlignForwardToCamera();
	
	// 現在の移動スピードを取得
	// m/s
	// UEの単位が1unit = 1cm 変換の必要あり
	float CurrentSpeed = PhysicsState.Velocity.Size();
	float CurrentSpeedMPS = CurrentSpeed /100.0f;
	
	// ステアリング入力
	float MaxYawRate = 1.2f;
	PhysicsState.AngularVelocity.Z = SteeringInput * MaxYawRate;
	UpdateAngularVelocity(DeltaTime);
	
	// 力の計算
	// 前進(前方向のインプットを受け取って最大駆動力をかける)
	float DriveForce = ThrottleInput * MaxForce;
	float DragForce = AirResistanceCoefficient*(CurrentSpeedMPS*CurrentSpeedMPS);
	
	// 直進方向の「力の合計(Total Force)」を算出
	// 前に進む力 - ブレーキ力 - 空気抵抗
	// 合力
	float ForwardTotalForce = DriveForce - DragForce;
	if (CurrentSpeedMPS > 0.05f)
	{
		ForwardTotalForce -= RollingResistance;
	}
	
	// 進行方向のベクトルを計算
	FMyVector3D ForwardVector = PhysicsState.Rotation.GetForwardVector();
	// 加速度の計算
	// 力/質量=加速度
	float ForwardAcceleration = ForwardTotalForce / BikeMass;
	// cm/s^2に単位をそろえる
	FMyVector3D AccelerationVector = ForwardVector * (ForwardAcceleration * 100.0f);
	
	// 数値積分(速度と位置の更新)
	//　1フレームに加速する値を加算
	PhysicsState.Velocity += AccelerationVector * DeltaTime;
	
	// 停止時の微小な振動とバックを防止する処理
	if (ThrottleInput <= 0.0f && CurrentSpeed < 10.0f)
	{
		PhysicsState.Velocity = FMyVector3D(0.0f, 0.0f, 0.0f);
	}
	
	// 1フレームで移動したベクトルを取得して加算する
	PhysicsState.Position += PhysicsState.Velocity * DeltaTime;
}

// 車体の向きと傾きを更新する
void ABikeMovement::UpdateAngularVelocity(float DeltaTime)
{
	// 現在の移動スピードを取得
	float AngularSpeed = PhysicsState.AngularVelocity.Size();
	
	if (AngularSpeed > 0.0001f)
	{
		// 回転軸を正規化
		FMyVector3D RotationAxis = PhysicsState.AngularVelocity.Normalize();
		// この1フレームで動くラジアンを計算
		float AngleThisFrame = AngularSpeed * DeltaTime;
		// 回転軸から、この1フレーム分のQuaternion
		FMyQuat DeltaQuat = FMyQuat::FromAxisAngle(RotationAxis, AngleThisFrame);
		PhysicsState.Rotation = PhysicsState.Rotation * DeltaQuat;
	}
}

// カメラの向きに合わせtバイクの回転を直接揃える
void ABikeMovement::AlignForwardToCamera()
{
	if (!Camera)
		return;
	
	// 現在のカメラ回転をFMyRotatorへ変換
	FMyRotator CameraRotator = FMyRotator::FromFRotator(Camera->GetComponentRotation());
	
	// 水平回転のみを取り出す
	FMyRotator YawRotator(0.0f,0.0f,CameraRotator.Yaw);
	
	// Quaternionに変換する
	PhysicsState.Rotation = YawRotator.ToQuat();
}

// ToDo : バンク角の計算



void ABikeMovement::OnThrottleInput(const FInputActionValue& Value)
{
	ThrottleInput = Value.Get<float>();
}

void ABikeMovement::OnThrottleCompleted(const FInputActionValue& Value)
{
	ThrottleInput = 0.0f;
}

void ABikeMovement::OnSteeringInput(const FInputActionValue& Value)
{
	SteeringInput = Value.Get<float>();
}

void ABikeMovement::OnSteeringInputCompleted(const FInputActionValue& Value)
{
	SteeringInput = 0.0f;
}
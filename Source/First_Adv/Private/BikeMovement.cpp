#include "BikeMovement.h"
#include "InputMappingContext.h"
#include "Camera/CameraComponent.h"
#include  "InputAction.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/SpringArmComponent.h"

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
	
	PhysicsState.EngineRPM = IdleRPM;
	PhysicsState.CurrentGear = 1;
	
	if (CameraComponent == nullptr)
	{
		CameraComponent = FindComponentByClass<UCameraComponent>();
	}
	
	if (SpringArmComponent == nullptr)
	{
		SpringArmComponent = FindComponentByClass<USpringArmComponent>();
	}
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

// アクセルとブレーキの力を計算する
void ABikeMovement::UpdateCustomPhysics(float DeltaTime)
{
	if (DeltaTime <= 0.0f)
		return;
	
	// 現在の移動スピードを取得
	// m/s
	// UEの単位が1unit = 1cm 変換の必要あり
	float CurrentSpeed = PhysicsState.Velocity.Size();
	float CurrentSpeedMPS = CurrentSpeed /100.0f;
	
	// ステアリング入力
	float MaxYawRate = 1.2f;
	PhysicsState.AngularVelocity.Z = -SteeringInput * MaxYawRate;
	UpdateAngularVelocity(DeltaTime);
	
	// 力の計算
	// 駆動力を取得
	float DriveForce =  CalculateDriveForce(CurrentSpeedMPS);
	// 空気抵抗と転がり抵抗
	float DragForce = AirResistanceCoefficient*(CurrentSpeedMPS*CurrentSpeedMPS);
	
	// 直進方向の「力の合計(Total Force)」を算出
	// 前に進む力 - ブレーキ力 - 空気抵抗
	// 合力
	float ForwardTotalForce = DriveForce - DragForce;
	if (CurrentSpeedMPS > 0.05f)
	{
		ForwardTotalForce -= RollingResistance;
	}
	
	FMyVector3D AccelerationDirection = YawRotation.GetForwardVector();
	
	// 加速度の計算
	// 力/質量=加速度
	float ForwardAcceleration = ForwardTotalForce / BikeMass;
	FMyVector3D LongitudinalAcceleration = AccelerationDirection * (ForwardAcceleration * 100.0f);
	
	FMyVector3D UpVector(0.0f,0.0f,1.0f);
	FMyVector3D RightVector = UpVector.Cross(AccelerationDirection);
	
	// タイヤの横力(コーナリングコースの計算)
	// ハンドル切れ角
	float SteeringAngleRad = SteeringInput * 0.4f;
	
	// 前輪のスリップ角と横力
	float FrontSlipAngle = CalculateSlipAngle(SteeringAngleRad);
	float FrontLateralForce = CalculatePacejkaLateralForce(FrontSlipAngle);
	
	// 後輪のスリップ角と横力
	float RearSlipAngle = CalculateSlipAngle(0.0f);
	float RearLateralForce = CalculatePacejkaLateralForce(RearSlipAngle);
	
	// 前後輪の横力を合算
	float TotalLateralForce = FrontLateralForce + RearLateralForce;
	
	// 横加速度(f/m)
	float LateralAcceleration = TotalLateralForce / BikeMass;
	FMyVector3D LateralAccelerationVector = RightVector * (LateralAcceleration * 100.0f);
	
	// 前後加速度 + 横加速度を合算して速度を更新
	FMyVector3D TotalAccelerationVector = LongitudinalAcceleration + LateralAccelerationVector;
	PhysicsState.Velocity += TotalAccelerationVector * DeltaTime;
	
	// バンク角の計算
	UpdateLeanAngle(DeltaTime);
	FMyVector3D ForwardDirection = YawRotation.GetForwardVector();
	float CurrentYawRad = std::atan2(ForwardDirection.Y, ForwardDirection.X);
	constexpr float RadToDeg = 180.0f / 3.14159265358979323846f;
	float RollDeg  = PhysicsState.LeanAngle * RadToDeg;
	float PitchDeg = 0.0f;                         
	float YawDeg   = CurrentYawRad * RadToDeg;
	
	FMyRotator FinalRotator(RollDeg, PitchDeg, YawDeg);
	PhysicsState.Rotation = FinalRotator.ToQuat();
	
	// 完全停止ガード
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
		
		// ベースとなるYaw回転を更新
		YawRotation = YawRotation * DeltaQuat;
	}
}

FMyVector3D ABikeMovement::GetCameraForwardVector() const
{
	if (CameraComponent == nullptr)
	{
		return PhysicsState.Rotation.GetForwardVector();
	}
	
	// UEのカメラから現在のワールド座標を取得
	FRotator CameraRotation = CameraComponent->GetComponentRotation();
	
	// 上下や傾きを除外し、水平向きのみの回転作成
	FMyRotator YawRotator(0.0f,0.0f,CameraRotation.Yaw);
	
	return YawRotator.GetForwardVector();
}

// 車速からエンジン回転数(RPM)を逆算
float ABikeMovement::CalculateEngineRPM(float SpeedMPS)
{
	// 停車時や超低速時はアイドリング回転数を維持
	if (SpeedMPS > 0.5f)
	{
		return IdleRPM;
	}
	
	// タイヤの角速度(rad/s) = 速度(m/s) / 半径(m)
	float WheelAngularVelocity = SpeedMPS / RearWheelRadius;
	// 現在のギア比を取得
	int32 GearIndex = std::clamp(PhysicsState.CurrentGear,0,GearRatios.Num());
	float CurrentGearRatio = GearRatios[GearIndex];
	
	// 全体(トータル)の減速比 = 一次減速比 * ギア比 * ファイナルドライブ比
	float TotalReductionRatio = PrimaryReductionRatio * CurrentGearRatio * FinalDriveRatio;
	
	// エンジン回転速度(rad/s) = タイヤ回転速度 * トータル減速比
	float EngineRadPerSec = WheelAngularVelocity * TotalReductionRatio;
	
	// rad/sからRPM(1分あたりの回転数)へ変換 RPM = (rad/s) * 60 / 2π
	constexpr float RadPerSecToRPM = 60.0f / (2.0f * 3.14159265358979323846f);
	float CalculateRPM = EngineRadPerSec * RadPerSecToRPM;
	
	// アイドリング回転数 ～　レプリミット(MaxRPM)の範囲に制限する
	return std::clamp(CalculateRPM,IdleRPM,MaxRPM);
}

// RPMに応じたエンジントルク(Nm)を弾き出す関数
// トルクカーブ
float ABikeMovement::GetEngineTorqueAtRPM(float RPM) const
{
	// 最大トルク
	float MaxTorque = 110.0f;
	float NormalizeRPM = RPM / MaxRPM;
	
	// 簡易的なエンジントルクカーブ
	float TorqueFactor = 0.4f + 0.6f * std::sin(NormalizeRPM * 3.14159265358979323846f);
	
	return MaxTorque * TorqueFactor;
}

// エンジントルクからリアタイアの駆動力を計算
float ABikeMovement::CalculateDriveForce(float SpeedMPS)
{
	if (ThrottleInput <= 0.0f)
	{
		return 0.0f;
	}
	
	// 現在の車速からRPMを計算
	PhysicsState.EngineRPM  = CalculateEngineRPM(SpeedMPS);
	// RPMから発生エンジントルクを算出
	float BaseTorque = GetEngineTorqueAtRPM(PhysicsState.EngineRPM);
	// スロットル開度を乗算
	float EngineTorque = BaseTorque * ThrottleInput;
	// トータル減速比の計算
	int32 GearIndex = std::clamp(PhysicsState.CurrentGear,0,GearRatios.Num());
	float TotalReductionRatio = PrimaryReductionRatio * GearRatios[GearIndex] * FinalDriveRatio;
	
	// リアホイールに伝わトルク(Nm) = エンジントルク * トータル減速比 * 伝達効率
	float RearWheelTorque = EngineTorque * TotalReductionRatio * 0.95f;
	
	// タイヤが路面を押す駆動力(N) = ホイールトルク / タイヤ半径
	float DriveForce = RearWheelTorque / RearWheelRadius;
	
	return DriveForce;
}

// バイクの場合、タイヤを向けている向きと実際に車体が進んでいる方向に角度のズレが生じているためそのずれている角度を求める
// このずれが起きることによって車体を曲げる力が発生する(コーナリングフォース)
float ABikeMovement::CalculateSlipAngle(float SteeringAngleRad) const
{
	// 現在のスピードを取得
	float Speed = PhysicsState.Velocity.Size();
	// 低速時はスリップしない
	if (Speed < 10.0f)
	{
		return 0.0f;
	}
	
	// 進行方向ベクトル
	FMyVector3D MoveDirection = PhysicsState.Velocity.Normalize();
	// 車体の正面ベクトル
	FMyVector3D HeadingDirection = PhysicsState.Rotation.GetForwardVector();
	
	// 上方向の単位ベクトル(Z軸)
	FMyVector3D UpVector(0.0f,0.0f,1.0f);
	
	FMyVector3D CrossProduct = HeadingDirection.Cross(MoveDirection);
	float SlipSin = CrossProduct.Dot(UpVector);
	float SlipCos = HeadingDirection.Dot(MoveDirection);
	
	// 角度の跳ね上がりのないスリップ角を取得
	float BodySlipAngle = std::atan2(SlipSin,SlipCos);
	
	// 進行方向と車体正面の2D平面上での角度のズレ(スリップ角)を算出
	// 進行方向の角度
	// float MoveAngle = std::atan2(MoveDirection.Y, MoveDirection.X);
	// // 車体角度
	// float HeadingAngle = std::atan2(HeadingDirection.Y, HeadingDirection.X);
	// スリップ角を求めて返却
	return BodySlipAngle - SteeringAngleRad;;
}

float ABikeMovement::CalculatePacejkaLateralForce(float SlipAngle) const
{
	// 剛性
	constexpr float StiffnessFactor = 10.0f;
	// 形状因子
	constexpr float ShapeFactor = 1.3f;
	// ピーク摩擦価値 / 最大グリップ力
	constexpr float PeakFrictionValue = 2500.0f;
	// 曲率因子
	constexpr float CurvatureFactor = -0.1f;
	
	float BAlpha = StiffnessFactor * SlipAngle;
	float LateralForce = PeakFrictionValue * std::sin(BAlpha - CurvatureFactor * (BAlpha - std::atan(BAlpha)));
	
	return -LateralForce;
}

// バンク角による旋回推進力を計算
float ABikeMovement::CalculateCamberThrust(float LeanAngleRad) const
{
	// キャンバートラスト = キャンバー剛性 * バンク角(ラジアン)
	// 車体が右に傾くと右方向へ、左に傾くと左方向へ力が発生する
	return CamberStiffness * LeanAngleRad;
}

void ABikeMovement::UpdateLeanAngle(float DeltaTime)
{
	float SpeedMPS = PhysicsState.Velocity.Size() /100.0f;
	float Alpha = 1.0f - std::exp(-LeanResponseSpeed * DeltaTime);
	// 低速時は傾けない
	if (SpeedMPS < 1.0f)
	{
		// 正面に戻す
		PhysicsState.LeanAngle = std::lerp(PhysicsState.LeanAngle,0.0f,Alpha);
		return;
	}
	
	// 重力加速度
	constexpr float Gravity = 9.81f;
	// 旋回角速度
	float YawRate = PhysicsState.AngularVelocity.Z;
	
	// 目標バンク角 = atan((速度 * 角速度) / 重力)
	float TargetLeanAngle = -std::atan(SpeedMPS * YawRate / Gravity);
	
	// バンク角の上限制限(限界角度を超えないようにクランプ)
	TargetLeanAngle = std::clamp(TargetLeanAngle,-MaxLeanAngleRad,MaxLeanAngleRad);
	
	// なめらかに目標バンク角へ追従(線形補完: std::lerp)
	PhysicsState.LeanAngle = std::lerp(PhysicsState.LeanAngle,TargetLeanAngle,Alpha);
}

// シフトチェンジの処理
void ABikeMovement::OnShiftUp(const FInputActionValue& Value)
{
	if (PhysicsState.CurrentGear < GearRatios.Num())
	{
		PhysicsState.CurrentGear++;
	}
}

void ABikeMovement::OnShiftDown(const FInputActionValue& Value)
{
	if (PhysicsState.CurrentGear > 1)
	{
		PhysicsState.CurrentGear--;
	}
}

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
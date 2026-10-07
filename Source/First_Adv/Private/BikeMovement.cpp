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
	FVector StartLoc = GetActorLocation();
	FRotator StartRot = GetActorRotation();
	
	// PhysicsState の位置・回転をアクターの初期配置座標で同期
	PhysicsState.Position = FMyVector3D::FromFVector(StartLoc);
	
	FMyRotator InitYawRot(0.0f,0.0f,StartRot.Yaw);
	PhysicsState.Rotation = InitYawRot.ToQuat();
	PhysicsState.Velocity = FMyVector3D::ZeroVector();
	PhysicsState.AngularVelocity = FMyVector3D::ZeroVector();
	
	// RigidBodyの初期化
	RigidBodyState.Position = PhysicsState.Position;
	RigidBodyState.Orientation = PhysicsState.Rotation;
	RigidBodyState.LinearVelocity = FMyVector3D::ZeroVector();
	RigidBodyState.AngularVelocity = FMyVector3D::ZeroVector();
	
	// 剛体エンジンのパラメータ初期化 (車重 200kg, 慣性モーメント対角成分)
	FMyVector3D Inertia(35.0f, 75.0f, 45.0f);
	DynamicsEngine.InitializeParam(BikeMass, Inertia);
	
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
	
	// 現在の移動スピードを取得(m/s)
	// UEの単位が1unit = 1cm 変換の必要あり
	float CurrentSpeed = PhysicsState.Velocity.Size();
	float CurrentSpeedMPS = CurrentSpeed /100.0f;
	
	// 前方向の力の計算
	FMyVector3D LongitudinalAcceleration = ForwardVector(CurrentSpeedMPS);
	
	// ステアリング入力
	PhysicsState.AngularVelocity.Z = -SteeringInput * MaxYawRate;
	// 車体の向きの更新
	UpdateAngularVelocity(DeltaTime);
	
	// 加速度の方向ベクトル
	// バイクのForwardベクターの向きを取得
	FMyVector3D AccelerationDirection = PhysicsState.Rotation.GetForwardVector().Normalize();
	
	FMyVector3D UpVector(0.0f,0.0f,1.0f);
	FMyVector3D RightVector = UpVector.Cross(AccelerationDirection);
	
	// 横加速度と方向ベクトルから横加速度ベクトルを求める
	FMyVector3D LateralAccelerationVector = RightVector * (LateralAcceleration() * 100.0f);
	
	// 最終的に行いたいバイクの曲がるための計算
	// 旋回させる全体の横力 = 前後輪の横力 + キャラバートラスト(車体の傾き)
	
	// 前後加速度 + 横加速度を合算して速度を更新
	FMyVector3D TotalAccelerationVector = LongitudinalAcceleration + LateralAccelerationVector;
	PhysicsState.Velocity += TotalAccelerationVector * DeltaTime;
	
	BankAngle(DeltaTime);
	
	// 完全停止ガード
	if (ThrottleInput <= 0.0f && CurrentSpeed < 10.0f)
	{
		PhysicsState.Velocity = FMyVector3D(0.0f, 0.0f, 0.0f);
	}
	
	// 1フレームで移動したベクトルを取得して加算する
	PhysicsState.Position += PhysicsState.Velocity * DeltaTime;
}

// 基本はf = maに埋め込む形
// バイクの場合は二輪のため、F(drive) - F(resist) = m * a
// 前方向の力を加える
FMyVector3D ABikeMovement::ForwardVector(float CurrentSpeed)
{
	// 駆動力(F(drive))を取得
	float DriveForce =  CalculateDriveForce(CurrentSpeed);
	// 空気抵抗
	float DragForce = AirResistanceCoefficient*(CurrentSpeed*CurrentSpeed);
	// 転がり抵抗
	float RollingForce = 0.0f;
	
	// 動いているときだけ計算をする
	if (CurrentSpeed > 0.05f)
	{
		RollingForce = RollingResistance;
	}
	
	// 合力(m*a) = 駆動力 - 空気抵抗
	float ForwardTotalForce = DriveForce - (DragForce - RollingForce);
	
	// 加速度の計算
	// F(drive) - F(resist) = m * a
	// ForwardToTalForce = BikeMass * a
	float ForwardAcceleration = ForwardTotalForce / BikeMass;
	
	// 加速度の計算結果をUEの単位にして返す
	// m -> cm
	return YawRotation.GetForwardVector() * (ForwardAcceleration * 100.0f);
}

// 駆動力を計算
// F(drive) = T(wheel) / R(wheel)
// T = リアタイヤを軸にかかる回転力
// R = タイヤ半径
// エンジントルクを考慮した場合
// F(drive) = (T(engineTorque)*i(減速比)*駆動伝達効率/R)*ThrottleInput
float ABikeMovement::CalculateDriveForce(float SpeedMPS)
{
	// スロットルが開いてなければ0を返す = 駆動力0(動いてない)
	if (ThrottleInput <= 0.0f)
	{
		return 0.0f;
	}
	
	// 現在の車速からRPMを計算
	// RPM = 1分間に何回転するか(回転速度)
	PhysicsState.EngineRPM  = CalculateEngineRPM(SpeedMPS);
	// RPMから発生エンジントルクを算出
	// エンジンが発生させている回転力(トルク)
	float BaseTorque = GetEngineTorqueAtRPM(PhysicsState.EngineRPM);
	// 現在のスロットルを乗算し実際のトルクを求める
	float EngineTorque = BaseTorque * ThrottleInput;
	// リアホイールに伝わるトルク(Nm) = エンジントルク * トータル減速比 * 伝達効率
	// エンジントルクからリアホイールの回転力を取得
	float RearWheelTorque = EngineTorque * TotalReductionRatio() * 0.95f;
	// タイヤが路面を押す駆動力(N) = ホイールトルク / タイヤ半径
	return RearWheelTorque / RearWheelRadius;
}

// 車速からエンジン回転数(RPM)を計算
// RPM = s/m(車速)/R(wheel) * トータル減速比 * 60 / 2π
float ABikeMovement::CalculateEngineRPM(float SpeedMPS)
{
	// 停車時や超低速時はアイドリング回転数を維持
	if (SpeedMPS > 0.5f)
	{
		return IdleRPM;
	}
	
	// タイヤの角速度(rad/s) = 速度(m/s) / 半径(m)
	float WheelAngularVelocity = SpeedMPS / RearWheelRadius;
	// rad/sからRPM(1分あたりの回転数)へ変換 RPM = (rad/s) * 60 / 2π
	constexpr float RadPerSecToRPM = 60.0f / (2.0f * 3.14159265358979323846f);
	// RPM = s/m(車速)/R(wheel) * トータル減速比 * RadPerSecToRPM
	float CalculateRPM = WheelAngularVelocity * TotalReductionRatio() * RadPerSecToRPM;
	
	// アイドリング回転数 ～　レプリミット(MaxRPM)の範囲に制限する
	return std::clamp(CalculateRPM,IdleRPM,MaxRPM);
}

// RPMに応じたエンジントルク(Nm)を返す関数
// トルクカーブ
// 低回転時 : 力が弱い 中回転 : 最大値 高回転 : 力が落ちる
// 山型のグラフの特性
float ABikeMovement::GetEngineTorqueAtRPM(float RPM) const
{
	// 現在のトルクを正規化する
	float NormalizeRPM = RPM / MaxRPM;
	
	// 簡易的なエンジントルクカーブ
	// グラフの滑らかさ(最低値の保証をするために固定値化)
	// 発進時にスピードを出したいか後からスピードがでるようにしたいか
	// 走りやすさを調整できる
	float TorqueFactor = TorqueCurve + 1.0 - TorqueCurve * std::sin(NormalizeRPM * 3.14159265358979323846f);
	
	return MaxTorque * TorqueFactor;
}

// トータルの減速比を返す
float ABikeMovement::TotalReductionRatio()
{
	// 境界外にいかないように強制
	int32 GearIndex = std::clamp(PhysicsState.CurrentGear,0,GearRatios.Num());
	// バイクは3段階の減速メカニズムが存在
	// すべてをかけ合わせたトータルから全体で何倍のトルクが増幅されるかを取得
	return  PrimaryReductionRatio * GearRatios[GearIndex] * FinalDriveRatio;
}

// 横方向の加速度を返す
// F = maで横に向かって車体をどれくらいの勢いで押し曲げるかを計算
float ABikeMovement::LateralAcceleration() const
{
	// タイヤの横力(コーナリングコースの計算)
	// ハンドル切れ角
	// 市販車の平均が30度くらいなため0.5で設定
	float SteeringAngleRad = SteeringInput * 0.5f;
	
	// 前輪の路面に対する角度のズレを求める(スリップ角)
	// 前輪は切れ角の影響を直接うけるので切れ角から前輪のスリップ角を求める
	float FrontSlipAngle = CalculateSlipAngle(SteeringAngleRad);
	// スリップからマジックフォーミュラを用いて前輪が地面をとらえる横力(N)を求める
	float FrontLateralForce = CalculatePacejkaLateralForce(FrontSlipAngle);
	
	// 後輪のスリップ角と横力
	// 後輪はハンドルを切っても回転しないのでスリップ角は0
	float RearSlipAngle = CalculateSlipAngle(0.0f);
	// 車体の滑り角度から後輪の横力を取得
	float RearLateralForce = CalculatePacejkaLateralForce(RearSlipAngle);
	
	// 前後輪の横力(N)を合算
	float TotalLateralForce = FrontLateralForce + RearLateralForce;
	
	// 横加速度(a(m/s2))を求めて返す
	return  TotalLateralForce / BikeMass;
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

// バイクの場合、タイヤを向けている向きと実際に車体が進んでいる方向に角度のズレが生じているためそのずれている角度を求める(スリップ角)
// ズレを求めるには外積と内積を使う必要がある
float ABikeMovement::CalculateSlipAngle(float SteeringAngleRad) const
{
	// 現在のスピードを取得
	float Speed = PhysicsState.Velocity.Size();
	// 低速時はスリップしない
	if (Speed < 10.0f)
	{
		return 0.0f;
	}
	
	// 実際に行きたい方向ベクトル
	FMyVector3D MoveDirection = PhysicsState.Velocity.Normalize();
	// 車体の正面ベクトル
	FMyVector3D HeadingDirection = PhysicsState.Rotation.GetForwardVector();
	
	// 上方向の単位ベクトル(Z軸)
	FMyVector3D UpVector(0.0f,0.0f,1.0f);
	
	// 正面と進行方向の外積をとって向きのベクトル取得する
	FMyVector3D CrossProduct = HeadingDirection.Cross(MoveDirection);
	// 上方向の内積をとることでプラスかマイナスかの数値を取得する
	// 高さを求める
	float SlipSin = CrossProduct.Dot(UpVector);
	// 内積(底辺を求める)
	float SlipCos = HeadingDirection.Dot(MoveDirection);
	
	// 角度の跳ね上がりのないスリップ角を取得
	// 三角形から滑り角度を計算
	// atan(高さ、底辺)
	float BodySlipAngle = std::atan2(SlipSin,SlipCos);
	// スリップ角を求めて返却
	// α(front) = 車体事態の滑り角度 - タイヤ自身を旋回方向へ向けた角度
	return BodySlipAngle - SteeringAngleRad;
}

// マジックフォーミュラ
// 横力の計算
// F(y) = D*sin(C * arctan(B(α) - E(B(α) - arctan(B(α)))))
float ABikeMovement::CalculatePacejkaLateralForce(float SlipAngle) const
{
	// B : タイヤの硬さ
	constexpr float StiffnessFactor = 10.0f;
	// C : カーブや丸み
	constexpr float ShapeFactor = 1.3f;
	//  D : タイヤの限界グリップ力 
	constexpr float PeakFrictionValue = 2500.0f;
	// E : 限界後のグリップの落ち具合
	constexpr float CurvatureFactor = -0.1f;
	
	float BAlpha = StiffnessFactor * SlipAngle;
	// 内側のカーブ計算
	// B(α) - E(B(α) - arctan(B(α))
	float Inner = BAlpha - CurvatureFactor * (BAlpha - std::atan(BAlpha));
	// C(ShapeFactor)とatanを適用して滑らかな限界突破挙動を作る
	float LateralForce = PeakFrictionValue * std::sin(ShapeFactor * std::atan(Inner));
	
	return -LateralForce;
}

// バンク角から車体を横に傾ける
void ABikeMovement::BankAngle(float DeltaTime)
{
	// 車体を傾ける
	UpdateLeanAngle(DeltaTime);
	FMyVector3D ForwardDirection = YawRotation.GetForwardVector();
	// 水平方向にどこを向いているか
	// ForwardのX,Y成分から角度を求める(arctan(Y,Z))
	float CurrentYawRad = std::atan2(ForwardDirection.Y, ForwardDirection.X);
	// 度数法への変換
	constexpr float RadToDeg = 180.0f / 3.14159265358979323846f;
	// 横への倒れこみ(バンク角)
	float RollDeg  = PhysicsState.LeanAngle * RadToDeg;
	// 前後の傾き
	float PitchDeg = 0.0f;      
	// 旋回角
	float YawDeg   = CurrentYawRad * RadToDeg;
	
	// オイラー角からQuatに変換
	FMyRotator FinalRotator(RollDeg, PitchDeg, YawDeg);
	PhysicsState.Rotation = FinalRotator.ToQuat();
}

// 車体をどのくらい傾けるかを求めlerpで滑らかに傾けさせる
void ABikeMovement::UpdateLeanAngle(float DeltaTime)
{
	float SpeedMPS = PhysicsState.Velocity.Size() /100.0f;
	// 現在の補完率を保存
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
	// θ = artan((速度 * 角速度) / 重力)
	float TargetLeanAngle = -std::atan(SpeedMPS * YawRate / Gravity);
	
	// バンク角の上限制限(限界角度を超えないようにクランプ)
	TargetLeanAngle = std::clamp(TargetLeanAngle,-MaxLeanAngleRad,MaxLeanAngleRad);
	
	// なめらかに目標バンク角へ追従(線形補完: std::lerp)
	PhysicsState.LeanAngle = std::lerp(PhysicsState.LeanAngle,TargetLeanAngle,Alpha);
}

// バンク角による旋回推進力を計算
float ABikeMovement::CalculateCamberThrust(float LeanAngleRad) const
{
	// キャンバートラスト = キャンバー剛性 * バンク角(ラジアン)
	// 車体が右に傾くと右方向へ、左に傾くと左方向へ力が発生する
	return CamberStiffness * LeanAngleRad;
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
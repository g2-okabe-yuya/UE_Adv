#include "BikeMovement.h"
#include "InputMappingContext.h"
#include "Camera/CameraComponent.h"
#include  "InputAction.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "NiagaraComponent.h"
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

void ABikeMovement::Initialize()
{
	// 初期位置の保存
	FVector StartLocation = GetActorLocation();
	FRotator StartRotation = GetActorRotation();
	
	// PhysicsState の位置・回転をアクターの初期配置座標で同期
	PhysicsState.Position = FMyVector3D::FromFVector(StartLocation);
	// 初期回転を保存
	// 初期位置の回転がおかしいのでここも修正が必要
	//FMyRotator InitRotation(StartRotation.Pitch,StartRotation.Yaw,StartRotation.Roll);
	PhysicsState.Rotation = FMyQuat::FromFQuat(StartRotation.Quaternion());
	YawRotation = FMyQuat::FromFQuat(StartRotation.Quaternion());
	// 速度・角速度の初期化
	PhysicsState.Velocity = FMyVector3D::ZeroVector();
	PhysicsState.AngularVelocity = FMyVector3D::ZeroVector();
	// 回転数を初期化
	PhysicsState.EngineRPM = IdleRPM;
	// ギアを1速にする
	PhysicsState.CurrentGear = 1;
	
	// 入力値の初期化
	BrakeInput = 0.0f;
	ThrottleInput = 0.0f;
	
	// カメラとスプリングアームを取得
	if (CameraComponent == nullptr)
	{
		CameraComponent = FindComponentByClass<UCameraComponent>();
	}
	
	if (SpringArmComponent == nullptr)
	{
		SpringArmComponent = FindComponentByClass<USpringArmComponent>();
	}
	
	// エフェクトを取得
	// 火花の散るエフェクトを作成
	// 現在2つを実装しているが1つにしてもいいかも
	if (CurveEngineFireEffect == nullptr)
	{
		TArray<UNiagaraComponent*> NiagaraComponents;
		GetComponents<UNiagaraComponent>(NiagaraComponents);

		for (UNiagaraComponent* Component : NiagaraComponents)
		{
			if (!Component)
			{
				continue;
			}
			
			if (Component->GetFName() == FName("NCurveEngineFireEffect"))
			{
				CurveEngineFireEffect = Component;
			}
		}

		// 名前で見つからなければ最初に見つかったNiagaraコンポーネントを割り当てる fallback
		if (CurveEngineFireEffect == nullptr && NiagaraComponents.Num() > 0)
		{
			CurveEngineFireEffect = NiagaraComponents[0];
		}
	}
}

void ABikeMovement::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	//CheckDeath();
	
	if (DeltaTime <= 0.0f)
	{
		return;
	}
	
	// Knockback時に一定時間行動制限を付与するための拘束時間
	// この実装は修正が必要
	if (IsKnockBack)
	{
		KnockbackTimer -= DeltaTime;
		if (KnockbackTimer <= 0.0f)
		{
			// 操作不能解除
			IsKnockBack = false;
		}
	}
	
	// 地面の接地判定
	// 空中にいるときの挙動制御
	UpdateGroundState(DeltaTime);

	// 4. アクターへの位置反映
	FHitResult Hit;
	// 	bスイープ = 目的地までスイープするかどうか、途中で重なりが発生する場合はそれを検知し、
	// 	何かに遮られた場合は目標地点の手前で停止します。
	// 	ルートコンポーネントのみがスイープされ、衝突がないかチェックされます。
	// 	子コンポーネントはスイープされずに移動します。衝突が無効になっている場合は、この設定は効果がありません。
	SetActorLocationAndRotation(PhysicsState.Position.ToFVector(), PhysicsState.Rotation.ToFQuat(), false, &Hit);
	
	// 壁への衝突判定
	// 壁の判定がとれていない
	if (Hit.bBlockingHit)
	{
		// ヒット面の法線のZ成分が小さいときに壁とみなす
		if (FMath::Abs(Hit.ImpactNormal.Z) < 0.5f)
		{
			OnWallImpact(FMyVector3D::FromFVector(Hit.ImpactNormal));
		}
	}
}

// 地面の接地判定および状態更新をおこなう
void ABikeMovement::UpdateGroundState(float DeltaTime)
{
    // 前輪と後輪にそれぞれ独立したレイキャスト（SphereTrace）を実行
    FHitResult FrontHit, RearHit;
    bool OnFrontGrounded = CheckFrontWheelGrounded(FrontHit);
    bool OnRearGrounded  = CheckRearWheelGrounded(RearHit);

    // 貫通防止のため、どちらか一方でも接していれば走行（接地）状態とみなす
    IsGrounded = OnFrontGrounded || OnRearGrounded;

    if (IsGrounded)
    {
        // 接地中は重力を反映しない
    	if (PhysicsState.Velocity.Z < 0.0f)
    	{
    		PhysicsState.Velocity.Z = 0.0f;
    	}
    	
    	// アクセルとブレーキの計算
    	UpdateCustomPhysics(DeltaTime);
    	
    	// 速度ベクトルに基づいてX,Y座標を動かす
    	PhysicsState.Position.X += PhysicsState.Velocity.X * DeltaTime;
    	PhysicsState.Position.Y += PhysicsState.Velocity.Y * DeltaTime;
    	
    	// 接地状況に応じた車高を引き上げる計算
    	// 地面の法線ベクトルからバイクの角度を変える
    	FVector AverageNormal = FVector::UpVector;
    	FVector GroundImpactPoint = PhysicsState.Position.ToFVector();
    	
    	// 両輪接地
    	if (OnFrontGrounded && OnRearGrounded)
    	{
    		// 両輪の法線の平均を求める
    		AverageNormal = (FrontHit.ImpactNormal + RearHit.ImpactNormal).GetSafeNormal();
    		// 前後輪の中間座標
    		GroundImpactPoint = (FrontHit.ImpactPoint + RearHit.ImpactPoint) * 0.5f;
    	}
    	// 前輪のみ接地
    	else if (!OnRearGrounded && OnFrontGrounded)
    	{
    		// 前輪のみの場合は前輪のレイキャストがとらえた地面の法線の向きに合わせる
    		AverageNormal = FrontHit.ImpactNormal.GetSafeNormal();
    		GroundImpactPoint = FrontHit.ImpactPoint;
    	}
    	// 後輪のみ接地
	    else if (!OnFrontGrounded && OnRearGrounded)
	    {
	    	// 前輪の後輪版
	    	AverageNormal = RearHit.ImpactNormal.GetSafeNormal();
	    	GroundImpactPoint = RearHit.ImpactPoint;
	    }
    	
    	GroundNormal = FMyVector3D::FromFVector(AverageNormal);
    	float TargetZ = GroundImpactPoint.Z + CustomRideHeight;
    	float CurrentZ = PhysicsState.Position.Z;
    	// 道路の上を走る
    	float SmoothZ = FMath::FInterpTo(CurrentZ, TargetZ, DeltaTime, 30.0f);
    	
    	PhysicsState.Position.Z = SmoothZ;

    	// 5. 下向き速度のリセット
    	if (PhysicsState.Velocity.Z < 0.0f)
    	{
    		PhysicsState.Velocity.Z = 0.0f;
    	}
    }
    else
    {
	    // タイヤがついていないときは落下のための力を追加
    	SetGravity(DeltaTime);
    	// 慣性によって変化する移動計算を行う
    	PhysicsState.Position.X += PhysicsState.Velocity.X * DeltaTime;
    	PhysicsState.Position.Y += PhysicsState.Velocity.Y * DeltaTime;
    	PhysicsState.Position.Z += PhysicsState.Velocity.Z * DeltaTime;
    	// 空中での傾きをゆっくり水平にもどしていく
    	PhysicsState.LeanAngle = FMath::FInterpTo(PhysicsState.LeanAngle, 0.0f, DeltaTime, 3.0f);
    	// 空中での前後傾きの計算
    	FRotator CurrentRotation = PhysicsState.Rotation.ToFQuat().Rotator();
    	CurrentRotation.Normalize();
    	// 昇降速度に応じた目標のPitchを作成
    	// Rollは0.0fにしPitchは上昇/落下に応じた角度
    	
    	float CurrentYaw   = CurrentRotation.Yaw;
    	float CurrentPitch = CurrentRotation.Pitch;
    	float CurrentRoll  = CurrentRotation.Roll;
    	
    	float TargetPitch  = CurrentPitch;    	
    	
    	// 上向き方向を向いている時は水平に戻す
    	if (CurrentPitch > 0.0f)
    	{
    		TargetPitch = FMath::FInterpTo(CurrentPitch, 0.0f, DeltaTime, 1.5f);
    	}
	    
    	// 目標回転を設定
    	FRotator TargetAirRotation(TargetPitch, CurrentYaw,CurrentRoll);
    	
    	// Quaternionにして計算
    	FQuat CurrentQuat = PhysicsState.Rotation.ToFQuat();
    	FQuat TargetQuat  = TargetAirRotation.Quaternion();
    	
    	PhysicsState.Rotation = FMyQuat::FromFQuat(FMath::QInterpTo(CurrentQuat, TargetQuat, DeltaTime, 3.0f));
    }
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
		if (IA_Gear1)
		{
			EnhancedInputComponent->BindAction(IA_Gear1, ETriggerEvent::Started, this, &ABikeMovement::OnChangedGear1);
		}
		if (IA_Gear2)
		{
			EnhancedInputComponent->BindAction(IA_Gear2, ETriggerEvent::Started, this, &ABikeMovement::OnChangedGear2);
		}
		if (IA_Gear3)
		{
			EnhancedInputComponent->BindAction(IA_Gear3, ETriggerEvent::Started, this, &ABikeMovement::OnChangedGear3);
		}
		if (IA_Gear4)
		{
			EnhancedInputComponent->BindAction(IA_Gear4, ETriggerEvent::Started, this, &ABikeMovement::OnChangedGear4);
		}
		if (IA_Gear5)
		{
			EnhancedInputComponent->BindAction(IA_Gear5, ETriggerEvent::Started, this, &ABikeMovement::OnChangedGear5);
		}
		if (IA_Gear6)
		{
			EnhancedInputComponent->BindAction(IA_Gear6, ETriggerEvent::Started, this, &ABikeMovement::OnChangedGear6);
		}
		
		if (IA_Brake)
		{
			EnhancedInputComponent->BindAction(IA_Brake,ETriggerEvent::Triggered, this, &ABikeMovement::OnBrakeInput);
			EnhancedInputComponent->BindAction(IA_Brake,ETriggerEvent::Completed,this, &ABikeMovement::OnBrakeCompleted);
		}
	}
}

// アクセルとブレーキの力を計算する
// 回転計算をする
void ABikeMovement::UpdateCustomPhysics(float DeltaTime)
{
	if (DeltaTime <= 0.0f)
	{
		return;
	}
	
	float EffectiveThrottle = 0.0f;
	if (BrakeInput < 0.001f)
	{
		EffectiveThrottle = ThrottleInput;
	}
	
	// 現在の移動スピードを取得(m/s)
	// UEの単位が1unit = 1cm 変換の必要あり
	float CurrentSpeed = PhysicsState.Velocity.Size();
	float CurrentSpeedMPS = CurrentSpeed /100.0f;
	
	// UI表示用の速度を計算
	float UISpeedMPS = CurrentSpeedMPS;
	
	// ミニターボ発動中なら見た目の速度上昇幅を抑える
	if (CurrentBoostTimer > 0.0f)
	{
		float UIEditBoost = BoostForce / 100.0f / 3.0f;
		
		// 通常のスピードの到達想定値 + 抑えたブースト分に補正
		UISpeedMPS = FMath::Min(CurrentSpeedMPS, CurrentSpeedMPS - BoostForce / 100.0f + UIEditBoost); 
	}
	
	// カウントダウン中の演出
	if (IsCountdown)
	{
		// カウントダウン中はRPMのみあげる
		float TargetRPM = FMath::Lerp(IdleRPM,6000,EffectiveThrottle);
		PhysicsState.EngineRPM = FMath::FInterpTo(PhysicsState.EngineRPM, TargetRPM, DeltaTime, 10.0f);
	}
	// アクセルが踏まれていないときの処理
	else if (EffectiveThrottle < 0.05f)
	{
		// エンジンの目標値を計算
		float TargetRPM = CalculateEngineRPM(CurrentSpeedMPS);
		
		// 停止状態の時はIdleに設定する
		if (CurrentSpeed < 0.1f)
		{
			TargetRPM = IdleRPM;
		}
		
		// 高回転であるほど落ちる量を増やす
		float RpmRatio = PhysicsState.EngineRPM / MaxRPM;
		float DropSpeed = std::lerp(6.0f,15.0f,RpmRatio * RpmRatio);
		
		PhysicsState.EngineRPM = FMath::FInterpTo(PhysicsState.EngineRPM, TargetRPM, DeltaTime, DropSpeed);
		// アイドル回転数以下にならないように抑制
		PhysicsState.EngineRPM = FMath::Max(PhysicsState.EngineRPM, IdleRPM);
	}
	// アクセルが踏まれているならEngineを回転させる
	else
	{
		PhysicsState.EngineRPM = CalculateEngineRPM(CurrentSpeedMPS);
	}
	
	// 現在のスピードが更新されたことを通知
	if (OnSpeedChanged.IsBound())
	{
		OnSpeedChanged.Broadcast(UISpeedMPS);
	}
	
	// 現在のエンジンをUIに反映
	if (OnRpmChanged.IsBound())
	{
		OnRpmChanged.Broadcast(PhysicsState.EngineRPM);
	}
	
	// 前方向の力の計算
	FMyVector3D LongitudinalAcceleration = ForwardVector(CurrentSpeedMPS);
	
	PhysicsState.Velocity += LongitudinalAcceleration * DeltaTime;
	
	FMyVector3D ForwardDir = PhysicsState.Rotation.GetForwardVector().Normalize();
	float ForwardSpeed = PhysicsState.Velocity.Dot(ForwardDir);
	if (ForwardSpeed <= 0.0f && EffectiveThrottle <= 0.001f)
	{
		PhysicsState.Velocity = FMyVector3D::ZeroVector();
	}
	// カウントダウン中は速度の処理をスキップさせる
	if (IsCountdown)
	{
		PhysicsState.Velocity = FMyVector3D::ZeroVector();
		PhysicsState.AngularVelocity = FMyVector3D::ZeroVector();
	}
	
	// 横方向の計算
	PhysicsState.AngularVelocity.Z = SteeringInput * MaxYawRate;
	// 車体の向きの更新
	UpdateAngularVelocity(DeltaTime);
	// 最終的に行いたいバイクの曲がるための計算
	// 旋回させる全体の横力 = 前後輪の横力 + キャラバートラスト(車体の傾き)
	UpdateRotation(DeltaTime);
	// ミニターボの加速処理
	UpdateMiniTurbo(DeltaTime,CurrentSpeedMPS);
	
	// 回転後にForward方向が更新されず横向きの力が働かないように制限する
	// ToDo : ここもバイクてきにはあっているのではないか
	if (CurrentSpeedMPS > 0.5f)
	{
		FMyVector3D AccelerationDirection = PhysicsState.Rotation.GetForwardVector().Normalize();
       
		// 現在の実際の速度の大きさ（減速後の値）を使ってTargetVelocityを作成
		float UpdatedSpeed = PhysicsState.Velocity.Size();
		FMyVector3D TargetVelocity = AccelerationDirection * UpdatedSpeed;
       
		float CurrentVectorZ = PhysicsState.Velocity.Z;
       
		FMyVector3D InterResult = FMyVector3D::FromFVector(FMath::VInterpTo(
		   PhysicsState.Velocity.ToFVector(),
		   TargetVelocity.ToFVector(),
		   DeltaTime,
		   InterpSpeed));
       
		PhysicsState.Velocity = InterResult;
		PhysicsState.Velocity.Z = CurrentVectorZ;
       
		// リミッター制限
		const float RPMToRadPerSec = 2.0f * 3.14159265358979323846f / 60.0f;
		float MaxAllowedSpeedMPS = (MaxRPM * RPMToRadPerSec / TotalReductionRatio()) * RearWheelRadius;
		float MaxAllowedSpeedCMS = MaxAllowedSpeedMPS * 100.0f;

		FVector HorizontalVel(PhysicsState.Velocity.X, PhysicsState.Velocity.Y, 0.0f);
		if (HorizontalVel.Size() > MaxAllowedSpeedCMS)
		{
			HorizontalVel = HorizontalVel.GetSafeNormal() * MaxAllowedSpeedCMS;
			PhysicsState.Velocity.X = HorizontalVel.X;
			PhysicsState.Velocity.Y = HorizontalVel.Y;
		}
	}
}

// 基本はf = maに埋め込む形
// バイクの場合は二輪のため、F(drive) - F(resist) = m * a
// 前方向の力を加える
FMyVector3D ABikeMovement::ForwardVector(float CurrentSpeed)
{
	float EffectiveThrottle = 0.0f;
	
	if (BrakeInput < 0.001f)
	{
		EffectiveThrottle = ThrottleInput;
	}
	
	// 駆動力(F(drive))を取得
	float DriveForce;
	if (EffectiveThrottle <= 0.001f)
	{
		DriveForce = 0.0f;
	}
	else
	{
		DriveForce = CalculateDriveForce(CurrentSpeed);
	}
	
	// 空気抵抗
	float DragForce = AirResistanceCoefficient*(CurrentSpeed*CurrentSpeed);
	float RollingForce = 0.0f;
	// 転がり抵抗
	if (CurrentSpeed > 0.05f)
	{
		RollingForce = RollingResistance;
	}
	
	// ブレーキ力の計算
	// 最大ブレーキ力の10％の力をエンジンブレーキとして常にかけておく
	TotalBrakeForce = BikeMass * 9.81f * friction;
	BrakeForce = TotalBrakeForce * BrakeInput * 10.0f;
	
	float EngineForce = 0.0f;
	
	//　アクセルを踏んでいなければエンジンブレーキをかける
	if (EffectiveThrottle < 0.05 && CurrentSpeed > 0.1f)
	{
		// 最大ブレーキ力の約15％を上限として、RPM割合に応じてエンブレを掛ける
		float RpmRatio = PhysicsState.EngineRPM / MaxRPM;
		EngineForce = TotalBrakeForce * 0.15f * RpmRatio * 5;
	}
	
	// 抵抗力
	float TotalResistance = DragForce + RollingForce + BrakeForce + EngineForce;
	// 加速度の計算
	// F(drive) - F(resist) = m * a
	// ForwardToTalForce = BikeMass * a
	float ForwardAcceleration =0.0f;
	
	// アクセルOFFの時、抵抗力が「今の速度を0にする力」を超えないようクランプする
	if (EffectiveThrottle > 0.001f)
	{
		// 加速時: 駆動力 - 抵抗力
		float ForwardTotalForce = DriveForce - TotalResistance;
		ForwardAcceleration = ForwardTotalForce / BikeMass;
	}
	else
	{
		if (CurrentSpeed <= 0.001f)
		{
			ForwardAcceleration = 0.0f;
		}
		else
		{
			// 速度がある時だけ減速度を発生させる
			ForwardAcceleration = -TotalResistance / BikeMass;
		}
	}
	
	// 加速度の計算結果をUEの単位にして返す
	// m -> cm
	// return YawRotation.GetForwardVector().Normalize() * (ForwardAcceleration * 100.0f);
	return PhysicsState.Rotation.GetForwardVector().Normalize() * (ForwardAcceleration * 100.0f);
}

// 駆動力を計算
// F(drive) = T(wheel) / R(wheel)
// T = リアタイヤを軸にかかる回転力
// R = タイヤ半径
// エンジントルクを考慮した場合
// F(drive) = (T(engineTorque)*i(減速比)*駆動伝達効率/R)*ThrottleInput
float ABikeMovement::CalculateDriveForce(float SpeedMPS)
{
	if (ThrottleInput <= 0.001f)
	{
		return 0.0f;
	}
	
	float CalculatedRPM = CalculateEngineRPM(SpeedMPS);
	// 現在の車速からRPMを計算
	// RPM = 1分間に何回転するか(回転速度)
	PhysicsState.EngineRPM  = std::clamp(CalculatedRPM, IdleRPM, MaxRPM);
	// 現在のギアでMaxRPMを超えている場合
	// 完全に0にするのではなく。速度を維持するレベルまでトルクを衰退する
	float RevLimiterFactor = 1.0f;
	// 例: MaxRPMの95%から減衰を開始したい場合
	const float RevLimitStartRatio = 0.90f; 

	if (CalculatedRPM >= MaxRPM)
	{
		// レブリミット到達時は完全に駆動力をカット
		RevLimiterFactor = 0.0f;
	}
	else if (CalculatedRPM > MaxRPM * RevLimitStartRatio)
	{
		// 減衰ゾーン内での割合（0.0 〜 1.0）を計算
		float Range = MaxRPM * (1.0f - RevLimitStartRatio);
		float Progress = (CalculatedRPM - MaxRPM * RevLimitStartRatio) / Range;

		// 0.6f の部分を大きくすると素早くカット、小さくするとじんわりカット
		RevLimiterFactor = FMath::FInterpTo(1.0f, 0.0f, Progress, 2.0f);
	}
	// RPMから発生エンジントルクを算出
	// エンジンが発生させている回転力(トルク)
	// 物体が回転させる力
	float BaseTorque = GetEngineTorqueAtRPM(PhysicsState.EngineRPM);
	// 現在のスロットルを乗算し実際のトルクを求める
	float EngineTorque = BaseTorque * ThrottleInput * RevLimiterFactor;
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
	
	return MaxTorque * TorqueFactor * 5.0;
}

// トータルの減速比を返す
float ABikeMovement::TotalReductionRatio()
{
	// 0速の値をいれているが参照はしてほしくないので0は弾く
	int32 GearIndex = std::clamp(PhysicsState.CurrentGear,1,GearRatios.Num() - 1);
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
	// 後輪はハンドルを切っても回転しないので滑り角のみが反映される
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
		FMyVector3D RotationAxis = PhysicsState.AngularVelocity;
		RotationAxis = RotationAxis.Normalize();
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
	
	if (FMath::Abs(SteeringInput) < 0.01f && FMath::Abs(BodySlipAngle - SteeringAngleRad) < 0.02f)
	{
		return 0.0f;
	}
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
	const float StiffnessFactor = 10.0f;
	// C : カーブや丸み
	const float ShapeFactor = 1.3f;
	//  D : タイヤの限界グリップ力 
	const float PeakFrictionValue = 2500.0f;
	// E : 限界後のグリップの落ち具合
	const float CurvatureFactor = -0.1f;
	
	float BAlpha = StiffnessFactor * SlipAngle;
	// 内側のカーブ計算
	// B(α) - E(B(α) - arctan(B(α))
	float Inner = BAlpha - CurvatureFactor * (BAlpha - std::atan(BAlpha));
	// C(ShapeFactor)とatanを適用して滑らかな限界突破挙動を作る
	float LateralForce = PeakFrictionValue * std::sin(ShapeFactor * std::atan(Inner));
	
	return -LateralForce;
}

// バンク角から車体を横に傾ける
void ABikeMovement::UpdateRotation(float DeltaTime)
{
	// 傾けるべき角度
	BankAngle(DeltaTime);
	// バイクの方向ベクトルを取得
	FMyVector3D ForwardDirection = YawRotation.GetForwardVector().Normalize();
	// 地面の法線と進行方向から路面に沿ったRotationを取得
	FVector SurfaceUp = GroundNormal.ToFVector();
	if (SurfaceUp.IsNearlyZero())
	{
		SurfaceUp = FVector::UpVector;
	}
	// 方向ベクトルを軸としてバンク角分傾くQuadを作成
	// 前方向ベクトルを路面に投影（路面と水平にする）
	FVector SurfaceForward = FVector::VectorPlaneProject(ForwardDirection.ToFVector(), SurfaceUp).GetSafeNormal();
	FVector SurfaceRight = FVector::CrossProduct(SurfaceUp, SurfaceForward).GetSafeNormal();
	
	FMatrix SurfaceMatrix;
	SurfaceMatrix.SetAxes(&SurfaceForward, &SurfaceRight, &SurfaceUp);
	FQuat SurfaceQuat = SurfaceMatrix.ToQuat();
	// バンク角の合成
	FQuat LeanQuat = FQuat(SurfaceForward, PhysicsState.LeanAngle);
	// 最終的な回転を更新（路面の坂道傾斜 + バンク角が両立する）
	PhysicsState.Rotation = FMyQuat::FromFQuat(LeanQuat * SurfaceQuat);
}

// 車体をどのくらい傾けるかを求めlerpで滑らかに傾けさせる
// Angleを更新する
void ABikeMovement::BankAngle(float DeltaTime)
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
	PhysicsState.LeanAngle = std::lerp(PhysicsState.LeanAngle,TargetLeanAngle,Alpha);
}

// バンク角による旋回推進力を計算
float ABikeMovement::CalculateCamberThrust(float LeanAngleRad) const
{
	// キャンバートラスト = キャンバー剛性 * バンク角(ラジアン)
	// 車体が右に傾くと右方向へ、左に傾くと左方向へ力が発生する
	return CamberStiffness * LeanAngleRad;
}

// ギアの切り替えをおこなう
void ABikeMovement::SetGearDirectly(int32 NewGear)
{
	// 配列の範囲内(1～6)であることをチェックして変更
	if (NewGear >= 1 && NewGear < GearRatios.Num())
	{
		PhysicsState.CurrentGear = NewGear;
		if (OnGearTextChanged.IsBound())
		{
			OnGearTextChanged.Broadcast(PhysicsState.CurrentGear);
		}
	}
}

void ABikeMovement::OnWallImpact(const FMyVector3D& ImpactNormal)
{
	// すでにKnockbackしている状態なら無効化
	if (IsKnockBack)
	{
		return;
	}
	
	IsKnockBack = true;
	// Timerをセット
	// TIckでまわしてます
	KnockbackTimer = KnockbackDuration;
	
	// 前進速度を0にしてぶつかった方向と逆向きの力を与える
	FMyVector3D Impulse = FMyVector3D::FromFVector(ImpactNormal.ToFVector()).Normalize();
	
	// Z方向への飛び跳ねを抑え、水平に押し返す
	Impulse.Z = 0.1f;
	
	PhysicsState.Velocity = Impulse * KnockbackForce;
}

// 空中にいるときはバイクの操作を無効にするべきなので
// 単純な重力負荷による落下の仕方を意識
void ABikeMovement::SetGravity(float DeltaTime)
{
	// -Z軸の方向へ重力加速度を計算
	PhysicsState.Velocity.Z -= GroundGravity * GravityScale * DeltaTime;
}

// バイクが落下したときにGameOverを呼び出す
void ABikeMovement::CheckDeath()
{
	// 現在のZ座標を取得
	float CurrentZ = GetActorLocation().Z;
	
	// 設定した閾値より下回った場合
	if (CurrentZ < fallDeathZ)
	{
		IsDead = true;
		
		if (OnDead.IsBound())
		{
			OnDead.Broadcast(true);
		}
	}
}

// 入力時間によるチャージと実際にブーストを加算する処理
void ABikeMovement::UpdateMiniTurbo(float DeltaTime,float CurrentSpeedMPS)
{
	CurveEngineFireEffect->Activate(true);
	// 現在カーブ中かどうかを確認する
	bool IsTurning = false;
	if (CurrentSpeedMPS > TurboMinimumSpeed && FMath::Abs(SteeringInput) >= SteeringInputMinPower)
	{
		IsTurning = true;
	}

	if (IsTurning)
	{
		CurrentInputTimer += DeltaTime;

		// 基底時間を超えたらチャージ完了
		if (CurrentInputTimer >= RequiredInputTime && !IsTurboCharged)
		{
			IsTurboCharged = true;
		}
	}
	else
	{
		// 条件を満たさなくなった時にチャージ中ならブースト発動
		if (IsTurboCharged)
		{
			CurrentBoostTimer = BoostDuration;
			IsTurboCharged = false;
			
			FMyVector3D ForwardDir = PhysicsState.Rotation.GetForwardVector().Normalize();
			// ターボ分の力を加算
			PhysicsState.Velocity += ForwardDir * BoostForce;
		}
       
		CurrentInputTimer = 0.0f;

		// カーブ中でなくなったらエフェクトを停止
		if (CurveEngineFireEffect && CurveEngineFireEffect->IsActive())
		{
			CurveEngineFireEffect->Deactivate();
		}
	}

	if (CurrentBoostTimer > 0.0f)
	{
		CurrentBoostTimer -= DeltaTime;
	}
}

// 前輪の接地判定
bool ABikeMovement::CheckFrontWheelGrounded(FHitResult& OutHitResult) const
{
    FVector BikeLocation = PhysicsState.Position.ToFVector();
	
    // バイクのローカル軸を取得
	// 車体の傾きの影響を受けない水平軸を算出
	FRotator YawOnlyRotation(0.0f, PhysicsState.Rotation.ToFQuat().Rotator().Yaw, 0.0f);
	//FVector ForwardDirection = PhysicsState.Rotation.GetForwardVector().ToFVector();
	FVector ForwardDirection = YawOnlyRotation.Vector();
	FVector RightDirection = FRotationMatrix(YawOnlyRotation).GetUnitAxis(EAxis::Y);
    //FVector RightDirection = PhysicsState.Rotation.GetRightVector().ToFVector();
	//FVector UpDirection = PhysicsState.Rotation.GetUpVector().ToFVector();

    // 調整用の個別オフセットをすべて加算
    FVector Start = BikeLocation + ForwardDirection * FrontWheelXOffset + RightDirection * FrontWheelYOffset + FVector::UpVector * SensorHeightOffset;
    FVector End = Start - FVector::UpVector * (CustomTraceDistance + SensorHeightOffset);

    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this);

	// レイキャスト
	// 1本の光線を放つ
    bool Hit = GetWorld()->
	LineTraceSingleByChannel(OutHitResult,Start,End,ECC_WorldStatic,QueryParams);
	// 地面法線との判定
    bool bIsGrounded = Hit && OutHitResult.ImpactNormal.Z > 0.1f;

    // デバッグ描画
	if (UWorld* World = GetWorld())
	{
		FColor LineColor = bIsGrounded ? FColor::Green : FColor::Red;
        
		// スタート位置からヒット位置（またはEnd）まで一直線を描く
		FVector LineEndPos = bIsGrounded ? OutHitResult.ImpactPoint : End;
		DrawDebugLine(World, Start, LineEndPos, LineColor, false, -1.0f, 0, 2.0f);

		if (bIsGrounded)
		{
			// 接地点に黄色い小さなポッチを描画
			DrawDebugSphere(World, OutHitResult.ImpactPoint, 6.0f, 12, FColor::Yellow, false, -1.0f, 0, 2.0f);
			// 法線矢印
			DrawDebugDirectionalArrow(World, OutHitResult.ImpactPoint, OutHitResult.ImpactPoint + OutHitResult.ImpactNormal * 30.0f, 8.0f, FColor::Blue, false, -1.0f, 0, 2.0f);
		}
	}

    return bIsGrounded;
}

// 後輪の接地判定
bool ABikeMovement::CheckRearWheelGrounded(FHitResult& OutHitResult) const
{
	FVector BikeLocation =PhysicsState.Position.ToFVector();
	
	FRotator YawOnlyRotation(0.0f, PhysicsState.Rotation.ToFQuat().Rotator().Yaw, 0.0f);
	FVector ForwardDirection = YawOnlyRotation.Vector();
	FVector RightDirection   = FRotationMatrix(YawOnlyRotation).GetUnitAxis(EAxis::Y);

    // 後輪用の調整オフセットを適用
    FVector Start = BikeLocation + ForwardDirection * RearWheelXOffset + RightDirection * RearWheelYOffset+ FVector::UpVector * SensorHeightOffset;
	FVector End = Start - FVector::UpVector * (CustomTraceDistance + SensorHeightOffset);

    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this);

    bool Hit = GetWorld()->LineTraceSingleByChannel(OutHitResult,Start,End,ECC_WorldStatic,QueryParams);

    bool bIsGrounded = Hit && OutHitResult.ImpactNormal.Z > 0.1f;

    return bIsGrounded;
}

// アクセル処理
void ABikeMovement::OnThrottleInput(const FInputActionValue& Value)
{
	ThrottleInput = Value.Get<float>();
}
void ABikeMovement::OnThrottleCompleted(const FInputActionValue& Value)
{
	ThrottleInput = 0.0f;
}

// カーブ
void ABikeMovement::OnSteeringInput(const FInputActionValue& Value)
{
	float RawInput = Value.Get<float>();
	const float DeltaTime = GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.016f;
	SteeringInput = FMath::FInterpTo(SteeringInput, RawInput, DeltaTime, 5.0f);
}
void ABikeMovement::OnSteeringInputCompleted(const FInputActionValue& Value)
{
	SteeringInput = 0.0f;
}

// ブレーキ処理
void ABikeMovement::OnBrakeInput(const FInputActionValue& Value)
{
	BrakeInput = Value.Get<float>();
}
void ABikeMovement::OnBrakeCompleted(const FInputActionValue& Value)
{
	BrakeInput = 0.0f;
}

// void ABikeMovement::OnConstruction(const FTransform& Transform)
// {
//     Super::OnConstruction(Transform);
//
//     if (UWorld* World = GetWorld())
//     {
//        FVector BikeLocation = GetActorLocation();
//        FVector ForwardDir   = GetActorForwardVector();
//        FVector RightDir     = GetActorRightVector();
//        FVector UpDir        = FVector::UpVector;
//
//        // 前輪光線のスタートとエンド
//        FVector FrontStart = BikeLocation + (ForwardDir * FrontWheelXOffset) + (RightDir * FrontWheelYOffset) + (UpDir * SensorHeightOffset);
//        FVector FrontEnd   = FrontStart - (UpDir * CustomTraceDistance);
//
//        // 後輪光線のスタートとエンド
//        FVector RearStart = BikeLocation + (ForwardDir * RearWheelXOffset) + (RightDir * RearWheelYOffset) + (UpDir * SensorHeightOffset);
//        FVector RearEnd   = RearStart - (UpDir * CustomTraceDistance);
//
//        // 前輪：水色の直線 / 後輪：オレンジ色の直線
//        DrawDebugLine(World, FrontStart, FrontEnd, FColor::Cyan, false, 2.0f, 0, 2.5f);
//        DrawDebugLine(World, RearStart, RearEnd, FColor::Orange, false, 2.0f, 0, 2.5f);
//     }
// }
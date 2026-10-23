#pragma once

#include "CoreMinimal.h"
#include "MyVector3D.h"
#include "InputMappingContext.h"
#include "FBikeRigidBodyDynamics.h"
#include "Camera/CameraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include  "InputAction.h"
#include "InputActionValue.h"
#include "GameFramework/Pawn.h"
#include "BikeMovement.generated.h"

// 速度をイベント駆動で更新するパラメーター
DECLARE_MULTICAST_DELEGATE_OneParam(FOnSpeedChanged, float /* NewSpeedMPS */);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnDead,bool /* IsDead */);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnRpmChanged,float /* NewRpm */);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnGearChanged,float /* NewGear */);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnGearTextChanged,int32 /*NewGearText */ );

// 前方宣言
class USpringArmComponent;
class UEnhancedInputComponent;

// Bikeの物理計算用構造体
struct FBikePhysics
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
	// ステアリング角(進行方向を変えるためのすべての装置)
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
	// 外部から通知を受け取るデリゲート
	FOnSpeedChanged OnSpeedChanged;
	FOnDead OnDead;
	FOnRpmChanged OnRpmChanged;
	FOnGearChanged OnGearChanged;
	FOnGearTextChanged OnGearTextChanged;
	// バイクを操作可能であるかを確認するフラグ
	bool IsCountdown = true;

protected:
	bool CheckRearWheelGrounded(FHitResult& OutHitResult) const;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	// メッシュの隙間に落ちてしまうことを防止
	UPROPERTY()
	TObjectPtr<UCameraComponent> CameraComponent;
	UPROPERTY()
	TObjectPtr<USpringArmComponent> SpringArmComponent;
	
	// Input設定
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
	UPROPERTY(EditAnywhere,Category="Input")
	UInputAction* IA_Gear1;
	UPROPERTY(EditAnywhere,Category="Input")
	UInputAction* IA_Gear2;
	UPROPERTY(EditAnywhere,Category="Input")
	UInputAction* IA_Gear3;
	UPROPERTY(EditAnywhere,Category="Input")
	UInputAction* IA_Gear4;
	UPROPERTY(EditAnywhere,Category="Input")
	UInputAction* IA_Gear5;
	UPROPERTY(EditAnywhere,Category="Input")
	UInputAction* IA_Gear6;
private:
	// Bikeの設定
	// 車体の質量
	UPROPERTY(EditAnywhere,Category="Bike Settings")
	float BikeMass = 230.0f;
	UPROPERTY(EditAnywhere,Category="Bike Settings")
	float MaxRPM = 14300.f;
	// リアタイヤの半径
	UPROPERTY(EditAnywhere,Category="Bike Settings")
	float RearWheelRadius = 0.321f;
	// 1速から6速の各ギア比
	UPROPERTY(EditAnywhere,Category="Bike Settings")
	TArray<float> GearRatios = {0,3.416f,2.25f,1.65f,1.35f,1.166f,1.038f};
	UPROPERTY(EditAnywhere,Category="Bike Settings")
	float InterpSpeed = 5.0f;
	// 落下時にゲームオーバーを表示したいので
	// 落下したと判定するための値
	UPROPERTY(EditAnywhere,Category="Bike Settings")
	float fallDeathZ = -1000.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bike Settings", meta = (AllowPrivateAccess = "true"))
	UNiagaraComponent* CurveEngineFireEffect;
	float IdleRPM = 1000.0f;
	
	// 接地判定
	// 車体中心から「前輪判定」をどれくらい前後に動かすか（プラスで前、マイナスで後ろ）
	UPROPERTY(EditAnywhere, Category = "Bike Settings")
	float FrontWheelXOffset = 60.0f;
	// 前輪判定をどれくらい「左右」に微調整するか（基本は0）
	UPROPERTY(EditAnywhere, Category = "Bike Settings")
	float FrontWheelYOffset = 0.0f;
	// 車体中心から「後輪判定」をどれくらい前後に動かすか（通常はマイナス値）
	UPROPERTY(EditAnywhere, Category = "Bike Settings")
	float RearWheelXOffset = -60.0f;
	// 後輪判定をどれくらい「左右」に微調整するか
	UPROPERTY(EditAnywhere, Category = "Bike Settings")
	float RearWheelYOffset = 0.0f;
	// 接地センサーの照射を開始する「高さ」（高めにして埋まり込みを防ぐ）
	UPROPERTY(EditAnywhere, Category = "Bike Settings")
	float SensorHeightOffset = 50.0f;
	// センサーが下を探す長さ（長すぎず、短すぎず、エディタで可視化して合わせる）
	UPROPERTY(EditAnywhere, Category = "Bike Settings")
	float CustomTraceDistance = 120.0f;
	// センサー球体の「太さ」（タイヤの幅や太さに合わせる）
	UPROPERTY(EditAnywhere, Category = "Bike Settings")
	float SensorRadius = 25.0f;
	// バイクが地面から浮く「基本車高」（埋まる場合はこれを大きくする。例: 50.0f〜80.0f）
	UPROPERTY(EditAnywhere, Category = "Bike Settings")
	float CustomRideHeight = 65.0f;
	UPROPERTY(EditAnywhere,Category = "Bike Settings")
	float GravityScale = 1.0f;
	
	// 空気抵抗係数
	float AirResistanceCoefficient = 0.02f;
	// 最大ブレーキ力
	float TotalBrakeForce = 0.0f;
	// ブレーキ力
	float BrakeForce = 0.0f;
	// 摩擦係数
	float friction = 0.9f;
	// 最大トルク量
	float MaxTorque = 115.0f;
	// トルクカーブの滑らかさを決定する
	float TorqueCurve = 0.8f;
	// 最大旋回角速度
	float MaxYawRate = 0.5f;
	// 最大バンク角
	float MaxLeanAngleRad = 0.87f;
	// 傾き(ロール)の追従スピード(補完スピード)
	float LeanResponseSpeed = 2.0f;
	// 一次減速比(エンジンからクラッチ)
	float PrimaryReductionRatio = 1.627f;
	// ファイナルドライブ比(スプロケット・チェーン)
	float FinalDriveRatio = 2.411f;
	// 水平方向(Yaw)の回転状態を保持するQuaternion
	FMyQuat YawRotation = FMyQuat::IdentityQuat();
	// キャンバー剛性(N/rad)
	float CamberStiffness = 800.0f;
	// 自然減速
	float RollingResistance = 12.0f;
	// 入力
	float ThrottleInput = 0.0f;
	float SteeringInput = 0.0f;
	float BrakeInput = 0.0f;
	// 接地判定
	bool IsGrounded = false;
	float TraceDistance = 100.0f;
	float GroundGravity = 981.0f;
	// 壁衝突
	// Knockback中か
	bool IsKnockBack = false;
	// knockback経過時間
	float KnockbackTimer = 0.0f;
	// 操作不能・吹き飛ばされ時間(秒)
	float KnockbackDuration = 0.8f;
	// 壁に押し返される力
	float KnockbackForce = 1500.0f;
	// ゲームオーバーの重複呼び出しを防ぐフラグ
	bool IsDead = false;
	// UI表示用の補正するスピード
	float DisplaySpeedMPS = 0.0f;
	// カーブ完了時にすこしだけブーストをかけて
	// ユーザーに達成感をもたらす
	// ステアリング入力とみなす最小閾値
	UPROPERTY(EditAnywhere,Category="Bike Settings")
	float SteeringInputMinPower = 0.1f;
	// ターボが発生するための最低Speed
	UPROPERTY(EditAnywhere,Category="Bike Settings")
	float TurboMinimumSpeed;
	// ミニターボに必要な入力時間
	UPROPERTY(EditAnywhere,Category="Bike Settings")
	float RequiredInputTime = 1.0f;
	// ブースト時の加速度
	UPROPERTY(EditAnywhere,Category="Bike Settings")
	float BoostForce = 3000.0f;
	// ブースト持続時間
	UPROPERTY(EditAnywhere,Category="Bike Settings")
	float BoostDuration = 0.8f;
	// 入力継続時間のタイマー
	float CurrentInputTimer = 0.0F;
	// ブースト持続時間
	float CurrentBoostTimer = 0.0f;
	// チャージ完了フラグ
	bool IsTurboCharged = false;
	
	// 地面の法線ベクトル
	FMyVector3D GroundNormal = FMyVector3D(0.0f, 0.0f, 0.0f);
	// 状態管理用ステート
	FBikePhysics PhysicsState;
	// 6Dof 剛体物理エンジン
	FBikeRigidBodyState RigidBodyState;
	
	void Initialize();
	// 前に力を与える計算
	FMyVector3D ForwardVector(float CurrentSpeed);
	// バイクの動きを更新する関数
	void UpdateCustomPhysics(float DeltaTime);
	void UpdateAngularVelocity(float DeltaTime);
	void OnThrottleInput(const FInputActionValue& Value);
	void OnThrottleCompleted(const FInputActionValue& value);
	void OnSteeringInput(const FInputActionValue& Value);
	void OnSteeringInputCompleted(const FInputActionValue& value);
	void OnBrakeInput(const FInputActionValue& Value);
	void OnBrakeCompleted(const FInputActionValue& Value);
	float CalculateEngineRPM(float SpeedMPS);
	float GetEngineTorqueAtRPM(float RPM) const;
	float CalculateDriveForce(float SpeedMPS);
	void UpdateGroundState(float DeltaTime);
	bool CheckFrontWheelGrounded(FHitResult& OutHitResult) const;
	// バンク角を計算して車体を傾ける
	void UpdateRotation(float DeltaTime); 
	float TotalReductionRatio();
	float CalculateCamberThrust(float LeanAngleRad) const;
	void BankAngle(float deltaTime);
	// 壁にぶつかったときの処理
	void OnWallImpact(const FMyVector3D& ImpactNormal);
	float LateralAcceleration() const;
	float CalculateSlipAngle(float SteeringAngleRad) const;
	float CalculatePacejkaLateralForce(float SlipAngle) const;
	// 重力を考慮する場合に使用する関数
	void SetGravity(float DeltaTime);
	// 落下判定
	void CheckDeath();
	void UpdateMiniTurbo(float DeltaTime, float CurrentSpeedMPS);

	// ギアチェンジ用関数
	// テンキー直接指定用関数
	void SetGearDirectly(int32 NewGear);
	void OnChangedGear1(){SetGearDirectly(1);}
	void OnChangedGear2(){SetGearDirectly(2);}
	void OnChangedGear3(){SetGearDirectly(3);}
	void OnChangedGear4(){SetGearDirectly(4);}
	void OnChangedGear5(){SetGearDirectly(5);}
	void OnChangedGear6(){SetGearDirectly(6);}
};
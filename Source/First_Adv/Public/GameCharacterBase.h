// ファイルの重複読み込みを防ぐためのお作法
#pragma once
// UEの機能(FStringやFVector,int32などをまとめた)軽量なヘッダー
#include "CoreMinimal.h"
// 親クラスの定義が書かれているヘッダー
#include "GameFramework/Character.h"
// UE独自のリフレクション(BPやUEエディタとC++を連携させる機能)のためのコードが自動生成されるファイル
// includeの一番最後に書く必要がある
#include "GameCharacterBase.generated.h"

// C++クラスをUEエディタやシステムに認識させる特別なクラスとして登録する関数
UCLASS()
class FIRST_ADV_API AGameCharacterBase : public ACharacter
{
	// クラスの最上部に必ず記述するマクロ
	// 内部でエンジン用のアセット管理や初期化コードを展開する
	GENERATED_BODY()

	// コンストラクタ
	// CameraやMeshなどのパーツの作成やParamの初期設定をここに記載
public:
	AGameCharacterBase();

	// Start関数
protected:
	virtual void BeginPlay() override;

public:	
	// Update関数
	virtual void Tick(float DeltaTime) override;
	// EnhancedInputにバインドするための関数
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};

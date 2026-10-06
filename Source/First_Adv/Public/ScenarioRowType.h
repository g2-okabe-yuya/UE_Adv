#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "ScenarioRowType.generated.h"

// Enumの宣言
// uint8なしでもビルドは可能その場合は4byte(int型)確保
UENUM(BlueprintType)
enum class EScenarioRowType : uint8
{
	None UMETA(DisplayName = "None"),
	ChoiceGroup UMETA(DisplayName = "Choice Group"),
	Dialogue UMETA(DisplayName = "Dialogue"),
	Choice UMETA(DisplayName = "Choice"),
};

// 構造体の宣言
USTRUCT(Blueprintable)
struct FIRST_ADV_API FScenarioRowType : public  FTableRowBase
{
	GENERATED_BODY()
	
	// UPROPERTY解説
	// EditAnywhere = Editorで直接編集が可能になる
	// BlueprintReadWrite = Blueprintからget set可能
	// Category = Editorで表示されるグループ
	// Serializeみたいなもの
	// シナリオに紐づいている識別ID
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Scenario")
	FName TextID;
	
	// テキストタイプ(選択なのかダイアログなのか)
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Scenario")
	EScenarioRowType Type = EScenarioRowType::None;
	
	// 話者
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Scenario")
	FText SpeakerName;
	
	// 本文
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Scenario")
	FText Dialogue;
	
	// セリフスキップなどセリフによって飛ばしたいときに使用
	// スキップ先を格納
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Scenario")
	FName NextID;
};
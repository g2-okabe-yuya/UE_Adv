#include "AdvHUD.h"
#include "Components/Button.h"

// スコープ解決演算子
// クラスの定義と実体を結びつける
void UAdvHUD::NativeConstruct()
{
	// Super = base
	Super::NativeConstruct();
	
	// Button_AdvのOnClickedにOnButtonClickedを結びつける
	Adv_Button->OnClicked.AddUniqueDynamic(this, &UAdvHUD::OnAdvButtonClicked);
}

void UAdvHUD::OnAdvButtonClicked()
{
	UE_LOG(LogTemp, Log, TEXT("AdvボタンがClickされました"));
}

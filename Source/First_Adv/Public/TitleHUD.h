#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TitleHUD.generated.h"

class UButton;
UCLASS()
class FIRST_ADV_API UTitleHUD : public UUserWidget
{
	GENERATED_BODY()
	
protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ButtonPlay;
	void NativeConstruct() override;
	
	// ButtonPlayのOnClickイベントに関連ずづけする
	UFUNCTION()
	void OnButtonPlayClicked();
	
private:
	UPROPERTY(meta = (BindWidget))
	class UButton* ButtonQuit;
	
	// ButtonのQuitのOnClickedイベントに関連づける
	UFUNCTION()
	void OnButtonQuitClicked();
};

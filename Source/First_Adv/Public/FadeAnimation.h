#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FadeAnimation.generated.h"


DECLARE_MULTICAST_DELEGATE(FOnFadeOutComplete);
UCLASS()
class FIRST_ADV_API UFadeAnimation : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	FOnFadeOutComplete OnFadeInComplete;
	UFUNCTION()
	void StartFadeAnimation();
	
private:
	UPROPERTY(EditAnywhere)
	float FadeInTime = 0.0f;
	
	UPROPERTY(Transient, meta = (BindWidgetAnim))
	TObjectPtr<UWidgetAnimation> FadeInAnimation;
	
	UFUNCTION()
	void OnFadeInFinished();
};

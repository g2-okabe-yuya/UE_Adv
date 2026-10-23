#include "FadeAnimation.h"

void UFadeAnimation::NativeConstruct()
{
	if (FadeInAnimation)
	{
		FWidgetAnimationDynamicEvent EndEvent;
		EndEvent.BindDynamic(this,&UFadeAnimation::OnFadeInFinished);
		BindToAnimationFinished(FadeInAnimation, EndEvent);
	}
}

// Fadeを再生する
void UFadeAnimation::StartFadeAnimation()
{
	if (FadeInAnimation)
	{
		PlayAnimation(FadeInAnimation,0.0f,1);
	}
}

// アニメーション再生完了時にUEから自動的に呼ばれる関数
void UFadeAnimation::OnFadeInFinished()
{
	StopAnimation(FadeInAnimation);
	// ここでデリゲートを発火させて外部（GameManager等）に知らせる
	if (OnFadeInComplete.IsBound())
	{
		OnFadeInComplete.Broadcast();
	}
}
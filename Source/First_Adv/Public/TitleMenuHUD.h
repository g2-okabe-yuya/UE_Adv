#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "TitleMenuHUD.generated.h"


UCLASS()
class FIRST_ADV_API ATitleMenuHUD : public AHUD
{
	GENERATED_BODY()
	
protected:
	virtual void BeginPlay() override;
};

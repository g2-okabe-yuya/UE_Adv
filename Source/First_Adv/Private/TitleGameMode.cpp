#include "TitleGameMode.h"
#include "TitleMenuHUD.h"

ATitleGameMode::ATitleGameMode()
{
	HUDClass = ATitleMenuHUD::StaticClass();
}

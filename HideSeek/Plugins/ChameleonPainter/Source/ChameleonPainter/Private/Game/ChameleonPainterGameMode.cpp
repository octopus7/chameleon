#include "Game/ChameleonPainterGameMode.h"

#include "Character/ChameleonHiderCharacter.h"

AChameleonPainterGameMode::AChameleonPainterGameMode()
{
	DefaultPawnClass = AChameleonHiderCharacter::StaticClass();
}

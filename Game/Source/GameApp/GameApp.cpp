#include "GameApp.h"
#include <GameLogic/Logic.h>

std::unique_ptr<GameLogic> GameApp::CreateGameLogic()
{
	return std::make_unique<Logic>();
}

#pragma once

#include <UIApplication.h>

class GameApp final : public UIApplication
{
public:
	GameApp()						   = default;
	GameApp(const GameApp&)			   = delete;
	GameApp& operator=(const GameApp&) = delete;

	// Inherited via UIApplication
	std::unique_ptr<GameLogic> CreateGameLogic() override;
};


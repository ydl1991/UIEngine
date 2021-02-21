#pragma once
#include "StateMachine/BaseState.h"

class Logic;

class GameMenuState final : public BaseState
{
public:
	explicit GameMenuState(Logic* pGame);
	// Do not apply copy
	GameMenuState(const GameMenuState&)				= delete;
	GameMenuState& operator=(const GameMenuState&)  = delete;

	// Inherited via BaseState
	void EnterState() override;
	void Update(float deltaTime) override;
	void ExitState() override;
	[[nodiscard]] const char* GetStateName() const override { return "Game Menu State"; }

private:
	void RegisterGameplayCallbacks();
	void CreateFloatingTitle();
};


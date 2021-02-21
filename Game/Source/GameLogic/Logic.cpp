#include "Logic.h"
#include "StateMachine/GameState/GameMenuState.h"
#include "StateMachine/GameState/GameRunningState.h"

bool Logic::Init()
{
	if (bool result = GameLogic::Init(); !result)
	{
		return result;
	}

	m_pStateMachine = std::make_unique<StateMachine>();
	if (!m_pStateMachine)
		return false;
	
	return true;
}

bool Logic::PostInit()
{
	if (bool result = GameLogic::PostInit(); !result)
	{
		return result;
	}

	m_pStateMachine->SetCurrentState(std::move(std::make_unique<GameMenuState>(this)));
	
	return true;
}

void Logic::Update(float deltaSeconds)
{
	m_pStateMachine->Update(deltaSeconds);
	GameLogic::Update(deltaSeconds);
}

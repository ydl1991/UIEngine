#include "StateMachine.h"

void StateMachine::Update(float deltaTime)
{
	if (m_pCurrentState)
	{
		m_pCurrentState->Update(deltaTime);
	}
}

void StateMachine::SetCurrentState(std::unique_ptr<BaseState> newState)
{
	if (m_pCurrentState)
	{
		m_pCurrentState->ExitState();
	}

	m_pCurrentState = std::move(newState);

	if (m_pCurrentState)
	{
		m_pCurrentState->EnterState();
	}
}

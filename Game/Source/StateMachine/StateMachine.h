#pragma once

#include <memory>
#include "BaseState.h"

/***********************************************************/
// State Machine that processes game states
/***********************************************************/
class StateMachine
{
public:
	StateMachine() = default;
	~StateMachine() = default;

	void Update(float deltaTime);

	void SetCurrentState(std::unique_ptr<BaseState> newState);
	[[nodiscard]] BaseState* GetCurrentState() const { return m_pCurrentState.get(); }

private:
	std::unique_ptr<BaseState> m_pCurrentState;
};

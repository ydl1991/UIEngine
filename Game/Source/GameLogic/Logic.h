#pragma once
#include <GameLogic.h>
#include "StateMachine/StateMachine.h"

class Logic final : public GameLogic
{
public:
	Logic()							= default;
	Logic(const Logic&)				= delete;
	Logic& operator=(const Logic&)  = delete;

	bool Init() override;
	bool PostInit() override;
	void Update(float deltaSeconds) override;
	StateMachine* GetStateMachine() { return m_pStateMachine.get(); }
	
private:
	std::unique_ptr<StateMachine> m_pStateMachine;				// a system that switches different game states
};


#pragma once

class Logic;

class BaseState
{
public:
	explicit BaseState(Logic* game) : m_pGame(game) {}
	virtual ~BaseState() = default;

	// pure virtual
	virtual void EnterState() = 0;
	virtual void Update(float deltaTime) = 0;
	virtual void ExitState() = 0;
	[[nodiscard]] virtual const char* GetStateName() const = 0;

protected:
	Logic* m_pGame;
};


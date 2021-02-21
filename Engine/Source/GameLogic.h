#pragma once
#include <Process/UIProcessController.h>

class GameLogic
{
public:
	GameLogic();
	virtual ~GameLogic()					= default;
	GameLogic(const GameLogic&)				= delete;
	GameLogic& operator=(const GameLogic&)  = delete;

	// virtual
	virtual bool Init();
	virtual bool PostInit();
	virtual void Update(float deltaSeconds);

	UIProcessController m_processController;
	bool m_endGame;
};


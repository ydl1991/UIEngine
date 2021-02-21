#include "GameLogic.h"

GameLogic::GameLogic()
	: m_endGame(false)
{
}

bool GameLogic::Init()
{
	return true;
}

bool GameLogic::PostInit()
{
	return true;
}

void GameLogic::Update(float deltaSeconds)
{
	// Update Processes
	m_processController.UpdateProcesses(deltaSeconds);
}

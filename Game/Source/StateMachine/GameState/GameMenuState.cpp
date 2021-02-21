#include "GameMenuState.h"
#include "GameLogic/Logic.h"
#include "StateMachine/StateMachine.h"
#include "GameRunningState.h"

#include <UIApplication.h>
#include <UI/UIManager.h>
#include <UI/UIElement/Button/UIButton.h>
#include <UI/UIElement/Textfield/UITextfield.h>
#include <Process/UIFloatingProcess.h>

GameMenuState::GameMenuState(Logic* pGame)
	: BaseState(pGame)
{
}

void GameMenuState::EnterState()
{
	UIApplication::Get()->LoadXML("Assets/XMLs/Final/Final-Game-Menu.xml");
	
	CreateFloatingTitle();
	RegisterGameplayCallbacks();
}

void GameMenuState::Update(float deltaTime)
{

}

void GameMenuState::ExitState()
{
	UIApplication::Get()->Reset();
}

void GameMenuState::RegisterGameplayCallbacks()
{
	UIManager& uiManager = UIApplication::Get()->GetUIManager();

	// Set Exit Button Callback
	if (UIButton* pExitButton = dynamic_cast<UIButton*>(uiManager.GetElementWithTag("ExitButton")))
	{
		pExitButton->AddClickCallback([this](int)
		{
			m_pGame->m_endGame = true;
		});
	}

	// Set Start Button Callback
	if (UIButton* pStartButton = dynamic_cast<UIButton*>(uiManager.GetElementWithTag("StartButton")))
	{
		pStartButton->AddClickCallback([this](int)
		{
			m_pGame->GetStateMachine()->SetCurrentState(std::move(std::make_unique<GameRunningState>(m_pGame)));
		});
	}
}

void GameMenuState::CreateFloatingTitle()
{
	UIManager& uiManager = UIApplication::Get()->GetUIManager();
	auto& sharedElements = uiManager.GetSharedElements();

	if (UITextfield* pTextfield = dynamic_cast<UITextfield*>(uiManager.GetElementWithTag("Title")))
	{
		auto pFloatingProcess = std::make_shared<UIFloatingProcess>(sharedElements[pTextfield->m_id], 20.f, 80.f);
		UIApplication::Get()->GetGame()->m_processController.AttachProcess(std::move(pFloatingProcess));
	}
}

#include "GameRunningState.h"
#include "GameLogic/Logic.h"
#include "StateMachine/StateMachine.h"
#include "GameMenuState.h"

#include <UIApplication.h>
#include <UI/UIManager.h>
#include <UI/UIElement/Button/UIButton.h>
#include <UI/UIElement/Input/UIInput.h>
#include <UI/UIElement/Repeat/UIRepeat.h>
#include <UI/UIProperty/UIProperty.h>
#include <Utils/XOrShift.h>
#include <Process/UIDelayProcess.h>
#include <Process/UIProcessController.h>

#include <algorithm>

static constexpr const char* s_kTrialVariableName = "trial";
static constexpr const char* s_kPairLeftVariableName = "pairLeft";
static constexpr const char* s_kPairInputVariableName = "numPair";
static constexpr int s_kMaxNumOfPair = 15;

static constexpr const char* s_kFilenamePrefix = "Assets/XMLs/Final/Match-Image-";
static constexpr const char* s_kFilenamePostfix = ".xml";
static constexpr int s_kPairOfImages = 13;

static constexpr const char* s_kMatchImageTagPrefix = "match";

GameRunningState::GameRunningState(Logic* pGame)
	: BaseState(pGame)
{

}

void GameRunningState::EnterState()
{
	m_parentIdToMatchImageIds.reserve(100);

	UIApplication::Get()->LoadXML("Assets/XMLs/Final/Final-Game-UI.xml");
	RegisterGameplayCallbacks();
	
	UIManager& uiManager = UIApplication::Get()->GetUIManager();
	AssignMatchGameImages(uiManager, uiManager.GetElementWithTag("GamePanel")->m_id);
}

void GameRunningState::Update(float)
{

}

void GameRunningState::ExitState()
{
	UIApplication::Get()->Reset();
}

void GameRunningState::RegisterGameplayCallbacks()
{
	UIManager& uiManager = UIApplication::Get()->GetUIManager();

	// Set Exit Button Callback
	if (UIButton* pExitButton = dynamic_cast<UIButton*>(uiManager.GetElementWithTag("ExitButton")))
	{
		pExitButton->AddClickCallback([this](int)
		{
			m_pGame->GetStateMachine()->SetCurrentState(std::move(std::make_unique<GameMenuState>(m_pGame)));
		});
	}

	// Set Reset Button Callback
	UIButton* pResetButton = dynamic_cast<UIButton*>(uiManager.GetElementWithTag("ResetButton"));
	UIRepeat* pRepeat = dynamic_cast<UIRepeat*>(uiManager.GetElementWithTag("GamePanel"));
	if (pResetButton && pRepeat)
	{
		// Create data variable 'numPair' and bind to repeat element count
		UIProperty& numPairProperty = uiManager.GetOrCreateNewUIProperty(s_kPairInputVariableName);
		numPairProperty = 0;
		pRepeat->BindRepeatCount(numPairProperty);

		pResetButton->AddClickCallback([this](int)
		{
			UIManager& manager = UIApplication::Get()->GetUIManager();
			if (UIInput* pInput = dynamic_cast<UIInput*>(manager.GetElementWithTag("PairInput")))
			{
				int pairCount = std::stoi(pInput->Text().ToString());
				auto* pMessage = manager.GetData("message");
				if (pairCount <= 0 || pairCount > s_kMaxNumOfPair)
				{
					if (pMessage)
						*pMessage = "Number of input pairs doesn't match game pairs range [0 - 15).";

					return;
				}

				manager.GetOrCreateNewUIProperty(s_kTrialVariableName) = 0;
				manager.GetOrCreateNewUIProperty(s_kPairLeftVariableName) = pairCount;
				manager.GetOrCreateNewUIProperty(s_kPairInputVariableName) = pairCount * 2;
				AssignMatchGameImages(manager, manager.GetElementWithTag("GamePanel")->m_id);

				if (pMessage)
					*pMessage = "Successfully reset game to " + std::to_string(pairCount) + " pairs.";
			}
		});
	}

	// Set Matching Images

}

// ------------------------------------------------------- //
//	Assign random image pairs to each button in the
//	main game panel
// ------------------------------------------------------- //
void GameRunningState::AssignMatchGameImages(UIManager& manager, int rootId)
{
	// Get ids of all cards(buttons) in game panel
	auto& children = manager.GetChildrenIds(rootId);
	size_t childCount = children.size();
	std::vector<int> matchIds = {};

	// XorShift random number generator
	auto* pRNG = UIApplication::Get()->GetRNG();

	for (size_t index = 0; index < childCount; ++index)
	{
		int childId = children[index];
		int matchId = 0;

		if (index < childCount / 2)
		{
			// randomly select a image, make a pair
			matchId = pRNG->RandomIntRange(0, s_kPairOfImages);
			matchIds.emplace_back(matchId);

			// generate the image element's XML address
			char fileName[100] = {};
			strcat_s(fileName, s_kFilenamePrefix);
			strcat_s(fileName, std::to_string(matchId).c_str());
			strcat_s(fileName, s_kFilenamePostfix);

			// load the image from XML and add it to the card 
			manager.LoadXml(fileName, childId);
		}
		else
		{
			// randomly select the left-over image from Id queue
			int matchIdIndex = pRNG->RandomIntRange(0, static_cast<int>(matchIds.size()));
			matchId = matchIds[matchIdIndex];
			std::swap(matchIds[matchIdIndex], matchIds.back());
			matchIds.pop_back();

			// generate the image element's XML address
			char fileName[100] = {};
			strcat_s(fileName, s_kFilenamePrefix);
			strcat_s(fileName, std::to_string(matchId).c_str());
			strcat_s(fileName, s_kFilenamePostfix);

			// load the image from XML and add it to the card 
			manager.LoadXml(fileName, childId);
		}

		AssignFlipImageCallback(manager, childId, matchId);
	}

	UIApplication::Get()->SignalRearrange();
}

void GameRunningState::AssignFlipImageCallback(UIManager& manager, int parentId, int matchId)
{
	// compose the tag name
	std::string matchImageTag = s_kMatchImageTagPrefix;
	matchImageTag += std::to_string(matchId);

	// get the newly added image element with the tag, as well as it's parent card element
	UIElement* pMatchImageElement = manager.GetChildWithTag(parentId, matchImageTag);
	auto* pButton = dynamic_cast<UIButton*>(manager.GetElementAt(parentId));
	
	if (pMatchImageElement && pButton)
	{
		// record data into hash map with < Key(id): parent, Value(pair): image element id, match id (from 0 to 12)
		m_parentIdToMatchImageIds.emplace(parentId, std::make_pair(pMatchImageElement->m_id, matchId));

		pButton->AddClickCallback([this](int id)
		{
			// if 2 other cards are already flipped, or the card has currently been flipped, exit
			if (m_currentSelected.size() >= 2 ||
				std::find(m_currentSelected.begin(), m_currentSelected.end(), id) != m_currentSelected.end())
				return;

			UIManager& uiManager = UIApplication::Get()->GetUIManager();
			auto& sharedElements = uiManager.GetSharedElements();

			// Reveal Image
			auto [imageId, matchId] = m_parentIdToMatchImageIds.at(id);
			sharedElements[imageId]->m_visible = true;
			m_currentSelected.emplace_back(id);

			// if current number of flipping cards is 2, Create process to check if 2 cards match
			if (m_currentSelected.size() >= 2)
			{
				// increment number of trial
				UIProperty& trialNum = uiManager.GetOrCreateNewUIProperty(s_kTrialVariableName);
				const int* currentTrial = trialNum.Get<int>();
				trialNum = *currentTrial + 1;

				if (CheckForPair())
				{
					UIProperty& message = uiManager.GetOrCreateNewUIProperty("message");
					UIProperty& pairLeft = uiManager.GetOrCreateNewUIProperty(s_kPairLeftVariableName);
					const int* left = pairLeft.Get<int>();
					pairLeft = *left - 1;

					if (*left == 0)
						message = "Congratulations! You found all the pairs and become the winner.";
					else
						message = "You found a pair! Great job and keep it up.";
				}
			}
		});
	}
}

bool GameRunningState::CheckForPair()
{
	auto& uiManager = UIApplication::Get()->GetUIManager();
	auto& sharedElements = uiManager.GetSharedElements();
	auto& processController = UIApplication::Get()->GetGame()->m_processController;

	// Get data of both images
	int parent1Id = m_currentSelected[0];
	int parent2Id = m_currentSelected[1];
	auto [image1Id, matchId1] = m_parentIdToMatchImageIds.at(parent1Id);
	auto [image2Id, matchId2] = m_parentIdToMatchImageIds.at(parent2Id);

	// if image match, delete both images
	if (matchId1 == matchId2)
	{
		// create delay destroy process
		auto pDelayProcess1 = std::make_shared<UIDelayProcess>(sharedElements[parent1Id], 1.f);
		pDelayProcess1->SetCallback(UIProcess::State::kSucceeded, [this](const std::weak_ptr<UIElement>& pOwner)
		{
			if (!pOwner.expired())
			{
				UIApplication::Get()->GetUIManager().ClearSubtree(pOwner.lock()->m_id);
				m_currentSelected.clear();
			}
		});

		// create delay destroy process
		auto pDelayProcess2 = std::make_shared<UIDelayProcess>(sharedElements[parent2Id], 1.f);
		pDelayProcess2->SetCallback(UIProcess::State::kSucceeded, [this](const std::weak_ptr<UIElement>& pOwner)
		{
			if (!pOwner.expired())
			{
				UIApplication::Get()->GetUIManager().ClearSubtree(pOwner.lock()->m_id);
				m_currentSelected.clear();
			}
		});

		processController.AttachProcess(std::move(pDelayProcess1));
		processController.AttachProcess(std::move(pDelayProcess2));

		return true;
	}

	// if images do not match, flip them back
	// create delay flip back process
	auto pDelayProcess1 = std::make_shared<UIDelayProcess>(sharedElements[image1Id], 1.f);
	pDelayProcess1->SetCallback(UIProcess::State::kSucceeded, [this](const std::weak_ptr<UIElement>& pOwner)
	{
		if (!pOwner.expired())
		{
			pOwner.lock()->m_visible = false;
			m_currentSelected.clear();
		}
	});

	// create delay flip back process
	auto pDelayProcess2 = std::make_shared<UIDelayProcess>(sharedElements[image2Id], 1.f);
	pDelayProcess2->SetCallback(UIProcess::State::kSucceeded, [this](const std::weak_ptr<UIElement>& pOwner)
	{
		if (!pOwner.expired())
		{
			pOwner.lock()->m_visible = false;
			m_currentSelected.clear();
		}
	});

	processController.AttachProcess(std::move(pDelayProcess1));
	processController.AttachProcess(std::move(pDelayProcess2));

	return false;
}
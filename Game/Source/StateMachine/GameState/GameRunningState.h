#pragma once
#include <StateMachine/BaseState.h>
#include <UI/UIManager.h>

#include <unordered_map>
#include <vector>

class Logic;

class GameRunningState final : public BaseState
{
public:
	explicit GameRunningState(Logic* pGame);
	// Do not apply copy
	GameRunningState(const GameRunningState&)			 = delete;
	GameRunningState& operator=(const GameRunningState&) = delete;

	// Inherited via BaseState
	void EnterState() override;
	void Update(float deltaTime) override;
	void ExitState() override;
	[[nodiscard]] const char* GetStateName() const override { return "Game Running State"; }
	
private:
	void RegisterGameplayCallbacks();
	void AssignMatchGameImages(UIManager& manager, int rootId);
	void AssignFlipImageCallback(UIManager& manager, int parentId, int matchId);
	bool CheckForPair();

private:
	static constexpr size_t s_kNumOfCardsToCheck = 2;
	std::vector<int> m_currentSelected;

	// Key: Parent Element Id, Value: <Image Element Id, Image Match Id>
	std::unordered_map<int, std::pair<int, int>> m_parentIdToMatchImageIds;
};


#pragma once
#include "../UIElement.h"
#include "UI/UIProperty/UIProperty.h"
#include <string>
#include <vector>
#include <unordered_set>
#include <functional>

enum class ButtonState
{
	kUp = 0,
	kOver,
	kDown,
	kUndefined,
	kTotal,
};

class UIProperty;

class UIButton final : public UIElement
{
public:
	// Alias
	using Callback = std::function<void(int)>;

	UIButton();

	int CheckMouseClick() override;
	void ReorderChild(int id);
	void RemoveChild(int id);
	[[nodiscard]] std::unordered_set<int> ParseChildState(const std::string& stateString) const;
	void AddClickCallback(Callback func);
	void TriggerClickCallbacks();

public:
	std::string m_onVariableName;
	UIProperty m_onValue;

	ButtonState m_state;
	std::vector<int> m_stateChildIndices[(int)ButtonState::kTotal];

private:
	std::vector<Callback> m_clickCallbacks;
};


#include "UIButton.h"
#include "Mouse/Mouse.h"
#include "UIApplication.h"
#include "UI/UIManager.h"

#include <algorithm>

UIButton::UIButton()
	: m_onVariableName("")
	, m_state(ButtonState::kUp)
{
	AddClickCallback([this](int) {
		if (m_onVariableName.empty())
			return;

		if (auto* pProp = UIApplication::Get()->GetUIManager().GetData(m_onVariableName))
			*pProp = m_onValue;
	});

	m_elementType = ElementType::kButton;
}

int UIButton::CheckMouseClick()
{
	Mouse& mouse = Mouse::Get();
	if (mouse.IsButtonDown(Mouse::MouseButton::kButtonLeft))
	{		
		m_state = ButtonState::kDown;
		return (int)Mouse::MouseButton::kButtonLeft;
	}

	if (mouse.IsButtonReleased(Mouse::MouseButton::kButtonLeft))
	{
		UIApplication::Get()->GetUIManager().RegisterClickElement(m_id);
	}
	
	m_state = ButtonState::kOver;
	return -1;
}

void UIButton::ReorderChild(int id)
{
	auto& uiManager = UIApplication::Get()->GetUIManager();
	auto* pChild = uiManager.GetElementAt(id);
	auto pStates = ParseChildState(pChild->m_buttonState.ToString());

	for (int i = 0; i < (int)ButtonState::kTotal; ++i)
	{
		auto it = std::find(m_stateChildIndices[i].begin(), m_stateChildIndices[i].end(), id);

		if (pStates.find(i) != pStates.end() && it == m_stateChildIndices[i].end())
		{
			m_stateChildIndices[i].emplace_back(id);
		}
		else if (pStates.find(i) == pStates.end() && it != m_stateChildIndices[i].end())
		{
			std::swap(*it, m_stateChildIndices[i].back());
			m_stateChildIndices[i].pop_back();
		}
	}
}

void UIButton::RemoveChild(int id)
{
	for (auto& vec : m_stateChildIndices)
	{
		auto it = std::find(vec.begin(), vec.end(), id);
		if (it != vec.end())
			vec.erase(it);
	}
}

std::unordered_set<int> UIButton::ParseChildState(const std::string& stateString) const
{
	std::unordered_set<int> result = {};

	size_t start = 0;

	while (start != std::string::npos)
	{
		size_t end = stateString.find_first_of(',', start);
		std::string substr = stateString.substr(start, end - start);

		if (substr == "up")
			result.emplace((int)ButtonState::kUp);
		else if (substr == "down")
			result.emplace((int)ButtonState::kDown);
		else if (substr == "over")
			result.emplace((int)ButtonState::kOver);
		else
			result.emplace((int)ButtonState::kUndefined);

		start = end == std::string::npos ? end : end + 1;
	}

	// if use std::move will prevent compiler copy elision
	return result;
}

void UIButton::AddClickCallback(Callback func)
{
	m_clickCallbacks.emplace_back(std::move(func));
}

void UIButton::TriggerClickCallbacks()
{
	std::for_each(m_clickCallbacks.begin(), m_clickCallbacks.end(), [this](Callback& func) { func(m_id); });
}

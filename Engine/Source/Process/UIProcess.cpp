#include "UIProcess.h"
#include "UI/UIElement/UIElement.h"

UIProcess::UIProcess()
	: m_state{ }
{
}

UIProcess::UIProcess(std::shared_ptr<UIElement>& pOwner)
	: m_state{ kUninitialized }
	, m_pOwner{ pOwner }
{
}

void UIProcess::Update(float deltaSeconds)
{
	if (m_pOwner.expired())
		Abort();
}

std::shared_ptr<UIProcess> UIProcess::RemoveChild()
{
	auto pChild = m_pChild;
	m_pChild = nullptr;
	return pChild;
}

void UIProcess::CheckForAbort(int ownerId)
{
	if (m_pOwner.expired() || m_pOwner.lock()->m_id == ownerId)
		Abort();
}

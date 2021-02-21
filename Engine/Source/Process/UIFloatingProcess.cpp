#include "UIFloatingProcess.h"
#include "UI/UIElement/UIElement.h"

#include <SDL/SDL.h>

UIFloatingProcess::UIFloatingProcess(std::shared_ptr<UIElement>& pOwner, float offset, float floatingSpeed)
	: UIProcess{ pOwner }
	, m_speed { floatingSpeed }
	, m_floatingOffset{ offset }
	, m_currentOffset{ 0 }
	, m_dir{ 1 }
{
}

void UIFloatingProcess::Update(float deltaSeconds)
{
	UIProcess::Update(deltaSeconds);

	if (IsAlive())
	{
		if (m_currentOffset < -m_floatingOffset)
		{
			m_dir = 1;
		}
		else if(m_currentOffset > m_floatingOffset)
		{
			m_dir = -1;
		}

		m_currentOffset += deltaSeconds * m_speed * m_dir;
		m_pOwner.lock()->m_renderRect.y += static_cast<int>(m_currentOffset);
	}
}

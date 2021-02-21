#include "UIDelayProcess.h"

UIDelayProcess::UIDelayProcess(std::shared_ptr<UIElement>& pOwner, float time)
	: UIProcess{ pOwner }
	, m_duration{ time }
	, m_timer{ 0.0f }
{
}

bool UIDelayProcess::Init()
{
	m_timer = m_duration;
	return true;
}

void UIDelayProcess::Update(float deltaSeconds)
{
	UIProcess::Update(deltaSeconds);

	if (IsAlive())
	{
		m_timer -= deltaSeconds;

		if (m_timer < 0.f)
			Succeed();
	}
}

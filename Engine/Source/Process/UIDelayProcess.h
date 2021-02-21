#pragma once
#include "UIProcess.h"

class UIDelayProcess final : public UIProcess
{
public:
	explicit UIDelayProcess(std::shared_ptr<UIElement>& pOwner, float time);

	bool Init() override;
	void Update(float deltaSeconds) override;

private:
	float m_duration;
	float m_timer;
};


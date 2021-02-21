#pragma once
#include "UIProcess.h"

class UIFloatingProcess final : public UIProcess
{
public:
	explicit UIFloatingProcess(std::shared_ptr<UIElement>& pOwner, float offset, float floatingSpeed);
	
	void Update(float deltaSeconds) override;

private:
	float m_speed;
	float m_floatingOffset;
	float m_currentOffset;
	float m_dir;
};


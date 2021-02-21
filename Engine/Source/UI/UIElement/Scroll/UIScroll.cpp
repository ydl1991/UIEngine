#include "UIScroll.h"
#include "Mouse/Mouse.h"

UIScroll::UIScroll()
	: m_scrollOffset(0)
	, m_maxScroll(0)
{
	m_elementType = ElementType::kScroll;
	m_childLayout = LayoutType::kScrollVertical;
}

bool UIScroll::OffsetCheck()
{
	int currentScroll = Mouse::Get().GetWheelValue();
	if (m_maxScroll <= 0 || m_scrollOffset + currentScroll < -m_maxScroll || m_scrollOffset + currentScroll > 0)
		return false;

	m_scrollOffset += currentScroll;
	return true;
}

#pragma once
#include "UI/UIElement/UIElement.h"

class UIScroll final : public UIElement
{
public:
	UIScroll();

	bool OffsetCheck();

	int m_scrollOffset;
	int m_maxScroll;
};


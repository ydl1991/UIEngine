#pragma once
#include "UI/UIElement/UIElement.h"
#include <SDL/SDL.h>

class UIRectShape final : public UIElement
{
public:
	UIRectShape();

	void Render(SDL_Renderer* pRenderer, float parentW, float parentH) override;

	bool m_filled;
	UIProperty m_r;
	UIProperty m_g;
	UIProperty m_b;
};


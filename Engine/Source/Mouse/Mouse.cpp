#include "Mouse.h"
#include <cstring>
#include <SDL/SDL.h>

Mouse::Mouse()
	: m_buttonState{}
	, m_prevButtonState{}
	, m_wheelValue(0)
	, m_mouseX(0)
	, m_mouseY(0)
{
}

void Mouse::SetButtonState(MouseButton button, bool down)
{
	if (button != MouseButton::kButtonTotal)
	{
		m_buttonState[(int)button] = down;
	}
}

Mouse& Mouse::Get()
{
	static Mouse instance;
	return instance;
}

void Mouse::Reset()
{
	std::memcpy(m_prevButtonState, m_buttonState, sizeof m_buttonState);

	m_wheelValue = 0;
	
	// Collect the mouse state
	// Represent it as an event for the UI tree
	// Pass it down the tree
	SDL_GetMouseState(&m_mouseX, &m_mouseY);
}
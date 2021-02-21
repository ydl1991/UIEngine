#include <cstring>
#include "Keyboard.h"

Keyboard::Keyboard()
	: m_lastKeyboardState{}
	, m_keyboardState{}
{
}

Keyboard& Keyboard::Get()
{
	static Keyboard instance;
	return instance;
}

bool Keyboard::IsKeyDown(KeyCode keyCode) const
{
	return m_keyboardState[keyCode];
}

bool Keyboard::WasKeyPressed(KeyCode keyCode) const
{
	return m_lastKeyboardState[keyCode] && !m_keyboardState[keyCode];
}

void Keyboard::SetKeyDown(KeyCode keyCode, bool isDown)
{
	m_keyboardState[keyCode] = isDown;
}

void Keyboard::Reset()
{
	std::memcpy(m_lastKeyboardState, m_keyboardState, sizeof m_keyboardState);
	std::memset(m_keyboardState, 0, sizeof m_keyboardState);
}

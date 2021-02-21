#pragma once
class Mouse
{
public:

	// \enum Button
	enum class MouseButton
	{
		// Key Buttons
		kButtonLeft = 0,
		kButtonRight,

		// Total
		kButtonTotal,
	};

	/// Destructor
	~Mouse()						= default;
	/// Do not apply copy
	Mouse(const Mouse&)				= delete;
	Mouse& operator=(const Mouse&)  = delete;

	/// Get keyboard instance
	static Mouse& Get();

	// Mouse Button
	void SetButtonState(MouseButton button, bool down);
	[[nodiscard]] bool IsButtonDown(MouseButton button) const { return m_buttonState[(int)button]; }
	[[nodiscard]] bool IsButtonPressed(MouseButton button) const { return m_buttonState[(int)button] && m_prevButtonState[(int)button]; }
	[[nodiscard]] bool IsButtonReleased(MouseButton button) const { return !m_buttonState[(int)button] && m_prevButtonState[(int)button]; }
	// Mouse Wheel
	void SetWheelValue(int val) { m_wheelValue = val; }
	[[nodiscard]] int GetWheelValue() const { return m_wheelValue; }
	[[nodiscard]] int GetMouseX() const { return m_mouseX; }
	[[nodiscard]] int GetMouseY() const { return m_mouseY; }

	void Reset();

private:
	Mouse();

	bool m_buttonState[(int)MouseButton::kButtonTotal];
	bool m_prevButtonState[(int)MouseButton::kButtonTotal];
	// Mouse Wheel
	int m_wheelValue;
	// Mouse Position
	int m_mouseX, m_mouseY;
};


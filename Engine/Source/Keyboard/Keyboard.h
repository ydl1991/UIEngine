#pragma once
#include <cinttypes>


class Keyboard
{
	static constexpr size_t s_kKeyboardLen = 255;
	bool m_lastKeyboardState[s_kKeyboardLen];
	bool m_keyboardState[s_kKeyboardLen];

public:
	/// Destructor
	~Keyboard()							 = default;
	/// Do not apply copy
	Keyboard(const Keyboard&)			 = delete;
	Keyboard& operator=(const Keyboard&) = delete;

	/// Get keyboard instance
	static Keyboard& Get();

	/// The allowed values for KeyCode align with Windows' Virtual Key defines.
	/// See https://docs.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
	/// Despite being an int, the maximum allowed value is 255
	using KeyCode = uint32_t;

	/// Returns whether the key represented by the given code is currently depressed.
	[[nodiscard]] bool IsKeyDown(KeyCode keyCode) const;

	/// Returns whether the key represented by the given code was pressed and then released.
	/// This implies there is a one-frame lag
	[[nodiscard]] bool WasKeyPressed(KeyCode keyCode) const;

	/// Marks whether the key represented by the given code is currently depressed.
	void SetKeyDown(KeyCode keyCode, bool isDown);

	/// Sets the state of all keys back to unpressed and mark keys that were previously depressed.
	/// This is to be called at the end of a frame.
	void Reset();

private:
	///  Constructor
	Keyboard();
};

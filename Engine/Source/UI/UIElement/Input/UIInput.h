#pragma once
#include "UI/UIElement/UIElement.h"
#include "UI/UIProperty/UIProperty.h"

#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>
#include <string>
#include <vector>

enum class FocusState
{
	kFocused = 0,
	kNotFocused,
	kTotal,
};

struct InputData
{
	std::string m_placeHolder;
	std::string m_inputType;
	std::string m_font;
	int m_fontSize;
	int m_max;

	InputData()
		: m_placeHolder("")
		, m_inputType("")
		, m_font("Assets/Fonts/Grandstander-Medium.ttf")
		, m_fontSize(20)
		, m_max(0)
	{

	}
};

class UIInput final : public UIElement
{
public:
	using UniqueSurface = std::unique_ptr<SDL_Surface, decltype(&SDL_FreeSurface)>;

	explicit UIInput(const InputData& data);
	~UIInput() override;

	void UpdateFocus() override;
	void Render(SDL_Renderer* pRenderer, float parentW, float parentH) override;
	void NotifyText(const char* pText) override;
	void GainFocus() override;
	void LoseFocus() override;
	void CheckKeyPress() override;
	void RemoveChild(int id);

	UIProperty& Text() { return m_text; }

private:
	void CheckUpdateSurface();
	void AdjustCursorPosition(int direction);
	int GetCursorPixelOffset();
	void ParseInputType(const std::string& inputType);
	
	bool WriteInput(const std::string& text);
	bool WriteString(const std::string& text);
	bool WriteInt(const std::string& text);
	bool WriteBool(const std::string& text);
	bool WriteFloat(const std::string& text);

public:
	FocusState m_state;
	std::vector<int> m_stateChildIndices[(int)FocusState::kTotal];

private:
	std::string m_placeHolder;
	UIProperty m_text;
	int m_max;

	SDL_Color m_color;
	SDL_Color m_defaultTextColor;
	TTF_Font* m_pFont;
	int m_fontSize;

	mutable UniqueTexture m_pTextTexture;
	mutable UniqueSurface m_pTextSurface;

	SDL_Rect m_cursorRect;
	int m_cursorPosition;
	int m_cursorHeight;
	int m_scrollOffset;
};


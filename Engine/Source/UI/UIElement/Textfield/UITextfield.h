#pragma once
#include "UI/UIElement/UIElement.h"
#include "UI/UIProperty/UIProperty.h"

#include <SDL/SDL.h>
#include <vector>
#include <string>

struct TextfieldData 
{
	std::string m_text;
	std::string m_hAlign;
	std::string m_vAlign;
	std::string m_fontName;
	int m_fontSize;
	bool m_flexSize;

	TextfieldData()
		: m_text("")
		, m_hAlign("left")
		, m_vAlign("top")
		, m_fontName("Assets/Fonts/Grandstander-Medium.ttf")
		, m_fontSize(16)
		, m_flexSize(true)
	{
	}
};

class UITextfield final : public UIElement
{
public:
	explicit UITextfield(const TextfieldData& data);

	void Render(SDL_Renderer* pRenderer, float parentW, float parentH) override;
	void Rearrange(const UIElement* pParent, int& x, int& y, int& maxW, int& maxH) override;
	void RefreshTexture() override;

	void AddBindingTextsRefreshingEvents();

public:
	std::vector<UIProperty> m_bindingTexts;

private:
	std::string m_text;
	std::string m_fontName;
	std::string m_hAlign;
	std::string m_vAlign;
	int m_fontSize;
};


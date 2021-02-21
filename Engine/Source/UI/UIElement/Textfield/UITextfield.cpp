#include "UITextfield.h"
#include "UIApplication.h"

UITextfield::UITextfield(const TextfieldData& data)
	: m_text(data.m_text)
	, m_fontName(data.m_fontName)
	, m_hAlign(data.m_hAlign)
	, m_vAlign(data.m_vAlign)
	, m_fontSize(data.m_fontSize)
{
	m_flexSize = data.m_flexSize;
	m_elementType = ElementType::kTextfield;
}

void UITextfield::Render(SDL_Renderer* pRenderer, float parentW, float parentH)
{
	if (!m_pTexture)
		return;

	SDL_Rect textRect = m_renderRect;

	if (m_flexSize)
		SDL_QueryTexture(m_pTexture.get(), nullptr, nullptr, &textRect.w, &textRect.h);

	SDL_RenderCopy(pRenderer, m_pTexture.get(), nullptr, &textRect);
}

void UITextfield::Rearrange(const UIElement* pParent, int& x, int& y, int& maxW, int& maxH)
{
	// do the dimension resize
	UIElement::Rearrange(pParent, x, y, maxW, maxH);

	// rebuild texture
	if (m_width.m_type == Coordinate::Type::kPercentage || m_height.m_type == Coordinate::Type::kPercentage)
		RefreshTexture();
}

void UITextfield::AddBindingTextsRefreshingEvents()
{
	for (auto& prop : m_bindingTexts)
	{
		prop.ChangedEvent() += [this](UIProperty*, const UIProperty::ValueType&, const UIProperty::ValueType&)
		{
			this->m_pTexture = nullptr;
			this->RefreshTexture();
		};
	}
}

void UITextfield::RefreshTexture()
{
	TextfieldData data;
	data.m_text = m_text;
	data.m_fontName = m_fontName;
	data.m_fontSize = m_fontSize;
	data.m_hAlign = m_hAlign;
	data.m_vAlign = m_vAlign;

	for (auto& txt : m_bindingTexts)
	{
		size_t pos = data.m_text.find("%s");
		if (pos == std::string::npos)
			break;

		data.m_text.replace(pos, 2, txt.ToString());
	}
	
	int width = m_positionRect.w;
	int height = m_positionRect.h;

	SDL_Texture* pTexture = UIApplication::Get()->GetResourceLoader().CreateTextfield(data, width, height);

	if (m_flexSize || m_positionRect.w == 0 || m_positionRect.h == 0)
	{
		SDL_QueryTexture(pTexture, nullptr, nullptr, &width, &height);

		if (m_positionRect.w == 0)
			m_width.m_value = width;
		if (m_positionRect.h == 0)
			m_height.m_value = height;
	}

	if (pTexture)
	{
		m_pTexture = UniqueTexture(pTexture, &SDL_DestroyTexture);
	}
		
}

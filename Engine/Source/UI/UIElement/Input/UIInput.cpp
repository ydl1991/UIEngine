#include "UIInput.h"
#include "Keyboard/Keyboard.h"
#include "UIApplication.h"
#include "Utils/Helper.h"
#include <algorithm>
#include <variant>

UIInput::UIInput(const InputData& data)
	: m_state(FocusState::kNotFocused)
	, m_placeHolder(data.m_placeHolder)
	, m_max(data.m_max)
	, m_color({ 0xFF, 0xFF, 0xFF, 0xFF })
	, m_defaultTextColor({ 0xB3, 0xB3, 0xB3, 0x66 })
	, m_pFont(nullptr)
	, m_fontSize(data.m_fontSize)
	, m_pTextTexture(nullptr, nullptr)
	, m_pTextSurface(nullptr, nullptr)
	, m_cursorRect{}
	, m_cursorPosition(0)
	, m_cursorHeight(0)
	, m_scrollOffset(0)
{
	m_pFont = TTF_OpenFont(data.m_font.c_str(), data.m_fontSize);

	if (!m_pFont)
	{
		SDL_Log("Failed to open font: %s", SDL_GetError());
	}
	else
	{
		// set cursor height and element height
		int height = TTF_FontHeight(m_pFont);
		m_cursorHeight = height;
		m_height = Coordinate(height);
		// init default text if any
		if (!m_placeHolder.empty())
			m_pTextSurface = UniqueSurface(TTF_RenderUTF8_Blended(m_pFont, m_placeHolder.c_str(), m_defaultTextColor), &SDL_FreeSurface);
	}

	m_elementType = ElementType::kInput;

	ParseInputType(data.m_inputType);
	m_text.ChangedEvent() += [this](UIProperty* , const UIProperty::ValueType&, const UIProperty::ValueType&)
	{
		m_pTextTexture = nullptr;
	};
}

UIInput::~UIInput()
{
	if (m_pFont)
		TTF_CloseFont(m_pFont);
}

void UIInput::UpdateFocus()
{
	CheckKeyPress();
	CheckUpdateSurface();
}

void UIInput::Render(SDL_Renderer* pRenderer, float parentW, float parentH)
{
	// create texture if not already created
	if (!m_pTextTexture && !m_pTextSurface)
		CheckUpdateSurface();

	if (!m_pTextTexture && m_pTextSurface)
	{
		m_pTextTexture = UniqueTexture(SDL_CreateTextureFromSurface(pRenderer, m_pTextSurface.get()), &SDL_DestroyTexture);
		m_pTextSurface = nullptr;

		if (m_pTextTexture)
			SDL_SetTextureBlendMode(m_pTextTexture.get(), SDL_BLENDMODE_BLEND);
	}

	// render texture
	if (m_pTextTexture)
	{
		SDL_Rect textRect = m_renderRect;
		SDL_QueryTexture(m_pTextTexture.get(), nullptr, nullptr, &textRect.w, &textRect.h);

		textRect.x -= m_scrollOffset;
		SDL_RenderSetClipRect(pRenderer, &m_renderRect);
		SDL_RenderCopy(pRenderer, m_pTextTexture.get(), nullptr, &textRect);
		SDL_RenderSetClipRect(pRenderer, nullptr);
	}

	// render cursor
	if (m_state == FocusState::kFocused)
	{
		SDL_SetRenderDrawColor(pRenderer, 0xFF, 0xFF, 0x00, 0xFF);
		SDL_RenderFillRect(pRenderer, &m_cursorRect);
		SDL_SetRenderDrawColor(pRenderer, 0x00, 0xFF, 0x00, 0xFF);
	}
	else
		SDL_SetRenderDrawColor(pRenderer, 0x00, 0x00, 0xFF, 0xFF);

	// render input box
	SDL_RenderDrawRect(pRenderer, &m_renderRect);
}

void UIInput::NotifyText(const char* pText)
{
	std::string text = m_text.ToString();
	int currentLen = (int)text.size();
	if (currentLen < m_max)
	{
		int txtLen = (int)std::strlen(pText);
		txtLen = m_max - currentLen < txtLen ? m_max - currentLen : txtLen;
		text.insert(m_cursorPosition, pText, txtLen);
		
		if (WriteInput(text))
			AdjustCursorPosition(txtLen);
	}
	else if (m_max == 0)
	{
		text.insert(m_cursorPosition, pText);
		
		if (WriteInput(text))
			AdjustCursorPosition((int)std::strlen(pText));
	}
}

void UIInput::GainFocus()
{
	m_state = FocusState::kFocused;
}

void UIInput::LoseFocus()
{
	m_state = FocusState::kNotFocused;
}

void UIInput::CheckKeyPress()
{
	std::string text = m_text.ToString();
	if (Keyboard::Get().IsKeyDown(SDL_SCANCODE_BACKSPACE))
	{
		if (!text.empty() && m_cursorPosition > 0)
		{
			text.erase((size_t)m_cursorPosition - 1, 1);

			if (WriteInput(text))
				AdjustCursorPosition(-1);
		}
	}
	else if (Keyboard::Get().IsKeyDown(SDL_SCANCODE_DELETE))
	{
		if (!text.empty() && m_cursorPosition < (int)text.size())
		{
			text.erase((size_t)m_cursorPosition, 1);
			WriteInput(text);
		}
	}
	else if (Keyboard::Get().IsKeyDown(SDL_SCANCODE_LEFT))
	{
		AdjustCursorPosition(-1);
	}
	else if (Keyboard::Get().IsKeyDown(SDL_SCANCODE_RIGHT))
	{
		AdjustCursorPosition(1);
	}
}

void UIInput::RemoveChild(int id)
{
	for (auto& vec : m_stateChildIndices)
	{
		auto it = std::find(vec.begin(), vec.end(), id);
		if (it != vec.end())
			vec.erase(it);
	}
}

void UIInput::CheckUpdateSurface()
{
	const std::string& text = m_text.ToString();
	if (!m_pTextTexture && m_pFont)
	{
		// if text is not empty, create texture from text
		if (!text.empty())
			m_pTextSurface = UniqueSurface(TTF_RenderUTF8_Blended(m_pFont, text.c_str(), m_color), &SDL_FreeSurface);
		// otherwise create texture from default text if any
		else if (!m_placeHolder.empty())
			m_pTextSurface = UniqueSurface(TTF_RenderUTF8_Blended(m_pFont, m_placeHolder.c_str(), m_defaultTextColor), &SDL_FreeSurface);
	}

	m_cursorRect.x = GetCursorPixelOffset() + m_renderRect.x - m_scrollOffset;
	m_cursorRect.y = m_renderRect.y;
	m_cursorRect.w = 5;
	m_cursorRect.h = m_cursorHeight;
}

void UIInput::AdjustCursorPosition(int direction)
{
	const std::string& text = m_text.ToString();
	m_cursorPosition = std::max<int>(0, std::min<int>((int)text.size(), m_cursorPosition + direction));

	int cursorX = GetCursorPixelOffset();
	m_scrollOffset = std::max<int>(0, cursorX - m_positionRect.w);
}

int UIInput::GetCursorPixelOffset()
{
	const std::string& text = m_text.ToString();
	
	if (m_cursorPosition > (int)text.size())
		m_cursorPosition = (int)text.size();

	std::string textPrecedingCursor;
	textPrecedingCursor.assign(text.begin(), text.begin() + m_cursorPosition);

	int offset = 0;
	TTF_SizeUTF8(m_pFont, textPrecedingCursor.c_str(), &offset, nullptr);

	return offset;
}

void UIInput::ParseInputType(const std::string& inputType)
{
	if (inputType == "int")
		m_text = 0;
	else if (inputType == "float")
		m_text = 0.0f;
}

bool UIInput::WriteInput(const std::string& text)
{
	UIProperty::Type propertyType = m_text.CurrentHoldingType();

	if (propertyType == UIProperty::Type::kString)
		return WriteString(text);
	if (propertyType == UIProperty::Type::kInt)
		return WriteInt(text);
	if (propertyType == UIProperty::Type::kBool)
		return WriteBool(text);
	if (propertyType == UIProperty::Type::kFloat)
		return WriteFloat(text);

	return false;
}

bool UIInput::WriteString(const std::string& text)
{
	m_text = text;
	m_pTextTexture = nullptr;

	return true;
}

bool UIInput::WriteInt(const std::string& text)
{
	if (text.empty())
	{
		m_text = 0;
		m_pTextTexture = nullptr;
		return true;
	}
	
	if (!IsInt(text))
	{
		SDL_Log("Fail to write Input to a int property! Make sure the input only contains integer!");
		return false;
	}

	m_text = std::stoi(text);
	m_pTextTexture = nullptr;

	return true;
}

bool UIInput::WriteBool(const std::string& text)
{
	//
	return true;
}

bool UIInput::WriteFloat(const std::string& text)
{
	if (text.empty())
	{
		m_text = 0.f;
		m_pTextTexture = nullptr;
		return true;
	}

	if (!IsFloat(text))
	{
		SDL_Log("Fail to write Input to a float property! Make sure the input only contains number!");
		return false;
	}

	m_text = std::stof(text);
	m_pTextTexture = nullptr;

	return true;
}

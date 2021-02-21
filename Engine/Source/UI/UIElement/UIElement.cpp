#include "UIElement.h"
#include "Mouse/Mouse.h"
#include "UI/UIProperty/UIProperty.h"
#include "UI/UIElement/Button/UIButton.h"
#include "UIApplication.h"
#include "UI/UIManager.h"

UIElement::UIElement()
	: m_positionRect()
	, m_renderRect()
	, m_childLayout(LayoutType::kNone)
	, m_elementType(ElementType::kNone)
	, m_visible(true)
	, m_flexSize(true)
	, m_id()
	, m_horizontalLayoutGap()
	, m_verticalLayoutGap()
	, m_pTexture(nullptr, nullptr)
{
	m_width.m_value.ChangedEvent() += [this](UIProperty*, const UIProperty::ValueType&, const UIProperty::ValueType&)
	{
		UIApplication::Get()->SignalRearrange();
	};

	m_height.m_value.ChangedEvent() += [this](UIProperty*, const UIProperty::ValueType&, const UIProperty::ValueType&)
	{
		UIApplication::Get()->SignalRearrange();
	};

	m_buttonState.ChangedEvent() += [this](UIProperty*, const UIProperty::ValueType&, const UIProperty::ValueType&)
	{
		auto& uiManager = UIApplication::Get()->GetUIManager();

		if (auto* pParent = uiManager.GetParentOf(m_id); pParent->m_elementType == ElementType::kButton)
			dynamic_cast<UIButton*>(pParent)->ReorderChild(m_id);
	};
}

void UIElement::Update()
{
	m_renderRect = m_positionRect;
}

void UIElement::UpdateFocus()
{
}

void UIElement::Render(SDL_Renderer* pRenderer, float parentW, float parentH)
{

}

void UIElement::Rearrange(const UIElement* pParent, int& x, int& y, int& maxW, int& maxH)
{
	ElementType type = pParent->m_elementType;
	SDL_Rect parentRect = pParent->m_positionRect;
	LayoutType parentLayout = pParent->m_childLayout;
	int horizontalGap = pParent->m_horizontalLayoutGap;
	int verticalGap = pParent->m_verticalLayoutGap;

	int width = (int)m_width.Resolve((float)parentRect.w);
	int height = (int)m_height.Resolve((float)parentRect.h);

	// adjust line if parent has a wrap layout
	if (parentLayout == LayoutType::kWrapHorizontal && x + width > parentRect.x + parentRect.w)
	{
		x = type == ElementType::kRepeat ? parentRect.x : parentRect.x + horizontalGap;
		y += maxH + verticalGap;
		maxH = height;
	}
	else if (parentLayout == LayoutType::kWrapVertical && y + height > parentRect.y + parentRect.h)
	{
		y = type == ElementType::kRepeat ? parentRect.y : parentRect.y + verticalGap;
		x += maxW + horizontalGap;
		maxW = width;
	}

	// adjust position of the child element
	m_positionRect.x = x;
	m_positionRect.y = y;

	if (m_width.m_type == Coordinate::Type::kPercentage && x + width > parentRect.x + parentRect.w)
		m_positionRect.w = parentRect.w - m_positionRect.x;
	else
		m_positionRect.w = width;

	if (m_height.m_type == Coordinate::Type::kPercentage && y + height > parentRect.y + parentRect.h)
		m_positionRect.h = parentRect.h - m_positionRect.y;
	else
		m_positionRect.h = height;

	// increment the x or y by child's width and height
	if (parentLayout == LayoutType::kStackHorizontal || parentLayout == LayoutType::kWrapHorizontal)
		x += m_positionRect.w + horizontalGap;
	else if (parentLayout == LayoutType::kStackVertical || parentLayout == LayoutType::kWrapVertical)
		y += m_positionRect.h + verticalGap;

	if (width > maxW)
		maxW = m_positionRect.w;
	if (height > maxH)
		maxH = m_positionRect.h;
}

bool UIElement::HitTest()
{
	int mouseX = Mouse::Get().GetMouseX();
	int mouseY = Mouse::Get().GetMouseY();
	if (m_positionRect.x <= mouseX && mouseX <= m_positionRect.x + m_positionRect.w && m_positionRect.y <= mouseY && mouseY <= m_positionRect.y + m_positionRect.h)
		return true;

	return false;
}

void UIElement::NotifyText(const char* pText)
{
}

void UIElement::GainFocus()
{
}

void UIElement::LoseFocus()
{
}

void UIElement::CheckKeyPress()
{
}

int UIElement::CheckMouseClick()
{
	if (Mouse::Get().IsButtonDown(Mouse::MouseButton::kButtonLeft))
		return (int)Mouse::MouseButton::kButtonLeft;

	return -1;
}

void UIElement::RefreshTexture()
{
}

bool UIElement::CompareTag(const std::string& tag) const
{
	if (tag.empty() || m_tag.empty())
		return false;

	return m_tag == tag;
}

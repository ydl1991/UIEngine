#pragma once
#include "UI/UIProperty/UIProperty.h"

#include <SDL/SDL.h>
#include <memory>
#include <string>

struct Coordinate
{
	enum class Type
	{
		kPixel,
		kPercentage,  // 0-100
	};

	Coordinate() : m_type(Type::kPixel), m_value(0) { }
	explicit Coordinate(int val) : m_type(Type::kPixel), m_value(val) { }
	explicit Coordinate(float val) : m_type(Type::kPercentage), m_value(val) { }

	[[nodiscard]] float Resolve(float dimension) const
	{
		if (m_type == Coordinate::Type::kPixel)
			return (float)(*m_value.Get<int>());
		else if (m_type == Coordinate::Type::kPercentage)
			return (*m_value.Get<float>() / 100.f) * dimension;
		return 0.0f;
	}

	Type m_type;
	UIProperty m_value;
};

class UIElement
{
public:
	enum class ElementType
	{
		kNone = 0,
		kStack,
		kWrap,
		kImage,
		kTextfield,
		kButton,
		kRect,
		kMask,
		kScroll,
		kInput,
		kRepeat
	};

	enum class LayoutType
	{
		kNone,				// children determine their own positions
		kStackVertical,		// child positions computed by parent
		kStackHorizontal,	// child positions computed by parent
		kWrapVertical,
		kWrapHorizontal,
		kScrollVertical,
		kScrollHorizontal
	};

	using UniqueTexture = std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)>;

	UIElement();
	virtual ~UIElement() = default;

	virtual void Update();
	virtual void UpdateFocus();
	virtual void Render(SDL_Renderer* pRenderer, float parentW, float parentH);
	virtual void Rearrange(const UIElement* pParent, int& x, int& y, int& maxW, int& maxH);
	virtual bool HitTest();
	virtual void NotifyText(const char* pText);
	virtual void GainFocus();
	virtual void LoseFocus();
	virtual void CheckKeyPress();
	virtual int CheckMouseClick();
	virtual void RefreshTexture();
	[[nodiscard]] bool CompareTag(const std::string& tag) const;

	[[nodiscard]] SDL_Texture* GetTexture() const { return m_pTexture.get(); }

	SDL_Rect m_positionRect;
	SDL_Rect m_renderRect;
	Coordinate m_width, m_height;
	LayoutType m_childLayout;
	ElementType m_elementType;
	UIProperty m_buttonState;
	bool m_visible;
	bool m_flexSize;
	int m_id;
	int m_horizontalLayoutGap;
	int m_verticalLayoutGap;
	std::string m_tag;

protected:
	UniqueTexture m_pTexture;
};


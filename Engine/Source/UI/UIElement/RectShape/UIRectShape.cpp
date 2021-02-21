#include "UIRectShape.h"

UIRectShape::UIRectShape()
	: m_filled(true)
	, m_r(255)
	, m_g(255)
	, m_b(255)
{
	m_elementType = ElementType::kRect;
}

void UIRectShape::Render(SDL_Renderer* pRenderer, float parentW, float parentH)
{
	SDL_SetRenderDrawColor(
		pRenderer, 
		static_cast<Uint8>(*m_r.Get<int>()), 
		static_cast<Uint8>(*m_g.Get<int>()), 
		static_cast<Uint8>(*m_b.Get<int>()), 
		255
	);

	if (m_filled)
		SDL_RenderFillRect(pRenderer, &m_renderRect);
	else
		SDL_RenderDrawRect(pRenderer, &m_renderRect);
}

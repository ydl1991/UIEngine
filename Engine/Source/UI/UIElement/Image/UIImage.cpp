#include "UIImage.h"
#include "UIApplication.h"

UIImage::UIImage(const ImageData& data, SDL_Texture* pTexture)
	: m_type(data.m_type)
	, m_source(data.m_source)
	, m_slices{ }
	, m_dX(0)
	, m_dY(0)
{
	std::memcpy(m_slices, data.m_slices, sizeof data.m_slices);
	m_pTexture = UniqueTexture(pTexture, &SDL_DestroyTexture);
	m_elementType = ElementType::kImage;
}

void UIImage::Render(SDL_Renderer* pRenderer, float parentW, float parentH)
{
	if (m_type == ImageType::kNormal)
	{	
		SDL_RenderCopy(pRenderer, m_pTexture.get(), nullptr, &m_renderRect);
	}
	else if (m_type == ImageType::k9Slices)
	{
		RenderSliced(pRenderer, parentW, parentH);
	}
}

// ------------------------------------------------------- //
//					Render sliced image
// ------------------------------------------------------- //
void UIImage::RenderSliced(SDL_Renderer* pRenderer, float parentW, float parentH)
{
	int w, h;
	SDL_QueryTexture(m_pTexture.get(), nullptr, nullptr, &w, &h);

	// 9 rects representing TL, T, TR, L, M, R, BL, B, BR
	const SDL_Rect s_sourceRectsMap[s_kNineSlice] = {
		// Top
		SDL_Rect{0,						0,			m_slices[0],						m_slices[2]},
		SDL_Rect{m_slices[0],			0,			w - m_slices[1] - m_slices[0],		m_slices[2]},
		SDL_Rect{w - m_slices[1],		0,			m_slices[1],						m_slices[2]},

		// Middle
		SDL_Rect{0,					m_slices[2],		m_slices[0],						h - m_slices[3] - m_slices[2]},
		SDL_Rect{m_slices[0],		m_slices[2],		w - m_slices[1] - m_slices[0],		h - m_slices[3] - m_slices[2]},
		SDL_Rect{w - m_slices[1],	m_slices[2],		m_slices[1],						h - m_slices[3] - m_slices[2]},

		// Bot
		SDL_Rect{0,					h - m_slices[3],		m_slices[0],						m_slices[3]},
		SDL_Rect{m_slices[0],		h - m_slices[3],		w - m_slices[1] - m_slices[0],		m_slices[3]},
		SDL_Rect{w - m_slices[1],	h - m_slices[3],		m_slices[1],						m_slices[3]},
	};

	int width = m_renderRect.w;
	int height = m_renderRect.h;

	const SDL_Rect destRectMap[s_kNineSlice] = {
		// Top Left
		SDL_Rect{ m_renderRect.x,	  m_renderRect.y,   m_slices[0],   m_slices[2] },

		// Top
		SDL_Rect{ m_renderRect.x + m_slices[0],   m_renderRect.y,   width - m_slices[1] - m_slices[0],	  m_slices[2] },

		// Top Right
		SDL_Rect{ m_renderRect.x + width - m_slices[1],	  m_renderRect.y,   m_slices[1],   m_slices[2] },

		// Mid Left
		SDL_Rect{ m_renderRect.x,	  m_renderRect.y + m_slices[2],	  m_slices[0],	  height - m_slices[3] - m_slices[2] },

		// Mid
		SDL_Rect{ m_renderRect.x + m_slices[0], m_renderRect.y + m_slices[2], width - m_slices[1] - m_slices[0], height - m_slices[3] - m_slices[2] },

		// Mid Right
		SDL_Rect{ m_renderRect.x + width - m_slices[1],   m_renderRect.y + m_slices[2],   m_slices[1],   height - m_slices[3] - m_slices[2] },

		// Bot Left
		SDL_Rect{ m_renderRect.x,    m_renderRect.y + height - m_slices[3],	   m_slices[0],    m_slices[3] },

		// Bot
		SDL_Rect{ m_renderRect.x + m_slices[0],	m_renderRect.y + height - m_slices[3],   width - m_slices[1] - m_slices[0],   m_slices[3] },

		// Bot Right
		SDL_Rect{ m_renderRect.x + width - m_slices[1],	  m_renderRect.y + height - m_slices[3],   m_slices[1],   m_slices[3]},
	};

	for (size_t i = 0; i < s_kNineSlice; ++i)
	{
		SDL_RenderCopy(pRenderer, m_pTexture.get(), &s_sourceRectsMap[i], &destRectMap[i]);
	}
}

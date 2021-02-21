#pragma once
#include "UI/UIElement/UIElement.h"

#include <SDL/SDL.h>
#include <string>

enum class ImageType
{
	kNormal = 0,
	k9Slices,
};

struct ImageData
{
	static constexpr size_t s_kSliceSize = 4;
	
	int m_slices[s_kSliceSize];
	std::string m_source;
	ImageType m_type;

	ImageData()
		: m_slices{ }
		, m_source("")
		, m_type(ImageType::kNormal)
	{
		std::memset(m_slices, 0, sizeof m_slices);
	}
};

class UIImage final : public UIElement
{
public:
	explicit UIImage(const ImageData& data, SDL_Texture* pTexture);

	void Render(SDL_Renderer* pRenderer, float parentW, float parentH) override;
	[[nodiscard]] std::string Source() const { return m_source; }
	
private:
	void RenderSliced(SDL_Renderer* pRenderer, float parentW, float parentH);

private:

	static constexpr size_t s_kNineSlice = 9;
	static constexpr size_t s_kSliceSize = 4;

	ImageType m_type;
	std::string m_source;
	int m_slices[s_kSliceSize];
	int m_dX;
	int m_dY;
};
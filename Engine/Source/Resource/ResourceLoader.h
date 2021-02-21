#pragma once
#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>
#include <vector>
#include <string>

struct TextfieldData;	// Defined in UITextfield.h
struct ImageData;		// Defined in UIImage.h

class ResourceLoader
{
public:
	ResourceLoader()								 = default;
	ResourceLoader(const ResourceLoader&)			 = delete;
	ResourceLoader& operator=(const ResourceLoader&) = delete;

	bool Init();

	// Create textfield from TextfieldData
	// Detailed commenting please see in .cpp file 
	[[nodiscard]] SDL_Texture* CreateTextfield(const TextfieldData& data, int spaceW, int spaceH) const;

	// Create image from ImageData
	// Detailed commenting please see in .cpp file 
	SDL_Texture* CreateImageTexture(const ImageData& data);

private:
	// Textfield helpers
	std::vector<std::string> LineBreaker(const char* str, int stringWidth, int lineWidth) const;
	SDL_Surface* MakeAlignedSurface(TTF_Font* pFont, const char* pStr, const char* pHAlign, const char* pVAlign, int lineWidth, int totalHeight) const;
};


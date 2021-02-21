#include "ResourceLoader.h"
#include "UI/UIElement/Image/UIImage.h"
#include "UI/UIElement/Textfield/UITextfield.h"
#include "../UIApplication.h"

#include <SDL/SDL_image.h>
#include <cmath>
#include <algorithm>

static constexpr SDL_Color s_kWhite = { 255, 255, 255, 255 };

bool ResourceLoader::Init()
{
	return true;
}

// ------------------------------------------------------- //
// Create textfield
// 
// Params:
// - data: All the info about the textfield
// - spaceW: Space width the textfield within
// - spaceH: Space height the textfield within
//
// Return:
// A sdl texture
// ------------------------------------------------------- //
SDL_Texture* ResourceLoader::CreateTextfield(const TextfieldData& data, int spaceW, int spaceH) const
{
	// this opens a font style and sets a size
	TTF_Font* pFont = TTF_OpenFont(data.m_fontName.c_str(), data.m_fontSize);

	if (!pFont)
	{
		SDL_Log("TTF_OpenFont failed: %s\n", TTF_GetError());
		return nullptr;
	}

	SDL_Surface* pTextSurface = nullptr;

	// make aligned surface
	// if space is specified, create surface using align, otherwise create the texture in one line
	if (spaceW != 0 || spaceH != 0)
	{
		pTextSurface = MakeAlignedSurface(pFont, data.m_text.c_str(), data.m_hAlign.c_str(), data.m_vAlign.c_str(), spaceW, spaceH);
	}
	else
	{
		pTextSurface = TTF_RenderUTF8_Blended(pFont, data.m_text.c_str(), s_kWhite);
	}

	SDL_Texture* pTexture = SDL_CreateTextureFromSurface(UIApplication::Get()->GetRenderer(), pTextSurface);
	SDL_FreeSurface(pTextSurface);
	TTF_CloseFont(pFont);

	return pTexture;
}

// ------------------------------------------------------- //
// Create image
// 
// Params:
// - data: All the info about the image
//
// Return:
// A sdl texture
// ------------------------------------------------------- //
SDL_Texture* ResourceLoader::CreateImageTexture(const ImageData& data)
{
	SDL_Surface* pTempSurface = IMG_Load(data.m_source.c_str());
	if (!pTempSurface)
	{
		SDL_Log("Failed to load image: %s", IMG_GetError());
		return nullptr;
	}

	SDL_Texture* pTexture = SDL_CreateTextureFromSurface(UIApplication::Get()->GetRenderer(), pTempSurface);
	SDL_FreeSurface(pTempSurface);
	return pTexture;
}

// ------------------------------------------------------- //
// Line breaker
// Breaks the input string into multiple strings each with
// specific line width
//
// Params:
// - str: The string to break
// - stringWidth: the original width of the string
// - lineWidth: the width of each line
//
// Return:
// A vector of strings
// ------------------------------------------------------- //
std::vector<std::string> ResourceLoader::LineBreaker(const char* str, int stringWidth, int lineWidth) const
{
	if (stringWidth == lineWidth)
	{
		return { str };
	}

	std::vector<std::string> result;

	std::string original = str;
	// number of characters left to process
	size_t charLeft = original.size();
	// approximate width per char
	auto widthPerChar = (size_t)std::ceil((float)stringWidth / (float)charLeft);
	// approximate number of character per line
	size_t numCharPerLine = (size_t)lineWidth / widthPerChar;

	size_t lineStart = 0;
	while (charLeft > numCharPerLine)
	{
		// find the last 'space' appeared until the end of the current line
		size_t lineEnd = original.find_last_of(' ', lineStart + numCharPerLine);
		// add the substring to result
		result.push_back(original.substr(lineStart, lineEnd - lineStart));
		// update the number of characters left to process
		charLeft -= (lineEnd - lineStart);
		// update line start to next line
		lineStart = lineEnd + 1;
	}

	// add the last line
	result.push_back(original.substr(lineStart));

	// if use std::move will prevent compiler copy elision
	return result;
}

// ------------------------------------------------------- //
// Align
// Wrap and Align the strings horizontally and vertically
//
// Return:
// A sdl surface
// ------------------------------------------------------- //
SDL_Surface* ResourceLoader::MakeAlignedSurface(TTF_Font* pFont, const char* pStr, const char* pHAlign, const char* pVAlign, int lineWidth, int totalHeight) const
{
	// the expected width and height of the original string
	int strW = 0;
	int strH = 0;
	TTF_SizeText(pFont, pStr, &strW, &strH);

	// determine the surface width, if line width determined, use line width, otherwise use original string width
	int surfaceWidth = lineWidth == 0 ? strW : lineWidth;
	// break line by surface width
	auto lines = LineBreaker(pStr, strW, surfaceWidth);
	// calculate the heights of all the lines
	int lineHeights = strH * (int)lines.size();
	// determine the surface height by comparing line heights and total heights.
	int surfaceHeight = totalHeight == 0 || totalHeight <= lineHeights ? lineHeights : totalHeight;

	// create a transparent surface as base with determined surface width and height
	SDL_Surface* pSurface = SDL_CreateRGBSurface(0, surfaceWidth, surfaceHeight, 32, 0xff, 0xff00, 0xff0000, 0xff000000);

	for (size_t i = 0; i < lines.size(); ++i)
	{
		// create surface for each string line
		SDL_Surface* pLine = TTF_RenderUTF8_Blended(pFont, lines[i].c_str(), s_kWhite);
		
		// set starting position of current line adjusted by alignment setting
		SDL_Rect rect{};

		// adjust horizontally
		if (lineWidth != 0)
		{
			if (std::strcmp(pHAlign, "center") == 0)
				rect.x = (lineWidth - pLine->w) / 2;
			else if (std::strcmp(pHAlign, "right") == 0)
				rect.x = lineWidth - pLine->w;
		}

		// adjust vertically
		if (totalHeight == 0 || totalHeight <= lineHeights || std::strcmp(pVAlign, "top") == 0)
		{
			rect.y = (int)i * strH;
		}
		else if (std::strcmp(pVAlign, "middle") == 0)
		{
			rect.y = (totalHeight - lineHeights) / 2 + (int)i * strH;
		}
		else if (std::strcmp(pVAlign, "bottom") == 0)
		{
			rect.y = (totalHeight - lineHeights) + (int)i * strH;
		}

		// add the surface to our base
		SDL_BlitSurface(pLine, nullptr, pSurface, &rect);
		SDL_FreeSurface(pLine);
	}

	return pSurface;
}


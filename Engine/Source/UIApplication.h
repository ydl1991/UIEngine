#pragma once
#include "Resource/ResourceLoader.h"
#include "UI/UIManager.h"

#include <memory>
#include <SDL/SDL.h>

class GameLogic;
class XOrShift;

class UIApplication
{
public:
	// Alias
	using UniqueWindow = std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)>;
	using UniqueRenderer = std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)>;

	UIApplication();
	virtual ~UIApplication();
	UIApplication(const UIApplication&)			   = delete;
	UIApplication& operator=(const UIApplication&) = delete;

	static UIApplication* Get();

	bool Init();
	bool Init(const char* pFileName);
	[[nodiscard]] bool PostInit() const;
	void Run();
	bool LoadXML(const char* pFileName);

	// --------------------------------------------------------------------- //
	// Accessors & Mutator
	// --------------------------------------------------------------------- //
	ResourceLoader& GetResourceLoader() { return m_resourceLoader; }
	UIManager& GetUIManager() { return m_UIManager; }
	[[nodiscard]] SDL_Renderer* GetRenderer() const { return m_pRenderer.get(); }
	[[nodiscard]] GameLogic* GetGame() const { return m_pGame.get(); }
	[[nodiscard]] XOrShift* GetRNG() const { return m_pRng.get(); }

	[[nodiscard]] int GetWindowWidth() const { return m_windowWidth; }
	[[nodiscard]] int GetWindowHeight() const { return m_windowHeight; }

	void SignalRearrange() { m_rearrangeUI = true; }
	void Reset();
	
	// Pure Virtual Function 
	virtual std::unique_ptr<GameLogic> CreateGameLogic() = 0;

private:
	bool InitSDL();
	bool UpdateEvent();
	void Render();
	void CheckResizeWindow();
	void InputNextFrame();

private:
	// Static
	static UIApplication* s_pApp;

	UniqueWindow m_pWindow;
	UniqueRenderer m_pRenderer;
	ResourceLoader m_resourceLoader;
	UIManager m_UIManager;
	std::unique_ptr<XOrShift> m_pRng;
	std::unique_ptr<GameLogic> m_pGame;

	int m_windowWidth;
	int m_windowHeight;
	bool m_rearrangeUI;
};


#include "UIApplication.h"
#include "Keyboard/Keyboard.h"
#include "Mouse/Mouse.h"
#include "GameLogic.h"
#include "Utils/XOrShift.h"

#include <TinyXml/tinyxml2.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_ttf.h>
#include <chrono>

UIApplication* UIApplication::s_pApp = nullptr;

UIApplication::UIApplication()
	: m_pWindow(nullptr, nullptr)
	, m_pRenderer(nullptr, nullptr)
	, m_windowWidth(1280)
	, m_windowHeight(720)
	, m_rearrangeUI(false)
{
	SDL_Init(SDL_INIT_EVERYTHING);
	TTF_Init();
	IMG_Init(IMG_INIT_PNG);
}

UIApplication::~UIApplication()
{
	m_pRenderer = nullptr;
	m_pWindow = nullptr;
	m_pGame = nullptr;
	m_UIManager.Clear();
	IMG_Quit();
	TTF_Quit();
	SDL_Quit();
}

UIApplication* UIApplication::Get()
{
	return s_pApp;
}

bool UIApplication::Init()
{
	s_pApp = this;

	if (!InitSDL())
	{
		SDL_Log("Failed to init SDL: %s", SDL_GetError());
		return false;
	}

	if (!m_resourceLoader.Init())
	{
		SDL_Log("Failed to init resource loader: %s", SDL_GetError());
		return false;
	}

	if (!m_UIManager.Init())
	{
		SDL_Log("Failed to init UI manager: %s", SDL_GetError());
		return false;
	}

	// seed random
	m_pRng = std::make_unique<XOrShift>(static_cast<unsigned long>(time(nullptr)));
	if (!m_pRng)
	{
		SDL_Log("Failed to create XorShift random number generator!");
		return false;
	}
	
	m_pGame = CreateGameLogic();
	if (!m_pGame || !m_pGame->Init())
	{
		SDL_Log("Failed to create game!");
		return false;
	}

	return PostInit();
}

bool UIApplication::Init(const char* pFileName)
{
	s_pApp = this;
	
	if (!InitSDL())
	{
		SDL_Log("Failed to init SDL: %s", SDL_GetError());
		return false;
	}

	if (!m_resourceLoader.Init())
	{
		SDL_Log("Failed to init resource loader: %s", SDL_GetError());
		return false;
	}

	if (!m_UIManager.Init())
	{
		SDL_Log("Failed to init UI manager: %s", SDL_GetError());
		return false;
	}

	// seed random
	m_pRng = std::make_unique<XOrShift>(static_cast<unsigned long>(time(nullptr)));
	if (!m_pRng)
	{
		SDL_Log("Failed to create XorShift random number generator!");
		return false;
	}
	
	m_pGame = CreateGameLogic();
	if (!m_pGame || !m_pGame->Init())
	{
		SDL_Log("Failed to create game!");
		return false;
	}
	
	return LoadXML(pFileName) && PostInit();
}

bool UIApplication::PostInit() const
{
	// post init game layer which init IViews
	if (!m_pGame->PostInit())
	{
		SDL_Log("Failed to post init game!");
		return false;
	}

	return true;
}

void UIApplication::Run()
{
	// Calculate delta seconds
	using namespace std::chrono;
	auto lastClock = high_resolution_clock::now();

	while (UpdateEvent() && !m_pGame->m_endGame)
	{
		// how much time has elapsed since the last frame?
		high_resolution_clock::time_point currentClock = high_resolution_clock::now();
		float deltaSeconds = duration_cast<duration<float>>(currentClock - lastClock).count();

		// Simulation
		m_UIManager.Update();
		m_pGame->Update(deltaSeconds);

		CheckResizeWindow();
		Render();

		InputNextFrame();
		lastClock = currentClock;
	}
}

void UIApplication::Render()
{
	SDL_SetRenderDrawColor(m_pRenderer.get(), 0, 0, 0, 255);
	SDL_RenderClear(m_pRenderer.get());

	m_UIManager.Render(m_pRenderer.get());

	SDL_RenderPresent(m_pRenderer.get());
}

void UIApplication::Reset()
{
	m_UIManager.Reset();
	m_pGame->m_processController.AbortAllProcesses();
}

bool UIApplication::InitSDL()
{
	// Create Window
	m_pWindow = UniqueWindow(SDL_CreateWindow(
		"UI Engine",
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		m_windowWidth, m_windowHeight,
		SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL),
		&SDL_DestroyWindow
	);

	if (!m_pWindow)
	{
		SDL_Log("Failed to create window: %s", SDL_GetError());
		return false;
	}

	// Create Renderer
	m_pRenderer = UniqueRenderer(SDL_CreateRenderer(
		m_pWindow.get(),
		-1,
		SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC),
		&SDL_DestroyRenderer
	);

	if (!m_pRenderer)
	{
		SDL_Log("Failed to create renderer: %s", SDL_GetError());
		return false;
	}

	return true;
}

bool UIApplication::LoadXML(const char* pFileName)
{
	tinyxml2::XMLDocument config;
	config.LoadFile(pFileName);

	const auto* rootElement = config.FirstChildElement("UI");
	if (!rootElement)
	{
		SDL_Log("Could not find UI object: %s", SDL_GetError());
		return false;
	}

	if (!m_UIManager.LoadXml(rootElement))
	{
		SDL_Log("Could not Load UI from XML: %s", SDL_GetError());
		return false;
	}

	return true;
}

bool UIApplication::UpdateEvent()
{
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		if (event.type == SDL_TEXTINPUT)
		{
			m_UIManager.NotifyTextOnFocusedElement(event.text.text);
		}
		else if (event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE))
		{
			return false;
		}
		else if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP)
		{
			Keyboard::Get().SetKeyDown(event.key.keysym.scancode, event.type == SDL_KEYDOWN);
		}
		else if (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP)
		{
			if (event.button.button == SDL_BUTTON_LEFT)
				Mouse::Get().SetButtonState(Mouse::MouseButton::kButtonLeft, event.type == SDL_MOUSEBUTTONDOWN);
			else if (event.button.button == SDL_BUTTON_RIGHT)
				Mouse::Get().SetButtonState(Mouse::MouseButton::kButtonRight, event.type == SDL_MOUSEBUTTONDOWN);
		}
		else if (event.type == SDL_MOUSEWHEEL)
		{
			Mouse::Get().SetWheelValue(event.wheel.y);
		}
	}
	return true;
}

void UIApplication::CheckResizeWindow()
{
	int w, h;
	SDL_GetWindowSize(m_pWindow.get(), &w, &h);

	if (w != m_windowWidth || h != m_windowHeight || m_rearrangeUI)
	{
		m_windowWidth = w;
		m_windowHeight = h;

		m_UIManager.Rearrange(m_windowWidth, m_windowHeight);
		m_rearrangeUI = false;
	}
}

void UIApplication::InputNextFrame()
{
	Keyboard::Get().Reset();
	Mouse::Get().Reset();
}

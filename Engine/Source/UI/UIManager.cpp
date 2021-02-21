#include "UIManager.h"
#include "UI/UIElement/Image/UIImage.h"
#include "UI/UIElement/Textfield/UITextfield.h"
#include "UI/UIElement/Button/UIButton.h"
#include "UI/UIElement/RectShape/UIRectShape.h"
#include "UI/UIElement/Scroll/UIScroll.h"
#include "UI/UIElement/Input/UIInput.h"
#include "UI/UIElement/Repeat/UIRepeat.h"
#include "UIApplication.h"
#include "Mouse/Mouse.h"
#include "Utils/Helper.h"

#include <queue>
#include <cassert>

UIManager::UIManager()
	: m_kRootId(0)
	, m_focusElement(-1)
	, m_clickElement(-1)
{
	// Pre-allocate 1000 space for vectors, should be enough for the course, 
	// so we don't have to worry about the overhead of the dynamic re-allocation
	m_elements.reserve(1000);
	m_childrenMap.reserve(1000);
	m_parentMap.reserve(1000);
	m_freeIds.reserve(1000);
	m_dataMap.reserve(1000);
}

bool UIManager::Init()
{
	auto* pApp = UIApplication::Get();
	int windowWidth = pApp->GetWindowWidth();
	int windowHeight = pApp->GetWindowHeight();

	// init the root element
	m_elements.emplace_back(std::make_unique<UIElement>());
	auto& pRoot = m_elements[m_kRootId];
	pRoot->m_id = 0;
	pRoot->m_width = Coordinate(windowWidth);
	pRoot->m_height = Coordinate(windowHeight);
	pRoot->m_positionRect.w = windowWidth;
	pRoot->m_positionRect.h = windowHeight;
	m_childrenMap.emplace_back(std::vector<int>());
	m_parentMap.emplace_back(-1);

	return true;
}

bool UIManager::LoadXml(const tinyxml2::XMLElement* pXml)
{
	return InternalRecursivelyLoadXml(pXml, m_kRootId, 0.f, 0.f);
}

bool UIManager::LoadXml(const char* pFileName, int rootId)
{
	tinyxml2::XMLDocument config;
	config.LoadFile(pFileName);

	const auto* pRootXML = config.FirstChildElement("UI");
	if (!pRootXML)
	{
		SDL_Log("Could not find UI object: %s", SDL_GetError());
		return false;
	}

	int rootParentId = m_parentMap[rootId];
	int width = rootParentId == -1 ? UIApplication::Get()->GetWindowWidth() : m_elements[rootParentId]->m_positionRect.w;
	int height = rootParentId == -1 ? UIApplication::Get()->GetWindowHeight() : m_elements[rootParentId]->m_positionRect.h;
	if (!InternalRecursivelyLoadXml(pRootXML, rootId, static_cast<float>(width), static_cast<float>(height)))
	{
		SDL_Log("Could not Load UI from XML: %s", SDL_GetError());
		return false;
	}

	return true;
}

void UIManager::DynamicallyLoadRepeatElements(const tinyxml2::XMLElement* pXml, int id, int repeatCount)
{
	if (m_childrenMap[id].size() == static_cast<size_t>(repeatCount))
	{
		SDL_Log("Current children already in correct count!");
		return;
	}

	// clear out old elements
	ClearAllElementsInChildren(id);

	// build new elements
	int parentId = m_parentMap[id];
	int parentW = parentId == -1 ? 0 : m_elements[parentId]->m_positionRect.w;
	int parentH = parentId == -1 ? 0 : m_elements[parentId]->m_positionRect.h;
	if (!InternalRecursivelyLoadXml(pXml, id, static_cast<float>(parentW), static_cast<float>(parentH)))
	{
		SDL_Log("Failed to parse repeated UI Elements!");
	}
}

// ------------------------------------------------------- //
// Internally recursively load XML element, parse the info
// into Stack, Wrap, Image or Textfield
//
// Params:
// - pXml: The parsing element
// - wDimension: the parent width as width dimension to resolve children width coordinate
// - hDimension: the parent height as height dimension to resolve children height coordinate
//
// Return:
// Whether it's successful
// ------------------------------------------------------- //
bool UIManager::InternalRecursivelyLoadXml(const tinyxml2::XMLElement* pXml, int parentId, float wDimension, float hDimension)
{
	// parent info
	auto& pParent = m_elements[parentId];
	UIElement::ElementType type = pParent->m_elementType;

	float width = pParent->m_width.Resolve(wDimension);
	float height = pParent->m_height.Resolve(hDimension);
	float parentW = width == 0.f ? wDimension : width;
	float parentH = height == 0.f ? hDimension : height;

	// child position specification
	int childX = type == UIElement::ElementType::kRepeat ? pParent->m_positionRect.x : pParent->m_positionRect.x + pParent->m_horizontalLayoutGap;
	int childY = type == UIElement::ElementType::kRepeat ? pParent->m_positionRect.y : pParent->m_positionRect.y + pParent->m_verticalLayoutGap;
	int maxChildW = 0;
	int maxChildH = 0;

	if (!InternalLoadChildren(pXml, parentId, parentW, parentH, childX, childY, maxChildW, maxChildH))
	{
		SDL_Log("Failed to parse UI Elements!");
		return false;
	}

	if (pParent->m_elementType == UIElement::ElementType::kScroll)
	{
		InternalUpdateScrollBoarder(parentId);
	}
	else if ((pParent->m_elementType == UIElement::ElementType::kWrap || pParent->m_elementType == UIElement::ElementType::kStack) &&
			  pParent->m_flexSize)
	{
		AdjustStackWrapDimension(pParent->m_childLayout, parentId, childX, childY, maxChildW, maxChildH);
	}
	else if (pParent->m_elementType == UIElement::ElementType::kRepeat)
	{
		AdjustRepeatElementDimension(pParent->m_childLayout, parentId, childX, childY, maxChildW, maxChildH);
	}

	return true;
}

int UIManager::FindNextAvailableSpot()
{
	int id = 0;

	if (!m_freeIds.empty())
	{
		id = m_freeIds.back();
		m_freeIds.pop_back();
		return id;
	}

	id = static_cast<int>(m_elements.size());
	m_elements.emplace_back(nullptr);
	m_childrenMap.emplace_back(std::vector<int>());
	m_parentMap.emplace_back(-1);

	return id;
}

// ------------------------------------------------------- //
// Create a child and add child's id to parent's child list
//
// Params:
// - parentId
// - pChild: a unique pointer of UIElement
// Return:
// - child index
// ------------------------------------------------------- //
int UIManager::AddChild(int parentId, SharedUIElement pChild)
{
	// get child id
	int childId = FindNextAvailableSpot();

	// add child to element list and create a children list for the child, assign parent id to child element
	m_elements[childId] = std::move(pChild);
	m_elements[childId]->m_id = childId;
	m_parentMap[childId] = parentId;

	// add child to parent's children list
	m_childrenMap[parentId].emplace_back(childId);

	return childId;
}

void UIManager::Update()
{
	// Update all element
	std::for_each(m_elements.begin(), m_elements.end(), [](SharedUIElement& pElement)
	{
		if (pElement)
			pElement->Update();
	});
	
	// Mouse scrolls
	if (Mouse::Get().GetWheelValue() != 0)
		InternalUpdateUIMouseScroll(m_kRootId);

	// Mouse positions and click focus check
	int newFocus = -1;
	InternalUpdateMouseEvent(m_kRootId, newFocus);
	CheckFocus(newFocus);

	if (m_focusElement != -1)
		m_elements[m_focusElement]->UpdateFocus();

	// Trigger click events
	TriggerClickEvents();
}

// ------------------------------------------------------- //
// A DFS to go through UI layer by layer and render all 
// ------------------------------------------------------- //
void UIManager::Render(SDL_Renderer* pRenderer)
{
	// using stack to represent a DFS rendering
	std::stack<int> openSet;
	openSet.emplace(m_kRootId);

	// mask set stores the parent id of the mask element, and will pop the parent id 
	// when the subtree of the mask element has done processing and a sibling element
	// is present.
	std::stack<int> maskSet;

	while (!openSet.empty())
	{
		int id = openSet.top();
		openSet.pop();

		CheckMaskEffects(pRenderer, id, maskSet);

		// rendering
		auto parentW = static_cast<float>(m_elements[id]->m_positionRect.w);
		auto parentH = static_cast<float>(m_elements[id]->m_positionRect.h);

		for (int child : RetrieveElementChildren(id))
		{
			if (!m_elements[child]->m_visible)
				continue;

			m_elements[child]->Render(pRenderer, parentW, parentH);
			openSet.emplace(child);
		}
	}
}

// ------------------------------------------------------- //
// A BFS to go through UI layer by layer and resize all 
// ------------------------------------------------------- //
void UIManager::Rearrange(int newWidth, int newHeight)
{
	std::queue<int> openSet;
	openSet.emplace(m_kRootId);

	m_elements[m_kRootId]->m_width.m_value = newWidth;
	m_elements[m_kRootId]->m_height.m_value = newHeight;
	m_elements[m_kRootId]->m_positionRect.w = newWidth;
	m_elements[m_kRootId]->m_positionRect.h = newHeight;

	while (!openSet.empty())
	{
		int id = openSet.front();
		openSet.pop();

		SharedUIElement& pElement = m_elements[id];
		UIElement::ElementType type = pElement->m_elementType;
		SDL_Rect parentRect = pElement->m_positionRect;
		int childX = type == UIElement::ElementType::kRepeat ? parentRect.x : parentRect.x + pElement->m_horizontalLayoutGap;
		int childY = type == UIElement::ElementType::kRepeat ? parentRect.y : parentRect.y + pElement->m_verticalLayoutGap;
		int maxChildW = 0;
		int maxChildH = 0;

		for (int child : m_childrenMap[id])
		{
			m_elements[child]->Rearrange(pElement.get(), childX, childY, maxChildW, maxChildH);
			openSet.emplace(child);
		}
	}
}

// ------------------------------------------------------- //
// Update character text to focused element
// ------------------------------------------------------- //
void UIManager::NotifyTextOnFocusedElement(const char* pText)
{
	if (m_focusElement != -1)
		m_elements[m_focusElement]->NotifyText(pText);
}

void UIManager::RegisterClickElement(int id)
{
	m_clickElement = id;
}

void UIManager::Reset()
{
	Clear();
	Init();
	m_focusElement = -1;
	m_clickElement = -1;
}

void UIManager::Clear()
{
	m_elements.clear();
	m_childrenMap.clear();
	m_parentMap.clear();
	m_freeIds.clear();
	m_dataMap.clear();
}

void UIManager::ClearSubtree(int id)
{
	std::queue<int> openSet;
	openSet.emplace(id);
	int parentId = m_parentMap[id];
	
	while (!openSet.empty())
	{
		int cur = openSet.front();
		openSet.pop();

		for (int child : m_childrenMap[cur])
		{
			openSet.emplace(child);
		}

		m_freeIds.emplace_back(cur);
		m_elements[cur] = nullptr;
		m_childrenMap[cur].clear();
		m_parentMap[cur] = -1;
	}

	if (parentId != -1)
		RemoveChildFromParent(id, parentId);
}

void UIManager::RemoveChildFromParent(int childId, int parentId)
{
	if (m_elements[parentId]->m_elementType == UIElement::ElementType::kButton)
	{
		auto* pButton = dynamic_cast<UIButton*>(m_elements[parentId].get());
		pButton->RemoveChild(childId);
	}
	else if (m_elements[parentId]->m_elementType == UIElement::ElementType::kInput)
	{
		auto* pInput = dynamic_cast<UIInput*>(m_elements[parentId].get());
		pInput->RemoveChild(childId);
	}
	
	auto it = std::find(m_childrenMap[parentId].begin(), m_childrenMap[parentId].end(), childId);
	if (it != m_childrenMap[parentId].end())
		m_childrenMap[parentId].erase(it);
}

void UIManager::ClearAllElementsInChildren(int id)
{
	std::queue<int> openSet;

	for (int child : m_childrenMap[id])
	{
		openSet.emplace(child);
	}

	while (!openSet.empty())
	{
		int cur = openSet.front();
		openSet.pop();

		for (int child : m_childrenMap[cur])
		{
			openSet.emplace(child);
		}

		m_freeIds.emplace_back(cur);
		m_elements[cur] = nullptr;
		m_childrenMap[cur].clear();
		m_parentMap[cur] = -1;
	}

	m_childrenMap[id].clear();
}

UIElement* UIManager::GetParentOf(int index)
{
	if (index < 0 || index >= static_cast<int>(m_elements.size()) || m_parentMap[index] == -1)
		return nullptr;

	return GetElementAt(m_parentMap[index]);
}

UIElement* UIManager::GetElementAt(int index)
{
	if (index < 0 || index >= static_cast<int>(m_elements.size()) || !m_elements[index])
		return nullptr;

	return m_elements[index].get();
}

UIElement* UIManager::GetElementWithTag(const std::string& tag)
{
	auto it = std::find_if(m_elements.begin(), m_elements.end(), [&tag](SharedUIElement& pElement) -> bool
	{
		return pElement->CompareTag(tag);
	});

	return it != m_elements.end() ? it->get() : nullptr;
}

UIElement* UIManager::GetChildWithTag(int startId, const std::string& tag)
{
	if (startId < 0 || startId >= static_cast<int>(m_elements.size()) || !m_elements[startId])
		return nullptr;

	std::queue<int> openSet;
	openSet.emplace(startId);

	while (!openSet.empty())
	{
		int id = openSet.front();
		openSet.pop();

		for (int child : m_childrenMap[id])
		{
			if (m_elements[child]->CompareTag(tag))
				return m_elements[child].get();

			openSet.emplace(child);
		}
	}

	return nullptr;
}

std::vector<int>& UIManager::GetChildrenIds(int parentId)
{
	assert(static_cast<int>(m_elements.size()) > parentId && parentId >= 0);
	return m_childrenMap[parentId];
}

UIProperty* UIManager::GetData(const std::string& name)
{
	if (m_dataMap.find(name) == m_dataMap.end())
		return nullptr;

	return &m_dataMap[name];
}

bool UIManager::InternalParseElement(const tinyxml2::XMLElement* pXml, int parentId, float parentW, float parentH, int& x, int& y, int& maxW, int& maxH)
{
	// loop through all children elements
	for (const auto* childElement = pXml->FirstChildElement();
		childElement != nullptr; childElement = childElement->NextSiblingElement())
	{
		if (std::strcmp(childElement->Name(), "Data") == 0)
		{
			if (!ParseData(childElement, parentId))
			{
				SDL_Log("Failed to parse data");
				return false;
			}
		}
		else if (std::strcmp(childElement->Name(), "Stack") == 0)
		{
			if (!ParseStack(childElement, parentId, parentW, parentH, x, y, maxW, maxH))
			{
				SDL_Log("Failed to parse stack data");
				return false;
			}
		}
		else if (std::strcmp(childElement->Name(), "Wrap") == 0)
		{
			if (!ParseWrap(childElement, parentId, parentW, parentH, x, y, maxW, maxH))
			{
				SDL_Log("Failed to parse wrap data");
				return false;
			}
		}
		else if (std::strcmp(childElement->Name(), "Repeat") == 0)
		{
			if (!ParseRepeat(childElement, parentId, parentW, parentH, x, y, maxW, maxH))
			{
				SDL_Log("Failed to parse repeat data");
				return false;
			}
		}
		else if (std::strcmp(childElement->Name(), "Scroll") == 0)
		{
			if (!ParseScroll(childElement, parentId, parentW, parentH, x, y, maxW, maxH))
			{
				SDL_Log("Failed to parse scroll data");
				return false;
			}
		}
		else if (std::strcmp(childElement->Name(), "Mask") == 0)
		{
			if (!ParseMask(childElement, parentId, parentW, parentH, x, y, maxW, maxH))
			{
				SDL_Log("Failed to parse mask data");
				return false;
			}
		}
		else if (std::strcmp(childElement->Name(), "Button") == 0)
		{
			if (!ParseButton(childElement, parentId, parentW, parentH, x, y, maxW, maxH))
			{
				SDL_Log("Failed to parse button data");
				return false;
			}
		}
		else if (std::strcmp(childElement->Name(), "Image") == 0)
		{
			if (!ParseImage(childElement, parentId, parentW, parentH, x, y, maxW, maxH))
			{
				SDL_Log("Failed to parse image data");
				return false;
			}
		}
		else if (std::strcmp(childElement->Name(), "Label") == 0)
		{
			if (!ParseLabel(childElement, parentId, parentW, parentH, x, y, maxW, maxH))
			{
				SDL_Log("Failed to parse label data");
				return false;
			}
		}
		else if (std::strcmp(childElement->Name(), "RectShape") == 0)
		{
			if (!ParseRect(childElement, parentId, parentW, parentH, x, y, maxW, maxH))
			{
				SDL_Log("Failed to parse rect data");
				return false;
			}
		}
		else if (std::strcmp(childElement->Name(), "Input") == 0)
		{
			if (!ParseInput(childElement, parentId, parentW, parentH, x, y, maxW, maxH))
			{
				SDL_Log("Failed to parse input data");
				return false;
			}
		}
	}

	return true;
}

bool UIManager::InternalLoadChildren(const tinyxml2::XMLElement* pXml, int parentId, float parentW, float parentH, int& x, int& y, int& maxW, int& maxH)
{
	if (m_elements[parentId]->m_elementType == UIElement::ElementType::kRepeat)
	{
		auto* pElement = dynamic_cast<UIRepeat*>(m_elements[parentId].get());
		if (!pElement)
		{
			SDL_Log("Failed to cast repeat element!");
			return false;
		}
		
		// make a copy of the current element in case for later use
		if (!pElement->m_pDoc)
		{
			pElement->m_pDoc = std::make_unique<tinyxml2::XMLDocument>();
			pElement->m_pSource = pXml->DeepClone(pElement->m_pDoc.get());
		}
		
		if (int repeatCount = pElement->GetCount(); repeatCount != 0)
		{ 
			for (int i = 0; i < repeatCount; ++i)
			{
				if (!InternalParseElement(pXml, parentId, parentW, parentH, x, y, maxW, maxH))
					return false;
			}
		}
	}
	else
	{
		if (!InternalParseElement(pXml, parentId, parentW, parentH, x, y, maxW, maxH))
		{
			SDL_Log("Failed to parse UI Elements!");
			return false;
		}
	}

	return true;
}

// ------------------------------------------------------- //
// Parse stack info and create stack element
//
// Params:
// - pXml: The parsing element
// - parentId
// - parentW: the width of parent
// - parentH: the height of parent
// - x: the x position of the children
// - y: the y position of the children
// - maxW: the max width among all children
// - maxH: the max height among all children
// ------------------------------------------------------- //
bool UIManager::ParseStack(const tinyxml2::XMLElement* pXml, int parentId, float parentW, float parentH, int& x, int& y, int& maxW, int& maxH)
{
	// create child UIElement of the stack and set default layout and set width and height default to 100%
	int childId = AddChild(parentId, std::make_unique<UIElement>());
	auto& pElement = m_elements[childId];
	auto& pParent = m_elements[parentId];

	pElement->m_childLayout = UIElement::LayoutType::kStackVertical;
	pElement->m_elementType = UIElement::ElementType::kStack;
	pElement->m_positionRect = pParent->m_positionRect;
	pElement->m_positionRect.x = x;
	pElement->m_positionRect.y = y;
	pElement->m_width = Coordinate(100.f);
	pElement->m_height = Coordinate(100.f);

	// retrieve all the optional data if any
	if (const auto* pAttribute = pXml->FindAttribute("orientation"); pAttribute && std::strcmp(pAttribute->Value(), "horizontal") == 0)
		pElement->m_childLayout = UIElement::LayoutType::kStackHorizontal;

	if (const auto* pAttribute = pXml->FindAttribute("horizontalGap"))
		pElement->m_horizontalLayoutGap = pAttribute->IntValue();

	if (const auto* pAttribute = pXml->FindAttribute("verticalGap"))
		pElement->m_verticalLayoutGap = pAttribute->IntValue();

	// parse common attribute
	ParseCommonAttributes(pXml, pElement, parentW, parentH);

	// parse special attribute
	ParseSpecialAttribute(pXml, parentId, childId);

	// parse children elements of the stack here, for easy recalculate element dimension later
	if (!InternalRecursivelyLoadXml(pXml, childId, parentW, parentH))
		return false;

	// resolve child width and height
	int spaceW = static_cast<int>(pElement->m_width.Resolve(parentW));
	int spaceH = static_cast<int>(pElement->m_height.Resolve(parentH));

	// determine the actual width and height of the element
	int actualW = spaceW == 0 ? pParent->m_positionRect.w : spaceW;
	int actualH = spaceH == 0 ? pParent->m_positionRect.h : spaceH;

	// update max width and height among current children group of the parent
	if (actualW > maxW)
		maxW = actualW;
	if (actualH > maxH)
		maxH = actualH;

	// adjust child element according to parent layout
	if (pParent->m_childLayout != UIElement::LayoutType::kNone)
		AdjustUILayout(parentId, childId, x, y, actualW, actualH, maxW, maxH);
	else
	{
		pElement->m_positionRect.w = actualW;
		pElement->m_positionRect.h = actualH;
	}

	if (spaceW == 0)
		pElement->m_width.m_value = actualW;
	if (spaceH == 0)
		pElement->m_height.m_value = actualH;

	return true;
}

// ------------------------------------------------------- //
// Parse wrap info and create wrap element
//
// Params:
// - pXml: The parsing element
// - parentId
// - parentW: the width of parent
// - parentH: the height of parent
// - x: the x position of the children
// - y: the y position of the children
// - maxW: the max width among all children
// - maxH: the max height among all children
// ------------------------------------------------------- //
bool UIManager::ParseWrap(const tinyxml2::XMLElement* pXml, int parentId, float parentW, float parentH, int& x, int& y, int& maxW, int& maxH)
{
	// create child UIElement of the wrap and set default layout and set width and height default to 100%
	int childId = AddChild(parentId, std::make_unique<UIElement>());
	auto& pElement = m_elements[childId];
	auto& pParent = m_elements[parentId];

	pElement->m_childLayout = UIElement::LayoutType::kWrapHorizontal;
	pElement->m_elementType = UIElement::ElementType::kWrap;
	pElement->m_positionRect = pParent->m_positionRect;
	pElement->m_positionRect.x = x;
	pElement->m_positionRect.y = y;
	pElement->m_width = Coordinate(100.f);
	pElement->m_height = Coordinate(100.f);

	// retrieve all the optional data if any
	if (const auto* pAttribute = pXml->FindAttribute("orientation"); pAttribute && std::strcmp(pAttribute->Value(), "vertical") == 0)
		pElement->m_childLayout = UIElement::LayoutType::kWrapVertical;

	if (const auto* pAttribute = pXml->FindAttribute("horizontalGap"))
		pElement->m_horizontalLayoutGap = pAttribute->IntValue();

	if (const auto* pAttribute = pXml->FindAttribute("verticalGap"))
		pElement->m_verticalLayoutGap = pAttribute->IntValue();

	// parse common attribute
	ParseCommonAttributes(pXml, pElement, parentW, parentH);

	// parse special attribute
	ParseSpecialAttribute(pXml, parentId, childId);

	// parse children elements of the stack here, for easy recalculate element dimension later
	if (!InternalRecursivelyLoadXml(pXml, childId, parentW, parentH))
		return false;

	// resolve child width and height
	int spaceW = static_cast<int>(pElement->m_width.Resolve(parentW));
	int spaceH = static_cast<int>(pElement->m_height.Resolve(parentH));

	// determine the actual width and height of the element
	int actualW = spaceW == 0 ? pParent->m_positionRect.w : spaceW;
	int actualH = spaceH == 0 ? pParent->m_positionRect.h : spaceH;

	// update max width and height among current children group of the parent
	if (actualW > maxW)
		maxW = actualW;
	if (actualH > maxH)
		maxH = actualH;

	// adjust child element according to parent layout
	if (pParent->m_childLayout != UIElement::LayoutType::kNone)
		AdjustUILayout(parentId, childId, x, y, actualW, actualH, maxW, maxH);
	else
	{
		pElement->m_positionRect.w = actualW;
		pElement->m_positionRect.h = actualH;
	}

	if (spaceW == 0)
		pElement->m_width.m_value = actualW;
	if (spaceH == 0)
		pElement->m_height.m_value = actualH;

	return true;
}

// ------------------------------------------------------- //
// Parse image info and create image element
//
// Params:
// - pXml: The parsing element
// - parentId
// - parentW: the width of parent
// - parentH: the height of parent
// - x: the x position of the children
// - y: the y position of the children
// - maxW: the max width among all children
// - maxH: the max height among all children
// ------------------------------------------------------- //
bool UIManager::ParseImage(const tinyxml2::XMLElement* pXml, int parentId, float parentW, float parentH, int& x, int& y, int& maxW, int& maxH)
{
	ImageData data;
	data.m_source = pXml->Attribute("source");

	if (const auto* pAttribute = pXml->FindAttribute("slices"))
	{
		data.m_type = ImageType::k9Slices;
		std::string slicesValue = pAttribute->Value();
		size_t start = 0;

		for (int& slice : data.m_slices)
		{
			size_t end = slicesValue.find_first_of(',', start);
			if (end != std::string::npos)
			{
				slice = std::stoi(slicesValue.substr(start, end - start));
				start = end + 1;
			}
			else
				slice = std::stoi(slicesValue.substr(start));
		}
	}

	// create textfield within the given width and height
	SDL_Texture* pTexture = UIApplication::Get()->GetResourceLoader().CreateImageTexture(data);

	if (!pTexture)
	{
		SDL_Log("Failed to create image texture!");
		return false;
	}

	// create child UIElement as UIImage
	int childId = AddChild(parentId, std::make_unique<UIImage>(data, pTexture));
	auto& pElement = m_elements[childId];

	// parse common attribute
	ParseCommonAttributes(pXml, pElement, parentW, parentH);

	// parse special attribute
	ParseSpecialAttribute(pXml, parentId, childId);

	// get texture original width and height
	int textureW, textureH;
	SDL_QueryTexture(pTexture, nullptr, nullptr, &textureW, &textureH);

	// resolve child width and height
	int spaceW = static_cast<int>(pElement->m_width.Resolve(parentW));
	int spaceH = static_cast<int>(pElement->m_height.Resolve(parentH));

	// determine the actual width and height of the element
	int actualW = spaceW == 0 ? textureW : spaceW;
	int actualH = spaceH == 0 ? textureH : spaceH;

	// update max width and height among current children group of the parent
	if (actualW > maxW)
		maxW = actualW;
	if (actualH > maxH)
		maxH = actualH;

	// adjust child element according to parent layout
	AdjustUILayout(parentId, childId, x, y, actualW, actualH, maxW, maxH);

	if (spaceW == 0)
		pElement->m_width.m_value = actualW;
	if (spaceH == 0)
		pElement->m_height.m_value = actualH;

	return InternalRecursivelyLoadXml(pXml, childId, parentW, parentH);
}

// ------------------------------------------------------- //
// Parse label info and create label element
//
// Params:
// - pXml: The parsing element
// - parentId
// - parentW: the width of parent
// - parentH: the height of parent
// - x: the x position of the children
// - y: the y position of the children
// - maxW: the max width among all children
// - maxH: the max height among all children
// ------------------------------------------------------- //
bool UIManager::ParseLabel(const tinyxml2::XMLElement* pXml, int parentId, float parentW, float parentH, int& x, int& y, int& maxW, int& maxH)
{
	TextfieldData data;
	// get textfield text
	data.m_text = pXml->Attribute("text");
	
	// retrieve all the optional data if any
	if (pXml->FindAttribute("width"))
		data.m_flexSize = false;

	if (pXml->FindAttribute("height"))
		data.m_flexSize = false;

	if (const auto* pAttribute = pXml->FindAttribute("fontSize"))
		data.m_fontSize = pAttribute->IntValue();

	if (const auto* pHAlign = pXml->FindAttribute("halign"))
		data.m_hAlign = pHAlign->Value();

	if (const auto* pVAlign = pXml->FindAttribute("valign"))
		data.m_vAlign = pVAlign->Value();

	if (const auto* pFont = pXml->FindAttribute("font"))
		data.m_fontName = pFont->Value();

	// binding variables
	auto bindingVariables = QueryTextBindingVariables(data.m_text);

	// create child UIElement as UITextfield
	int childId = AddChild(parentId, std::make_unique<UITextfield>(data));
	auto* pElement = dynamic_cast<UITextfield*>(m_elements[childId].get());
	auto& pParent = m_elements[parentId];

	// parse common attribute
	ParseCommonAttributes(pXml, m_elements[childId], parentW, parentH);

	pElement->m_bindingTexts.resize(bindingVariables.size());
	for (size_t i = 0; i < bindingVariables.size(); ++i)
	{
		auto& prop = GetOrCreateNewUIProperty(bindingVariables[i]);
		auto& child = pElement->m_bindingTexts[i];
		prop.Bind(child);
	}

	pElement->AddBindingTextsRefreshingEvents();
	pElement->RefreshTexture();

	// parse special attribute
	ParseSpecialAttribute(pXml, parentId, childId);

	// resolve child width and height
	int actualW = static_cast<int>(pElement->m_width.Resolve(parentW));
	int actualH = static_cast<int>(pElement->m_height.Resolve(parentH));

	// update max width and height among current children group of the parent
	if (actualW > maxW)
		maxW = actualW;
	if (actualH > maxH)
		maxH = actualH;

	// adjust child element according to parent layout
	if (pParent->m_childLayout != UIElement::LayoutType::kNone)
	{
		AdjustUILayout(parentId, childId, x, y, actualW, actualH, maxW, maxH);
	}
	else
	{
		pElement->m_positionRect = pParent->m_positionRect;
		pElement->m_positionRect.w = actualW;
		pElement->m_positionRect.h = actualH;
	}

	return InternalRecursivelyLoadXml(pXml, childId, parentW, parentH);
}

// ------------------------------------------------------- //
// Parse button info and create button element
//
// Params:
// - pXml: The parsing element
// - parentId
// - parentW: the width of parent
// - parentH: the height of parent
// - x: the x position of the children
// - y: the y position of the children
// - maxW: the max width among all children
// - maxH: the max height among all children
// ------------------------------------------------------- //
bool UIManager::ParseButton(const tinyxml2::XMLElement* pXml, int parentId, float parentW, float parentH, int& x, int& y, int& maxW, int& maxH)
{
	// create child UIElement of the stack and set default layout and set width and height default to 100%
	int childId = AddChild(parentId, std::make_unique<UIButton>());
	auto& pElement = m_elements[childId];
	auto& pParent = m_elements[parentId];
	pElement->m_width = Coordinate(100.f);
	pElement->m_height = Coordinate(100.f);

	// parse common attribute
	ParseCommonAttributes(pXml, pElement, parentW, parentH);

	// parse special attribute
	ParseSpecialAttribute(pXml, parentId, childId);

	// resolve child width and height
	int spaceW = static_cast<int>(pElement->m_width.Resolve(parentW));
	int spaceH = static_cast<int>(pElement->m_height.Resolve(parentH));

	// determine the actual width and height of the element
	int actualW = spaceW == 0 ? pParent->m_positionRect.w : spaceW;
	int actualH = spaceH == 0 ? pParent->m_positionRect.h : spaceH;

	// update max width and height among current children group of the parent
	if (actualW > maxW)
		maxW = actualW;
	if (actualH > maxH)
		maxH = actualH;

	// adjust child element according to parent layout
	if (pParent->m_childLayout != UIElement::LayoutType::kNone)
	{
		AdjustUILayout(parentId, childId, x, y, actualW, actualH, maxW, maxH);
	}
	else
	{
		pElement->m_positionRect = pParent->m_positionRect;
		pElement->m_positionRect.w = actualW;
		pElement->m_positionRect.h = actualH;
	}

	if (spaceW == 0)
		pElement->m_width.m_value = actualW;
	if (spaceH == 0)
		pElement->m_height.m_value = actualH;

	return InternalRecursivelyLoadXml(pXml, childId, parentW, parentH);
}

// ------------------------------------------------------- //
// Parse RectShape info and create RectShape element
//
// Params:
// - pXml: The parsing element
// - parentId
// - parentW: the width of parent
// - parentH: the height of parent
// - x: the x position of the children
// - y: the y position of the children
// - maxW: the max width among all children
// - maxH: the max height among all children
// ------------------------------------------------------- //
bool UIManager::ParseRect(const tinyxml2::XMLElement* pXml, int parentId, float parentW, float parentH, int& x, int& y, int& maxW, int& maxH)
{
	// create child UIElement of the stack and set default layout and set width and height default to 100%
	int childId = AddChild(parentId, std::make_unique<UIRectShape>());
	auto* pElement = dynamic_cast<UIRectShape*>(m_elements[childId].get());
	auto& pParent = m_elements[parentId];
	pElement->m_width = Coordinate(100.f);
	pElement->m_height = Coordinate(100.f);

	// retrieve all the optional data if any
	if (const auto* pAttribute = pXml->FindAttribute("filled"))
		pElement->m_filled = pAttribute->BoolValue();

	if (const auto* pAttribute = pXml->FindAttribute("r"))
		ParseIntegerPropertyData(pAttribute, pElement->m_r, nullptr);

	if (const auto* pAttribute = pXml->FindAttribute("g"))
		ParseIntegerPropertyData(pAttribute, pElement->m_g, nullptr);

	if (const auto* pAttribute = pXml->FindAttribute("b"))
		ParseIntegerPropertyData(pAttribute, pElement->m_b, nullptr);

	// parse common attribute
	ParseCommonAttributes(pXml, m_elements[childId], parentW, parentH);
	
	// parse special attribute
	ParseSpecialAttribute(pXml, parentId, childId);

	// resolve child width and height
	int spaceW = static_cast<int>(pElement->m_width.Resolve(parentW));
	int spaceH = static_cast<int>(pElement->m_height.Resolve(parentH));

	// determine the actual width and height of the element
	int actualW = spaceW == 0 ? pParent->m_positionRect.w : spaceW;
	int actualH = spaceH == 0 ? pParent->m_positionRect.h : spaceH;

	// update max width and height among current children group of the parent
	if (actualW > maxW)
		maxW = actualW;
	if (actualH > maxH)
		maxH = actualH;

	// adjust child element according to parent layout
	if (pParent->m_childLayout != UIElement::LayoutType::kNone)
	{
		AdjustUILayout(parentId, childId, x, y, actualW, actualH, maxW, maxH);
	}
	else
	{
		pElement->m_positionRect = pParent->m_positionRect;
		pElement->m_positionRect.w = actualW;
		pElement->m_positionRect.h = actualH;
	}

	if (spaceW == 0)
		pElement->m_width.m_value = actualW;
	if (spaceH == 0)
		pElement->m_height.m_value = actualH;

	return InternalRecursivelyLoadXml(pXml, childId, parentW, parentH);
}

// ------------------------------------------------------- //
// Parse Repeat info
//
// The repeat info doesn't create a UI element, it works as
// repeatedly creating it's children elements according
// to the 'count'.
//
// Params:
// - pXml: The parsing element
// - parentId
// - parentW: the width of parent
// - parentH: the height of parent
// - x: the x position of the children
// - y: the y position of the children
// - maxW: the max width among all children
// - maxH: the max height among all children
// ------------------------------------------------------- //
bool UIManager::ParseRepeat(const tinyxml2::XMLElement* pXml, int parentId, float parentW, float parentH, int& x, int& y, int& maxW, int& maxH)
{
	int childId = AddChild(parentId, std::make_unique<UIRepeat>());
	auto* pElement = dynamic_cast<UIRepeat*>(m_elements[childId].get());
	auto& pParent = m_elements[parentId];

	pElement->m_childLayout = pParent->m_childLayout;
	pElement->m_horizontalLayoutGap = pParent->m_horizontalLayoutGap;
	pElement->m_verticalLayoutGap = pParent->m_verticalLayoutGap;
	pElement->m_positionRect.x = x;
	pElement->m_positionRect.y = y;
	pElement->m_positionRect.w = static_cast<int>(parentW);
	pElement->m_positionRect.h = static_cast<int>(parentH);
	pElement->m_width = Coordinate(100.f);
	pElement->m_height = Coordinate(100.f);

	ParseIntegerPropertyData(pXml->FindAttribute("count"), pElement->m_repeatCount, &pElement->m_inputControllable);

	// parse common attribute
	ParseCommonAttributes(pXml, m_elements[childId], parentW, parentH);

	// parse special attribute
	ParseSpecialAttribute(pXml, parentId, childId);

	pElement->m_positionRect.w -= pParent->m_horizontalLayoutGap * 2;
	pElement->m_positionRect.h -= pParent->m_verticalLayoutGap * 2;
	pElement->m_width = Coordinate(static_cast<float>(pElement->m_positionRect.w) / parentW * 100.f);
	pElement->m_height = Coordinate(static_cast<float>(pElement->m_positionRect.h) / parentH * 100.f);

	// parse children elements of the stack here, for easy recalculate element dimension later
	if (!InternalRecursivelyLoadXml(pXml, childId, parentW, parentH))
		return false;

	// resolve child width and height
	int spaceW = static_cast<int>(pElement->m_width.Resolve(parentW));
	int spaceH = static_cast<int>(pElement->m_height.Resolve(parentH));

	// update max width and height among current children group of the parent
	if (spaceW > maxW)
		maxW = spaceW;
	if (spaceH > maxH)
		maxH = spaceH;

	// adjust child element according to parent layout
	if (pParent->m_childLayout != UIElement::LayoutType::kNone)
	{
		AdjustUILayout(parentId, childId, x, y, spaceW, spaceH, maxW, maxH);
	}
	else
	{
		pElement->m_positionRect.w = spaceW;
		pElement->m_positionRect.h = spaceH;
	}

	return true;
}

// ------------------------------------------------------- //
// Parse Mask info and create Mask element
//
// Params:
// - pXml: The parsing element
// - parentId
// - parentW: the width of parent
// - parentH: the height of parent
// - x: the x position of the children
// - y: the y position of the children
// - maxW: the max width among all children
// - maxH: the max height among all children
// ------------------------------------------------------- //
bool UIManager::ParseMask(const tinyxml2::XMLElement* pXml, int parentId, float parentW, float parentH, int& x, int& y, int& maxW, int& maxH)
{
	// create child UIElement of the stack and set default layout and set width and height default to 100%
	int childId = AddChild(parentId, std::make_unique<UIElement>());
	auto& pElement = m_elements[childId];
	auto& pParent = m_elements[parentId];

	pElement->m_elementType = UIElement::ElementType::kMask;
	pElement->m_childLayout = pParent->m_childLayout;
	pElement->m_width = Coordinate(100.f);
	pElement->m_height = Coordinate(100.f);

	// parse common attribute
	ParseCommonAttributes(pXml, pElement, parentW, parentH);

	// parse special attribute
	ParseSpecialAttribute(pXml, parentId, childId);

	// resolve child width and height
	int spaceW = static_cast<int>(pElement->m_width.Resolve(parentW));
	int spaceH = static_cast<int>(pElement->m_height.Resolve(parentH));

	// determine the actual width and height of the element
	int actualW = spaceW == 0 ? pParent->m_positionRect.w : spaceW;
	int actualH = spaceH == 0 ? pParent->m_positionRect.h : spaceH;

	// update max width and height among current children group of the parent
	if (actualW > maxW)
		maxW = actualW;
	if (actualH > maxH)
		maxH = actualH;

	// adjust child element according to parent layout
	if (pParent->m_childLayout != UIElement::LayoutType::kNone)
	{
		AdjustUILayout(parentId, childId, x, y, actualW, actualH, maxW, maxH);
	}
	else
	{
		pElement->m_positionRect = pParent->m_positionRect;
		pElement->m_positionRect.w = actualW;
		pElement->m_positionRect.h = actualH;
	}

	if (spaceW == 0)
		pElement->m_width.m_value = actualW;
	if (spaceH == 0)
		pElement->m_height.m_value = actualH;

	return InternalRecursivelyLoadXml(pXml, childId, parentW, parentH);
}

// ------------------------------------------------------- //
// Parse Scroll info and create Scroll element
//
// Params:
// - pXml: The parsing element
// - parentId
// - parentW: the width of parent
// - parentH: the height of parent
// - x: the x position of the children
// - y: the y position of the children
// - maxW: the max width among all children
// - maxH: the max height among all children
// ------------------------------------------------------- //
bool UIManager::ParseScroll(const tinyxml2::XMLElement* pXml, int parentId, float parentW, float parentH, int& x, int& y, int& maxW, int& maxH)
{
	// create child UIElement of the wrap and set default layout and set width and height default to 100%
	int childId = AddChild(parentId, std::make_unique<UIScroll>());
	auto& pElement = m_elements[childId];
	auto& pParent = m_elements[parentId];

	pElement->m_width = Coordinate(100.f);
	pElement->m_height = Coordinate(100.f);

	// retrieve all the optional data if any
	if (const auto* pAttribute = pXml->FindAttribute("orientation"); pAttribute && std::strcmp(pAttribute->Value(), "horizontal") == 0)
		pElement->m_childLayout = UIElement::LayoutType::kScrollHorizontal;

	// parse common attribute
	ParseCommonAttributes(pXml, pElement, parentW, parentH);

	// parse special attribute
	ParseSpecialAttribute(pXml, parentId, childId);

	// resolve child width and height
	int spaceW = static_cast<int>(pElement->m_width.Resolve(parentW));
	int spaceH = static_cast<int>(pElement->m_height.Resolve(parentH));

	// determine the actual width and height of the element
	int actualW = spaceW == 0 ? pParent->m_positionRect.w : spaceW;
	int actualH = spaceH == 0 ? pParent->m_positionRect.h : spaceH;

	// update max width and height among current children group of the parent
	if (actualW > maxW)
		maxW = actualW;
	if (actualH > maxH)
		maxH = actualH;

	// adjust child element according to parent layout
	if (pParent->m_childLayout != UIElement::LayoutType::kNone)
	{
		AdjustUILayout(parentId, childId, x, y, actualW, actualH, maxW, maxH);
	}
	else
	{
		pElement->m_positionRect = pParent->m_positionRect;
		pElement->m_positionRect.w = actualW;
		pElement->m_positionRect.h = actualH;
	}

	if (spaceW == 0)
		pElement->m_width.m_value = actualW;
	if (spaceH == 0)
		pElement->m_height.m_value = actualH;

	return InternalRecursivelyLoadXml(pXml, childId, parentW, parentH);
}

bool UIManager::ParseInput(const tinyxml2::XMLElement* pXml, int parentId, float parentW, float parentH, int& x, int& y, int& maxW, int& maxH)
{
	InputData data;

	if (const auto* pAttribute = pXml->FindAttribute("placeholder"))
		data.m_placeHolder = pAttribute->Value();

	if (const auto* pAttribute = pXml->FindAttribute("inputType"))
		data.m_inputType = pAttribute->Value();
	
	if (const auto* pAttribute = pXml->FindAttribute("font"))
		data.m_font = pAttribute->Value();

	if (const auto* pAttribute = pXml->FindAttribute("fontSize"))
		data.m_fontSize = pAttribute->IntValue();

	if (const auto* pAttribute = pXml->FindAttribute("max"))
		data.m_max = pAttribute->IntValue();

	// create child UIElement of the stack and set default layout and set width and height default to 100%
	int childId = AddChild(parentId, std::make_unique<UIInput>(data));
	auto& pElement = m_elements[childId];
	auto& pParent = m_elements[parentId];

	pElement->m_width = Coordinate(100.f);

	// retrieve all the optional data if any
	if (const auto* pAttribute = pXml->FindAttribute("text"))
	{
		auto* pInput = dynamic_cast<UIInput*>(pElement.get());

		std::string str = pAttribute->Value();
		if (auto vec = QueryTextBindingVariables(str); !vec.empty())
		{
			auto& prop = GetOrCreateNewUIProperty(vec[0]);
			UIProperty::DoubleBind(prop, pInput->Text());
		}
		else
		{
			pInput->Text() = str;
		}
	}

	// parse common attribute
	ParseCommonAttributes(pXml, pElement, parentW, parentH);

	// parse special attribute
	ParseSpecialAttribute(pXml, parentId, childId);

	// resolve child width and height
	int spaceW = static_cast<int>(pElement->m_width.Resolve(parentW));
	int spaceH = static_cast<int>(pElement->m_height.Resolve(parentH));

	// determine the actual width and height of the element
	int actualW = spaceW == 0 ? pParent->m_positionRect.w : spaceW;
	int actualH = spaceH == 0 ? pParent->m_positionRect.h : spaceH;

	// update max width and height among current children group of the parent
	if (actualW > maxW)
		maxW = actualW;
	if (actualH > maxH)
		maxH = actualH;

	// adjust child element according to parent layout
	if (pParent->m_childLayout != UIElement::LayoutType::kNone)
	{
		AdjustUILayout(parentId, childId, x, y, actualW, actualH, maxW, maxH);
	}
	else
	{
		pElement->m_positionRect = pParent->m_positionRect;
		pElement->m_positionRect.w = actualW;
		pElement->m_positionRect.h = actualH;
	}

	if (spaceW == 0)
		pElement->m_width.m_value = actualW;
	if (spaceH == 0)
		pElement->m_height.m_value = actualH;

	return InternalRecursivelyLoadXml(pXml, childId, parentW, parentH);
}

bool UIManager::ParseData(const tinyxml2::XMLElement* pXml, int parentId)
{
	std::string variableName = pXml->Attribute("variable");
	UIProperty value = CreateUIPropertyFromData(pXml->FindAttribute("value"));

	if (const auto* pAttribute = pXml->FindAttribute("button:on"); pAttribute && std::strcmp(pAttribute->Value(), "clicked") == 0)
		return ParseDataButtonState(parentId, variableName, value);

	m_dataMap[variableName] = value;
	return true;
}

// ------------------------------------------------------- //
// Parse dimension data with struct type of Coordinate
// ------------------------------------------------------- //
void UIManager::ParseDimensionData(const tinyxml2::XMLAttribute* pAttribute, Coordinate& coord)
{
	std::string data = pAttribute->Value();
	size_t len = data.size();
	if (data[len - 1] == '%')
	{
		data.pop_back();
		coord = Coordinate(std::stof(data));
		return;
	}
	else if (data[len - 1] == '}')
	{
		auto& prop = GetOrCreateNewUIProperty(data.substr(1, len - 2));
		const auto* pVal = prop.Get<float>();
		coord.m_type = pVal ? Coordinate::Type::kPercentage : coord.m_type;
		prop.Bind(coord.m_value);
		return;
	}

	coord = Coordinate(std::stoi(data));
}

void UIManager::ParseIntegerPropertyData(const tinyxml2::XMLAttribute* pAttribute, UIProperty& val, bool* inputControl)
{
	std::string data = pAttribute->Value();
	size_t len = data.size();
	if (data[len - 1] == '}')
	{
		auto& prop = GetOrCreateNewUIProperty(data.substr(1, len - 2));
		
		if (prop.CurrentHoldingType() != UIProperty::Type::kInt)
			prop.CastToType(UIProperty::Type::kInt);

		prop.Bind(val);

		if (inputControl)
			*inputControl = true;
		return;
	}

	if (inputControl)
		*inputControl = false;

	val = UIProperty(std::stoi(data));
}

// ------------------------------------------------------- //
// Parse UI Button State for the child image and textfield
// ------------------------------------------------------- //
void UIManager::ParseSpecialAttribute(const tinyxml2::XMLElement* pXml, int parentId, int childId)
{
	ParseButtonState(pXml, parentId, childId);
	ParseFocusState(pXml, parentId, childId);
}

void UIManager::ParseButtonState(const tinyxml2::XMLElement* pXml, int parentId, int childId)
{
	if (m_elements[parentId]->m_elementType != UIElement::ElementType::kButton)
		return;

	auto* pParentElement = dynamic_cast<UIButton*>(m_elements[parentId].get());

	// if found specified button states, apply them
	if (const auto* pAttribute = pXml->FindAttribute("button:states"))
	{
		std::string state = pAttribute->Value();

		if (state[0] == '{')
		{
			auto& prop = GetOrCreateNewUIProperty(state.substr(1, state.size() - 2));
			prop.Bind(m_elements[childId]->m_buttonState);
			pParentElement->m_stateChildIndices[static_cast<int>(ButtonState::kUndefined)].emplace_back(childId);
		}
		else
		{
			m_elements[childId]->m_buttonState = state;
		}
	}
	// otherwise apply to all
	else
	{
		m_elements[childId]->m_buttonState = "up,down,over";
	}
}

void UIManager::ParseFocusState(const tinyxml2::XMLElement* pXml, int parentId, int childId)
{
	if (m_elements[parentId]->m_elementType != UIElement::ElementType::kInput)
		return;

	auto* pParentElement = dynamic_cast<UIInput*>(m_elements[parentId].get());

	// if found specified button states, apply them
	if (const auto* pAttribute = pXml->FindAttribute("input:focused"))
	{
		bool state = pAttribute->BoolValue();
		pParentElement->m_stateChildIndices[state ? (int)FocusState::kFocused : (int)FocusState::kNotFocused].emplace_back(childId);
	}
	// otherwise apply to all
	else
	{
		pParentElement->m_stateChildIndices[(int)FocusState::kFocused].emplace_back(childId);
		pParentElement->m_stateChildIndices[(int)FocusState::kNotFocused].emplace_back(childId);
	}
}

bool UIManager::ParseDataButtonState(int parentId, const std::string& vName, UIProperty& value)
{
	if (m_elements[parentId]->m_elementType != UIElement::ElementType::kButton)
		return false;

	auto* pParent = dynamic_cast<UIButton*>(m_elements[parentId].get());
	pParent->m_onVariableName = vName;
	pParent->m_onValue = value;

	return true;
}

void UIManager::ParseCommonAttributes(const tinyxml2::XMLElement* pXml, SharedUIElement& pElement, float parentW, float parentH)
{
	if (const auto* pAttribute = pXml->FindAttribute("width"))
	{
		ParseDimensionData(pAttribute, pElement->m_width);

		//if (pElement->m_elementType == UIElement::ElementType::kStack || pElement->m_elementType == UIElement::ElementType::kWrap)
		pElement->m_positionRect.w = (int)pElement->m_width.Resolve(parentW);
	}

	if (const auto* pAttribute = pXml->FindAttribute("height"))
	{
		ParseDimensionData(pAttribute, pElement->m_height);

		//if (pElement->m_elementType == UIElement::ElementType::kStack || pElement->m_elementType == UIElement::ElementType::kWrap)
		pElement->m_positionRect.h = static_cast<int>(pElement->m_height.Resolve(parentH));
	}

	if (const auto* pAttribute = pXml->FindAttribute("flexSize"))
		pElement->m_flexSize = pAttribute->BoolValue();

	if (const auto* pAttribute = pXml->FindAttribute("visible"))
		pElement->m_visible = pAttribute->BoolValue();

	if (const auto* pAttribute = pXml->FindAttribute("tag"))
		pElement->m_tag = pAttribute->Value();
}

// ------------------------------------------------------- //
// Adjust child UI position according to specific layout
// ------------------------------------------------------- //
void UIManager::AdjustUILayout(int parentId, int id, int& x, int& y, int w, int h, int& maxW, int& maxH)
{
	SharedUIElement& pParent = m_elements[parentId];
	SharedUIElement& pElement = m_elements[id];

	UIElement::LayoutType layout = pParent->m_childLayout;
	UIElement::ElementType type = pParent->m_elementType;
	SDL_Rect parentRect = pParent->m_positionRect;
	int horizontalGap = pParent->m_horizontalLayoutGap;
	int verticalGap = pParent->m_verticalLayoutGap;

	// adjust line if parent has a wrap layout
	if (layout == UIElement::LayoutType::kWrapHorizontal && x + w > parentRect.x + parentRect.w)
	{
		x = type == UIElement::ElementType::kRepeat ? parentRect.x : parentRect.x + horizontalGap;
		y += maxH + verticalGap;
		maxH = h;
	}
	else if (layout == UIElement::LayoutType::kWrapVertical && y + h > parentRect.y + parentRect.h)
	{
		y = type == UIElement::ElementType::kRepeat ? parentRect.y : parentRect.y + verticalGap;
		x += maxW + horizontalGap;
		maxW = w;
	}

	// adjust position of the child element
	pElement->m_positionRect.x = x;
	pElement->m_positionRect.y = y;
	pElement->m_positionRect.w = w;
	pElement->m_positionRect.h = h;

	// increment the x or y by child's width and height
	if (layout == UIElement::LayoutType::kStackHorizontal || layout == UIElement::LayoutType::kWrapHorizontal)
		x += w + horizontalGap;
	else if (layout == UIElement::LayoutType::kStackVertical || layout == UIElement::LayoutType::kWrapVertical)
		y += h + verticalGap;
}

void UIManager::AdjustStackWrapDimension(UIElement::LayoutType layout, int elementId, int x, int y, int maxW, int maxH)
{
	SharedUIElement& pElement = m_elements[elementId];

	if (layout == UIElement::LayoutType::kWrapHorizontal || layout == UIElement::LayoutType::kStackHorizontal)
	{
		pElement->m_height = Coordinate(y + maxH + pElement->m_verticalLayoutGap - pElement->m_positionRect.y);
	}
	else if (layout == UIElement::LayoutType::kWrapVertical || layout == UIElement::LayoutType::kStackVertical)
	{
		pElement->m_width = Coordinate(x + maxW + pElement->m_horizontalLayoutGap - pElement->m_positionRect.x);
	}
}

void UIManager::AdjustRepeatElementDimension(UIElement::LayoutType layout, int elementId, int x, int y, int maxW, int maxH)
{
	SharedUIElement& pElement = m_elements[elementId];

	if (layout == UIElement::LayoutType::kWrapHorizontal || layout == UIElement::LayoutType::kStackHorizontal)
	{
		pElement->m_height = Coordinate(y + maxH - pElement->m_positionRect.y);
	}
	else if (layout == UIElement::LayoutType::kWrapVertical || layout == UIElement::LayoutType::kStackVertical)
	{
		pElement->m_width = Coordinate(x + maxW - pElement->m_positionRect.x);
	}
}

// ------------------------------------------------------- //
// Recursively adjust child UI position in reaction to 
// mouse scroll value
// ------------------------------------------------------- //
void UIManager::RepositionScrollLayout(UIElement::LayoutType layout, int elementId)
{
	for (int child : m_childrenMap[elementId])
	{
		if (layout == UIElement::LayoutType::kScrollHorizontal)
			m_elements[child]->m_positionRect.x += Mouse::Get().GetWheelValue();
		else if (layout == UIElement::LayoutType::kScrollVertical)
			m_elements[child]->m_positionRect.y += Mouse::Get().GetWheelValue();

		RepositionScrollLayout(layout, child);
	}
}

// ------------------------------------------------------- //
// Recursively test and update UI elements in reaction to 
// mouse position and click
// ------------------------------------------------------- //
bool UIManager::InternalUpdateMouseEvent(int elementId, int& newFocus)
{
	bool mouseOverChild = false;

	for (int child : m_childrenMap[elementId])
	{
		UIElement::ElementType type = m_elements[child]->m_elementType;

		if (InternalUpdateMouseEvent(child, newFocus) && type == UIElement::ElementType::kButton)
		{
			dynamic_cast<UIButton*>(m_elements[child].get())->m_state = ButtonState::kUp;
			continue;
		}

		if (m_elements[child]->HitTest() && newFocus == -1)
		{
			if (m_elements[child]->CheckMouseClick() == (int)Mouse::MouseButton::kButtonLeft &&
				type == UIElement::ElementType::kInput)
			{
				newFocus = child;
			}

			mouseOverChild = type == UIElement::ElementType::kButton ? true : false;
		}
		else if (type == UIElement::ElementType::kButton)
			dynamic_cast<UIButton*>(m_elements[child].get())->m_state = ButtonState::kUp;
	}

	return mouseOverChild;
}

// ------------------------------------------------------- //
// Recursively test and update UI elements in reaction to 
// mouse scrolling value. The scroll effect will only take
// effects on the deepest scroll element and it's children.
// So once we found the deepest scroll element, we apply
// position adjustment to its children and exit.
// ------------------------------------------------------- //
bool UIManager::InternalUpdateUIMouseScroll(int elementId)
{
	for (int child : m_childrenMap[elementId])
	{
		if (InternalUpdateUIMouseScroll(child))
		{
			return true;
		}
		else if (m_elements[child]->m_elementType != UIElement::ElementType::kScroll)
		{
			continue;
		}

		auto* pScroll = dynamic_cast<UIScroll*>(m_elements[child].get());
		if (pScroll->HitTest() && pScroll->OffsetCheck())
		{
			RepositionScrollLayout(m_elements[child]->m_childLayout, child);
			return true;
		}
	}

	return false;
}

// ------------------------------------------------------- //
// Update the boarder of the scroll element
// ------------------------------------------------------- //
void UIManager::InternalUpdateScrollBoarder(int elementId)
{
	int boarder = 0;

	std::queue<int> openSet;
	openSet.emplace(m_kRootId);

	while (!openSet.empty())
	{
		int id = openSet.front();
		openSet.pop();

		for (int child : m_childrenMap[id])
		{
			if (m_elements[elementId]->m_childLayout == UIElement::LayoutType::kScrollHorizontal &&
				m_elements[child]->m_positionRect.x + m_elements[child]->m_positionRect.w > boarder)
			{
				boarder = m_elements[child]->m_positionRect.x + m_elements[child]->m_positionRect.w;
			}
			else if (m_elements[elementId]->m_childLayout == UIElement::LayoutType::kScrollVertical &&
				m_elements[child]->m_positionRect.y + m_elements[child]->m_positionRect.h > boarder)
			{
				boarder = m_elements[child]->m_positionRect.y + m_elements[child]->m_positionRect.h;
			}

			openSet.emplace(child);
		}
	}

	if (m_elements[elementId]->m_childLayout == UIElement::LayoutType::kScrollHorizontal)
		boarder -= m_elements[elementId]->m_positionRect.w;
	else if (m_elements[elementId]->m_childLayout == UIElement::LayoutType::kScrollVertical)
		boarder -= m_elements[elementId]->m_positionRect.h;

	dynamic_cast<UIScroll*>(m_elements[elementId].get())->m_maxScroll = boarder;
}

// ------------------------------------------------------- //
// Check the new focus result, apply change if it's valid
// ------------------------------------------------------- //
void UIManager::CheckFocus(int newFocus)
{	
	if (m_focusElement == newFocus || !Mouse::Get().IsButtonDown(Mouse::MouseButton::kButtonLeft))
		return;

	SDL_Log(("change focus " + std::to_string(newFocus)).c_str());

	if (m_focusElement != -1)
	{
		m_elements[m_focusElement]->LoseFocus();
		SDL_StopTextInput();
	}

	m_focusElement = newFocus;

	if (m_focusElement != -1)
	{
		m_elements[m_focusElement]->GainFocus();
		SDL_StartTextInput();
	}
}

void UIManager::TriggerClickEvents()
{
	if (m_clickElement >= 0 && m_clickElement < (int)m_elements.size() && m_elements[m_clickElement])
	{
		if (auto* pButton = dynamic_cast<UIButton*>(m_elements[m_clickElement].get()))
		{
			pButton->TriggerClickCallbacks();
		}
	}

	m_clickElement = -1;
}

UIProperty UIManager::CreateUIPropertyFromData(const tinyxml2::XMLAttribute* pAttribute) const
{
	std::string str = pAttribute->Value();
	if (!str.empty() && str.back() == '%')
	{
		str.pop_back();
		if (IsInt(str))
			return UIProperty(std::stof(str));
	}

	if (int valInt = 0; pAttribute->QueryIntValue(&valInt) == tinyxml2::XML_SUCCESS)
		return UIProperty(valInt);
	
	if (float valFloat = 0.f; pAttribute->QueryFloatValue(&valFloat) == tinyxml2::XML_SUCCESS)
		return UIProperty(valFloat);
	
	if (bool valBool = true; pAttribute->QueryBoolValue(&valBool) == tinyxml2::XML_SUCCESS)
		return UIProperty(valBool);

	return UIProperty(str);
}

UIProperty& UIManager::GetOrCreateNewUIProperty(const std::string& name)
{
	auto [condition, _] = m_dataMap.emplace(name, UIProperty());
	return condition->second;
}

std::vector<std::string> UIManager::QueryTextBindingVariables(std::string& fixedString) const
{
	std::vector<std::string> result = {};

	while (true)
	{
		size_t pos = fixedString.find_first_of('{');
		if (pos == std::string::npos)
			break;

		size_t end = fixedString.find_first_of('}', pos);
		result.emplace_back(fixedString.substr(pos + 1, end - pos - 1));
		fixedString.replace(pos, end - pos + 1, "%s");
	}

	// if use std::move will prevent compiler copy elision
	return result;
}

std::vector<int>& UIManager::RetrieveElementChildren(int elementId)
{
	if (m_elements[elementId]->m_elementType == UIElement::ElementType::kButton)
	{
		auto* pButton = dynamic_cast<UIButton*>(m_elements[elementId].get());
		return pButton->m_stateChildIndices[(int)pButton->m_state];
	}

	if (m_elements[elementId]->m_elementType == UIElement::ElementType::kInput)
	{
		auto* pInput = dynamic_cast<UIInput*>(m_elements[elementId].get());
		return pInput->m_stateChildIndices[(int)pInput->m_state];
	}

	return m_childrenMap[elementId];
}

// ------------------- //
//	  mask effects
// ------------------- //
void UIManager::CheckMaskEffects(SDL_Renderer* pRenderer, int elementId, std::stack<int>& maskSet)
{
	// if current element is a sibling element of the last mask element, it means the last mask element
	// has done rendering, so we cancel the clip
	if (!maskSet.empty() && m_parentMap[elementId] == maskSet.top())
	{
		SDL_RenderSetClipRect(pRenderer, nullptr);
		maskSet.pop();
	}

	// if current element is a mask element, we set clip and add it's parent id into mask set
	if (m_elements[elementId]->m_elementType == UIElement::ElementType::kMask)
	{
		SDL_RenderSetClipRect(pRenderer, &m_elements[elementId]->m_positionRect);
		maskSet.emplace(m_parentMap[elementId]);
	}
}

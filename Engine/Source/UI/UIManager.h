#pragma once
#include "UI/UIElement/UIElement.h"
#include "UI/UIProperty/UIProperty.h"

#include <SDL/SDL.h>
#include <vector>
#include <stack>
#include <unordered_map>
#include <string>
#include <TinyXml/tinyxml2.h>

struct Coordinate;	// Defined in UIElement.h

class UIManager
{
public:
	using SharedUIElement = std::shared_ptr<UIElement>;

	UIManager();
	~UIManager()						   = default;
	UIManager(const UIManager&)			   = delete;
	UIManager& operator=(const UIManager&) = delete;

	bool Init();
	bool LoadXml(const tinyxml2::XMLElement* pXml);
	bool LoadXml(const char* pFileName, int rootId);
	void DynamicallyLoadRepeatElements(const tinyxml2::XMLElement* pXml, int id, int repeatCount);

	void Update();
	void Render(SDL_Renderer* pRenderer);
	void Rearrange(int newWidth, int newHeight);
	void NotifyTextOnFocusedElement(const char* pText);
	void RegisterClickElement(int id);

	void Reset();
	void Clear();
	void ClearSubtree(int id);
	void ClearAllElementsInChildren(int id);
	void RemoveChildFromParent(int childId, int parentId);

	[[nodiscard]] int GetParentId(int childId) const { return m_parentMap[childId]; }
	UIElement* GetParentOf(int index);
	UIElement* GetElementAt(int index);
	UIElement* GetElementWithTag(const std::string& tag);
	UIElement* GetChildWithTag(int startId, const std::string& tag);
	std::vector<int>& GetChildrenIds(int parentId);
	std::vector<SharedUIElement>& GetSharedElements() { return m_elements; }

	UIProperty* GetData(const std::string& name);
	UIProperty& GetOrCreateNewUIProperty(const std::string& name);

private:
	int FindNextAvailableSpot();
	int AddChild(int parentId, SharedUIElement pChild);

	// functions to parse xml to spawn UIElement, see detailed comment in .cpp
	bool InternalRecursivelyLoadXml(const tinyxml2::XMLElement* pXml, int parentId, float wDimension, float hDimension);
	bool InternalParseElement(const tinyxml2::XMLElement* pXml, int parentId, float parentW, float parentH, int& x, int& y, int& maxW, int& maxH);
	bool InternalLoadChildren(const tinyxml2::XMLElement* pXml, int parentId, float parentW, float parentH, int& x, int& y, int& maxW, int& maxH);

	bool ParseStack(const tinyxml2::XMLElement* pXml, int parentId, float parentW, float parentH, int& x, int& y, int& maxW, int& maxH);
	bool ParseWrap(const tinyxml2::XMLElement* pXml, int parentId, float parentW, float parentH, int& x, int& y, int& maxW, int& maxH);
	bool ParseImage(const tinyxml2::XMLElement* pXml, int parentId, float parentW, float parentH, int& x, int& y, int& maxW, int& maxH);
	bool ParseLabel(const tinyxml2::XMLElement* pXml, int parentId, float parentW, float parentH, int& x, int& y, int& maxW, int& maxH);
	bool ParseButton(const tinyxml2::XMLElement* pXml, int parentId, float parentW, float parentH, int& x, int& y, int& maxW, int& maxH);
	bool ParseRect(const tinyxml2::XMLElement* pXml, int parentId, float parentW, float parentH, int& x, int& y, int& maxW, int& maxH);
	bool ParseRepeat(const tinyxml2::XMLElement* pXml, int parentId, float parentW, float parentH, int& x, int& y, int& maxW, int& maxH);
	bool ParseMask(const tinyxml2::XMLElement* pXml, int parentId, float parentW, float parentH, int& x, int& y, int& maxW, int& maxH);
	bool ParseScroll(const tinyxml2::XMLElement* pXml, int parentId, float parentW, float parentH, int& x, int& y, int& maxW, int& maxH);
	bool ParseInput(const tinyxml2::XMLElement* pXml, int parentId, float parentW, float parentH, int& x, int& y, int& maxW, int& maxH);
	bool ParseData(const tinyxml2::XMLElement* pXml, int parentId);
	
	void ParseDimensionData(const tinyxml2::XMLAttribute* pAttribute, Coordinate& coord);
	void ParseIntegerPropertyData(const tinyxml2::XMLAttribute* pAttribute, UIProperty& val, bool* inputControl);
	void ParseSpecialAttribute(const tinyxml2::XMLElement* pXml, int parentId, int childId);
	void ParseButtonState(const tinyxml2::XMLElement* pXml, int parentId, int childId);
	void ParseFocusState(const tinyxml2::XMLElement* pXml, int parentId, int childId);
	bool ParseDataButtonState(int parentId, const std::string& vName, UIProperty& value);
	void ParseCommonAttributes(const tinyxml2::XMLElement* pXml, SharedUIElement& pElement, float parentW, float parentH);

	// adjust UI element position according to Stack and Wrap layout
	void AdjustUILayout(int parentId, int id, int& x, int& y, int w, int h, int& maxW, int& maxH);
	void AdjustStackWrapDimension(UIElement::LayoutType layout, int elementId, int x, int y, int maxW, int maxH);
	void AdjustRepeatElementDimension(UIElement::LayoutType layout, int elementId, int x, int y, int maxW, int maxH);
	void RepositionScrollLayout(UIElement::LayoutType layout, int elementId);

	// mouse events update
	bool InternalUpdateMouseEvent(int elementId, int& newFocus);
	bool InternalUpdateUIMouseScroll(int elementId);
	void InternalUpdateScrollBoarder(int elementId);
	void CheckFocus(int newFocus);
	void TriggerClickEvents();

	// Data registration
	UIProperty CreateUIPropertyFromData(const tinyxml2::XMLAttribute* pAttribute) const;
	std::vector<std::string> QueryTextBindingVariables(std::string& fixedString) const;

	// Utils
	std::vector<int>& RetrieveElementChildren(int elementId);
	void CheckMaskEffects(SDL_Renderer* pRenderer, int elementId, std::stack<int>& maskSet);

private:
	std::vector<SharedUIElement> m_elements;
	std::vector<std::vector<int>> m_childrenMap;
	std::vector<int> m_parentMap;
	std::vector<int> m_freeIds;
	std::unordered_map<std::string, UIProperty> m_dataMap;

	const int m_kRootId;
	int m_focusElement;
	int m_clickElement;
};

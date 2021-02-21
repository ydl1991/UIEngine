#pragma once
#include "UI/UIElement/UIElement.h"
#include "UI/UIProperty/UIProperty.h"
#include <TinyXml/tinyxml2.h>
#include <memory>

class UIRepeat final : public UIElement
{
public:
	UIRepeat();

	[[nodiscard]] int GetCount() const { return *m_repeatCount.Get<int>(); }
	bool BindRepeatCount(UIProperty& binder);

	UIProperty m_repeatCount;

	bool m_inputControllable;
	std::unique_ptr<tinyxml2::XMLDocument> m_pDoc;
	tinyxml2::XMLNode* m_pSource;
};


#include "UIRepeat.h"
#include "UIApplication.h"

UIRepeat::UIRepeat()
	: m_repeatCount(0)
	, m_inputControllable(false)
	, m_pDoc(nullptr)
	, m_pSource(nullptr)
{
	m_elementType = ElementType::kRepeat;

	m_repeatCount.ChangedEvent() += [this](UIProperty*, const UIProperty::ValueType&, const UIProperty::ValueType& val)
	{
		if (!m_inputControllable)
			return;

		auto& uiManager = UIApplication::Get()->GetUIManager();

		uiManager.DynamicallyLoadRepeatElements(m_pSource->ToElement(), m_id, std::get<int>(val));
		UIApplication::Get()->SignalRearrange();
	};

}

bool UIRepeat::BindRepeatCount(UIProperty& binder)
{
	if (!binder.EqualType(m_repeatCount))
	{
		SDL_Log("Trying to bind properties with unequal type!");
		return false;
	}

	binder.Bind(m_repeatCount);
	m_inputControllable = true;
	return true;
}

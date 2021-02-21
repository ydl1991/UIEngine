#include "UIProperty.h"
#include "Utils/Helper.h"

void UIProperty::DoubleBind(UIProperty& propA, UIProperty& propB)
{
	// Initial sync
	propB = propA;

	// NOTE: Previously here we were trying to capture booleans for
	// tracking changes to prevent cyclical events from happening.
	// However, since these bool were on the stack, they go out of scope
	// after this function returns, which means their memory can be who knows what
	// But, as it turns out, these are not needed.
	// We already perform duplication checks in our ChangeValue method.
	// That is sufficient enough to not cause cyclical changed events.

	propA.ChangedEvent() += [&propB]
	(UIProperty*, const ValueType&, const ValueType& current)
	{
		propB = current;
	};
	propB.ChangedEvent() += [&propA]
	(UIProperty*, const ValueType&, const ValueType& current)
	{
		propA = current;
	};
}

void UIProperty::Bind(UIProperty& child)
{
	child = m_value;
	m_changedEvent += [&child](UIProperty*, const ValueType&, const ValueType& current)
	{
		child = current;
	};
}

bool UIProperty::EqualType(const UIProperty& prop) const
{
	return m_value.index() == prop.m_value.index();
}

UIProperty::Type UIProperty::CurrentHoldingType() const
{
	return (Type)(m_value.index());
}

void UIProperty::CastToType(Type toType)
{
	if (CurrentHoldingType() == toType)
		return;

	std::string str = ToString();

	if (toType == Type::kString)
	{
		m_value = str;
	}
	else if (toType == Type::kInt)
	{
		if (str.empty())
			m_value = 0;
		else if (IsInt(str))
			m_value = std::stoi(str);
	}
	else if (toType == Type::kFloat)
	{
		if (str.empty())
			m_value = 0.f;
		else if (IsFloat(str))
			m_value = std::stof(str);
	}
	else if (toType == Type::kBool)
	{
		if (str.empty())
			m_value = false;
		else if (IsBool(str))
			m_value = ToBool(str);
	}
}

bool UIProperty::EqualType(const UIProperty& prop1, const UIProperty& prop2)
{
	return prop1.m_value.index() == prop2.m_value.index();
}

std::string UIProperty::ToString(const ValueType& val)
{
	std::ostringstream valueString;

	if (const int* value = std::get_if<int>(&val))
	{
		valueString << *value;
	}
	else if (const float* value = std::get_if<float>(&val))
	{
		valueString << *value;
	}
	else if (const std::string* value = std::get_if<std::string>(&val))
	{
		valueString << *value;
	}
	else if (const bool* value = std::get_if<bool>(&val))
	{
		valueString << (*value ? "true" : "false");
	}
	else
	{
		valueString << "<none>";
	}

	return valueString.str();
}

std::string UIProperty::ToString()
{
	std::ostringstream valueString;

	if (const int* value = std::get_if<int>(&m_value))
	{
		valueString << *value;
	}
	else if (const float* value = std::get_if<float>(&m_value))
	{
		valueString << *value;
	}
	else if (const std::string* value = std::get_if<std::string>(&m_value))
	{
		valueString << *value;
	}
	else if (const bool* value = std::get_if<bool>(&m_value))
	{
		valueString << (*value ? "true" : "false");
	}
	else
	{
		valueString << "<none>";
	}

	return valueString.str();
};

void UIProperty::ChangeValue(const ValueType& newValue)
{
	if (newValue == m_value)// || newValue.index() != m_value.index())
		return;

	ValueType oldValue;
	std::swap(oldValue, m_value);
	m_value = newValue;
	m_changedEvent.Trigger(this, oldValue, m_value);
}

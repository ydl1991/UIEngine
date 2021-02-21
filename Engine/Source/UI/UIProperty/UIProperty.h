#pragma once
#include "UIObservableEvent.h"

#include <string>
#include <variant>

class UIProperty
{
public:
	enum class Type : uint8_t
	{
		kString = 0,
		kBool,
		kInt,
		kFloat
	};

	using ValueType = std::variant<std::string, bool, int, float>;
	using ChangedEventType = UIObservableEvent<UIProperty*, const ValueType&, const ValueType&>;

	static void DoubleBind(UIProperty& propA, UIProperty& propB);
	static bool EqualType(const UIProperty& prop1, const UIProperty& prop2);
	static std::string ToString(const ValueType& val);

	UIProperty() = default;
	explicit UIProperty(bool value) : m_value(value) {}
	explicit UIProperty(int value) : m_value(value) {}
	explicit UIProperty(float value) : m_value(value) {}
	explicit UIProperty(const std::string& value) : m_value(value) {}

	UIProperty& operator =(bool value) { ChangeValue(value); return *this; }
	UIProperty& operator =(int value) { ChangeValue(value); return *this; }
	UIProperty& operator =(float value) { ChangeValue(value); return *this; }
	UIProperty& operator =(const std::string& value) { ChangeValue(value); return *this; }
	UIProperty& operator =(const char* pValue) { *this = std::string(pValue);  return *this; }
	UIProperty& operator =(const ValueType& value) { ChangeValue(value); return *this; }
	UIProperty& operator =(const UIProperty& other) { ChangeValue(other.m_value); return *this; }

	explicit operator bool() const { return std::get<bool>(m_value); }
	explicit operator int() const { return std::get<int>(m_value); }
	explicit operator float() const { return std::get<float>(m_value); }
	explicit operator const std::string& () const { return std::get<std::string>(m_value); }
	explicit operator const ValueType& () const { return m_value; }

	ChangedEventType& ChangedEvent() { return m_changedEvent; }
	void Bind(UIProperty& child);
	[[nodiscard]] bool EqualType(const UIProperty& prop) const;
	[[nodiscard]] Type CurrentHoldingType() const;
	void CastToType(Type toType);

	std::string ToString();
	
	template<class T>
	const T* Get() const { return std::get_if<T>(&m_value); }

private:
	void ChangeValue(const ValueType& newValue);

private:
	ValueType m_value;
	ChangedEventType m_changedEvent;
};


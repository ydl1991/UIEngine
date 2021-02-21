#pragma once
#include <vector>
#include <functional>

template <typename ... Args>
class UIObservableEvent
{
public:
	using ListenerFunc = void(Args ... args);

	UIObservableEvent<Args...>& operator +=(std::function<ListenerFunc> func)
	{
		m_listeners.emplace_back(std::move(func));
		return *this;
	}

	void Trigger(Args ... args)
	{
		for (auto& listener : m_listeners)
		{
			listener(args...);
		}
	}

protected:
	std::vector<std::function<ListenerFunc>> m_listeners;
};
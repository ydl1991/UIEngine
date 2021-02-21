#pragma once

#include <functional>
#include <memory>

class UIElement;

class UIProcess
{
public:
	enum State : uint8_t
	{
		kSucceeded,
		kFailed,
		kAborted,

		kNumStatesWithCallback,

		kUninitialized,
		kRunning,
		kPaused,
	};

	// Alias
	using Callback = std::function<void(const std::weak_ptr<UIElement>&)>;

	UIProcess();
	virtual ~UIProcess() = default;
	explicit UIProcess(std::shared_ptr<UIElement>& pOwner);

	virtual bool Init() { return true; }
	virtual void Update(float deltaSeconds);

	void OnFailure() const { if (m_stateCallbacks[kFailed]) m_stateCallbacks[kFailed](m_pOwner); }
	void OnSucceed() const { if (m_stateCallbacks[kSucceeded]) m_stateCallbacks[kSucceeded](m_pOwner); }
	void OnAbort() const { if (m_stateCallbacks[kAborted]) m_stateCallbacks[kAborted](m_pOwner); }

	// Get Owner
	[[nodiscard]] std::shared_ptr<UIElement> GetOwner() const { return m_pOwner.lock(); }
	
	// State Getter
	[[nodiscard]] State GetState() const { return m_state; }
	// State Setters
	void Succeed() { m_state = kSucceeded; }
	void Abort() { m_state = kAborted; }
	void Fail() { m_state = kFailed; }
	void Pause() { m_state = kPaused; }
	void Resume() { m_state = kRunning; }
	void Reset() { m_state = kUninitialized; }

	// State Checkers
	[[nodiscard]] bool IsAlive() const { return (m_state == kRunning || m_state == kPaused); }
	[[nodiscard]] bool IsDead() const { return (!IsAlive() && m_state != kUninitialized); }

	// Set Callback function for states
	void SetCallback(State state, Callback callback) { m_stateCallbacks[state] = std::move(callback); }

	// child process
	void AttachChild(std::shared_ptr<UIProcess> pChild) { m_pChild = std::move(pChild); }
	std::shared_ptr<UIProcess> RemoveChild();

	void CheckForAbort(int ownerId);

protected:
	std::weak_ptr<UIElement> m_pOwner;
	
private:
	State m_state;
	Callback m_stateCallbacks[kNumStatesWithCallback];
	std::shared_ptr<UIProcess> m_pChild;
};
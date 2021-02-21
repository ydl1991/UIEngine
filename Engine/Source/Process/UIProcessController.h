#pragma once

#include <memory>
#include <vector>

#include <Process/UIProcess.h>

class UIProcessController
{
public:
	// Alias
	using SharedProcess = std::shared_ptr<UIProcess>;

	UIProcessController()									  = default;
	~UIProcessController();
	UIProcessController(const UIProcessController&)			  = delete;
	UIProcessController operator=(const UIProcessController&) = delete;

	void UpdateProcesses(float deltaSeconds);
	void AttachProcess(SharedProcess pProcess) { m_processes.emplace_back(std::move(pProcess)); }
	void AbortAllProcesses();
	void AbortProcessesWithElementId(int id);

private:
	std::vector<SharedProcess> m_processes;
};


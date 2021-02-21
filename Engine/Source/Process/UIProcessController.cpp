#include "UIProcessController.h"

UIProcessController::~UIProcessController()
{
	AbortAllProcesses();
}

void UIProcessController::UpdateProcesses(float deltaSeconds)
{
	size_t processIndex = 0;
	while (processIndex < m_processes.size())
	{
		// Only going to increment the index when we don't remove a process
		auto pProcess = m_processes[processIndex];

		if (pProcess->GetState() == UIProcess::State::kRunning)
		{
			pProcess->Update(deltaSeconds);
		}
		else if (pProcess->GetState() == UIProcess::State::kUninitialized)
		{
			if (pProcess->Init())
			{
				pProcess->Resume();
			}
			else
			{
				// remove it !
				m_processes.erase(m_processes.begin() + processIndex);
				continue;
			}
		}
		else if (pProcess->IsDead())
		{
			UIProcess::State state = pProcess->GetState();
			if (state == UIProcess::State::kSucceeded)
			{
				// Add a child if there is any
				pProcess->OnSucceed();
				auto pChild = pProcess->RemoveChild();
				if (pChild)
					m_processes.emplace_back(pChild);
			}
			else if (state == UIProcess::State::kFailed)
			{
				pProcess->OnFailure();
			}
			else if (state == UIProcess::State::kAborted)
			{
				pProcess->OnAbort();
			}

			pProcess->Reset();
			// remove it
			m_processes.erase(m_processes.begin() + processIndex);
			continue;
		}

		++processIndex;
	}
}

void UIProcessController::AbortAllProcesses()
{
	for (auto& proc : m_processes)
	{
		if (proc->IsAlive())
		{
			proc->Abort();
			proc->OnAbort();
		}
	}

	m_processes.clear();
}

void UIProcessController::AbortProcessesWithElementId(int id)
{
	for (auto& process : m_processes)
	{
		process->CheckForAbort(id);
	}
}

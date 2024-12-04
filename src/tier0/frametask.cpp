//=============================================================================//
//
// Purpose: 
// 
//-----------------------------------------------------------------------------
//
//=============================================================================//
#include "tier0/frametask.h"

//-----------------------------------------------------------------------------
// Purpose: run frame task and process queued calls
//-----------------------------------------------------------------------------
void CFrameTask::RunFrame()
{
    AUTO_LOCK(m_Mutex);

    for (QueuedTasks_s& delay : m_QueuedTasks)
    {
        if (delay.m_nDelayedFrames == 0)
        {
            delay.m_rFunctor();
        }
        else
        {
            --delay.m_nDelayedFrames;
        }
    }

    const auto newEnd = std::remove_if(m_QueuedTasks.begin(), m_QueuedTasks.end(),
        [](const QueuedTasks_s& delay)
        {
            return delay.m_nDelayedFrames == 0;
        });

    m_QueuedTasks.erase(newEnd, m_QueuedTasks.end());
}

//-----------------------------------------------------------------------------
// Purpose: is the task finished
// Output : true if finished, false otherwise
//-----------------------------------------------------------------------------
bool CFrameTask::IsFinished() const
{
    return false;
}

//-----------------------------------------------------------------------------
// Purpose: adds function to list, to be called after 'i' frames.
// Input  : functor - 
//          frames - 
//-----------------------------------------------------------------------------
void CFrameTask::Dispatch(std::function<void()> functor, unsigned int frames)
{
    AUTO_LOCK(m_Mutex);
    m_QueuedTasks.emplace_back(frames, functor);
}

//-----------------------------------------------------------------------------
std::list<IFrameTask*> g_TaskQueueList;
CFrameTask g_TaskQueue;

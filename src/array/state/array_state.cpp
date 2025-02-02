/*
 *   BSD LICENSE
 *   Copyright (c) 2021 Samsung Electronics Corporation
 *   All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "array_state.h"

#include <string>

#include "src/include/pos_event_id.h"
#include "src/logger/logger.h"

namespace pos
{
ArrayState::ArrayState(IStateControl* iState)
: iStateControl(iState)
{
    string sender = typeid(*this).name();
    degradedState = new StateContext(sender, SituationEnum::DEGRADED);
    rebuildingState = new StateContext(sender, SituationEnum::REBUILDING);
    stopState = new StateContext(sender, SituationEnum::FAULT);
    iStateControl->Subscribe(this, sender);
}

ArrayState::~ArrayState(void)
{
    iStateControl->Unsubscribe(this);
    delete stopState;
    delete rebuildingState;
    delete degradedState;
}

ArrayStateType
ArrayState::GetState(void)
{
    return state;
}

StateContext*
ArrayState::GetSysState(void)
{
    return iStateControl->GetState();
}

void
ArrayState::SetState(ArrayStateEnum nextState)
{
    // Method for Unit Test Only
    state = nextState;
}

void
ArrayState::SetLoad(uint32_t missingCnt, uint32_t brokenCnt)
{
    ArrayStateType newState;
    uint32_t abnormalCnt = missingCnt + brokenCnt;

    switch (abnormalCnt)
    {
        case 0:
        {
            newState = ArrayStateEnum::EXIST_NORMAL;
            break;
        }
        case 1:
        {
            newState = ArrayStateEnum::EXIST_DEGRADED;
            if (missingCnt != 0)
            {
                needForceMount = true;
            }
            break;
        }
        default:
        {
            newState = ArrayStateEnum::BROKEN;
        }
    }
    _SetState(newState);
}

void
ArrayState::SetCreate(void)
{
    _SetState(ArrayStateEnum::EXIST_NORMAL);
}

int
ArrayState::CanAddSpare(void)
{
    int eventId = 0;
    if (!IsMounted())
    {
        eventId = (int)POS_EVENT_ID::ARRAY_NEED_MOUNT;
        POS_TRACE_WARN(eventId, "Failed to add spare. Spare cannot be added without Array mounted");
    }
    return eventId;
}

int
ArrayState::CanRemoveSpare(void)
{
    int eventId = 0;
    if (!IsMounted())
    {
        eventId = (int)POS_EVENT_ID::ARRAY_NEED_MOUNT;
        POS_TRACE_WARN(eventId, "Failed to remove spare. Spare cannot be removed without Array mounted");
    }
    return eventId;
}

int
ArrayState::IsMountable(void)
{
    int eventId = 0;

    switch (state)
    {
        case ArrayStateEnum::NOT_EXIST:
        {
            eventId = (int)POS_EVENT_ID::ARRAY_STATE_NOT_EXIST;
            POS_TRACE_ERROR(eventId, "Failed to mount array. Array is not existed");
            break;
        }
        case ArrayStateEnum::EXIST_DEGRADED:
        {
            int degradedEvent = (int)POS_EVENT_ID::ARRAY_STATE_EXIST_DEGRADED;
            if (needForceMount)
            {
                POS_TRACE_WARN(degradedEvent, "Array force-mounted with degraded state");
            }
            else
            {
                POS_TRACE_WARN(degradedEvent, "Array mounted with degraded state");
            }
            break;
        }
        case ArrayStateEnum::BROKEN:
        {
            eventId = (int)POS_EVENT_ID::ARRAY_STATE_BROKEN;
            POS_TRACE_ERROR(eventId, "Failed to mount array. Array is broken");
            break;
        }
        default:
        {
        }
    }
    return eventId;
}

int
ArrayState::IsLoadable(void)
{
    int eventId = 0;
    if (IsMounted())
    {
        eventId = (int)POS_EVENT_ID::ARRAY_STATE_ONLINE;
        POS_TRACE_ERROR(eventId, "Failed to load array, already mounted");
    }
    else if (state == ArrayStateEnum::BROKEN)
    {
        eventId = (int)POS_EVENT_ID::ARRAY_BROKEN_ERROR;
        POS_TRACE_ERROR(eventId, "Failed to load array, broken error");
    }

    return eventId;
}

int
ArrayState::IsCreatable(void)
{
    int eventId = 0;
    if (IsMounted())
    {
        eventId = (int)POS_EVENT_ID::ARRAY_STATE_ONLINE;
        POS_TRACE_ERROR(eventId, "Failed to load array, already mounted");
    }
    else if (Exists())
    {
        eventId = (int)POS_EVENT_ID::ARRAY_STATE_EXIST;
        POS_TRACE_ERROR(eventId, "Failed to create array, already existed");
    }

    return eventId;
}

int
ArrayState::IsUnmountable(void)
{
    int eventId = 0;
    if (IsMounted() == false)
    {
        eventId = (int)POS_EVENT_ID::ARRAY_STATE_OFFLINE;
        POS_TRACE_ERROR(eventId, "Failed to unmount array, not mounted");
    }
    else if (state == ArrayStateEnum::REBUILD)
    {
        eventId = (int)POS_EVENT_ID::ARRAY_STATE_REBUILDING;
        POS_TRACE_ERROR(eventId, "Failed to unmount array, array is being rebuilt");
    }

    return eventId;
}

int
ArrayState::IsDeletable(void)
{
    int eventId = 0;
    if (IsMounted())
    {
        eventId = (int)POS_EVENT_ID::ARRAY_STATE_ONLINE;
        POS_TRACE_ERROR(eventId, "Failed to delete array, already mounted");
    }
    else if (Exists() == false)
    {
        eventId = (int)POS_EVENT_ID::ARRAY_STATE_NOT_EXIST;
        POS_TRACE_ERROR(eventId, "Failed to delete array, already mounted");
    }
    return eventId;
}

bool
ArrayState::Exists(void)
{
    bool isExist = state > ArrayStateEnum::NOT_EXIST;
    return isExist;
}

bool
ArrayState::IsMounted(void)
{
    bool isMounted = state >= ArrayStateEnum::NORMAL;
    return isMounted;
}

int
ArrayState::SetMount(void)
{
    switch (state)
    {
        case ArrayStateEnum::EXIST_NORMAL:
        {
            _SetState(ArrayStateEnum::NORMAL);
            break;
        }
        case ArrayStateEnum::EXIST_DEGRADED:
        {
            _SetState(ArrayStateEnum::DEGRADED);
            break;
        }
        default:
        {
            POS_EVENT_ID id = POS_EVENT_ID::ARRAY_STATE_CHANGE_ERROR;
            POS_TRACE_WARN(id,
                "Failed to change array state, curret state is {}",
                state.ToString());
            return (int)id;
        }
    }
    return 0;
}

int
ArrayState::SetUnmount(void)
{
    iStateControl->Remove(degradedState);

    switch (state)
    {
        case ArrayStateEnum::NORMAL:
        {
            _SetState(ArrayStateEnum::EXIST_NORMAL);
            break;
        }
        case ArrayStateEnum::DEGRADED:
        {
            _SetState(ArrayStateEnum::EXIST_DEGRADED);
            break;
        }
        default:
        {
            POS_EVENT_ID id = POS_EVENT_ID::ARRAY_STATE_CHANGE_ERROR;
            POS_TRACE_WARN(id,
                "Failed to change array state, curret state is {}",
                state.ToString());
            return (int)id;
        }
    }
    return 0;
}

void
ArrayState::DataRemoved(bool isRebuildingDevice)
{
    switch (state)
    {
        case ArrayStateEnum::EXIST_NORMAL:
        {
            _SetState(ArrayStateEnum::EXIST_DEGRADED);
            break;
        }
        case ArrayStateEnum::EXIST_DEGRADED:
        {
            _SetState(ArrayStateEnum::BROKEN);
            break;
        }
        case ArrayStateEnum::NORMAL:
        {
            _SetState(ArrayStateEnum::DEGRADED);
            break;
        }
        case ArrayStateEnum::REBUILD:
        {
            if (isRebuildingDevice)
            {
                iStateControl->Remove(rebuildingState);
                _SetState(ArrayStateEnum::DEGRADED);
            }
            else
            {
                _SetState(ArrayStateEnum::BROKEN);
            }
            break;
        }
        case ArrayStateEnum::DEGRADED:
        {
            _SetState(ArrayStateEnum::BROKEN);
            break;
        }
        case ArrayStateEnum::NOT_EXIST:
        {
            assert(0);
        }
        default:
        {
        }
    }
}

void
ArrayState::SetDelete(void)
{
    iStateControl->Remove(stopState);
    _SetState(ArrayStateEnum::NOT_EXIST);
}

bool
ArrayState::IsRebuildable(void)
{
    return state == ArrayStateEnum::DEGRADED;
}

bool
ArrayState::IsBroken(void)
{
    return state == ArrayStateEnum::BROKEN;
}

bool
ArrayState::SetRebuild(void)
{
    if (IsRebuildable() == false)
    {
        return false;
    }
    _SetState(ArrayStateEnum::REBUILD);
    bool res = _WaitState(rebuildingState);
    if (res == false)
    {
        iStateControl->Remove(rebuildingState);
        _SetState(ArrayStateEnum::DEGRADED);
        return false;
    }

    return true;
}

void
ArrayState::SetDegraded(void)
{
    _SetState(ArrayStateEnum::DEGRADED);
}

bool
ArrayState::IsRecoverable(void)
{
    switch (state)
    {
        case ArrayStateEnum::EXIST_DEGRADED:
        case ArrayStateEnum::DEGRADED:
        {
            return true;
        }
        // TODO(srm) if poseidonos is not online, it will cause the error
        // case ArrayStateEnum::BROKEN:
        // {
        //     return false;
        // }
        default:
        {
            return false;
        }
    }
}

void
ArrayState::SetRebuildDone(bool isSuccess)
{
    iStateControl->Remove(rebuildingState);
    if (isSuccess)
    {
        if (state == ArrayStateEnum::REBUILD)
        {
            iStateControl->Remove(degradedState);
            _SetState(ArrayStateEnum::NORMAL);
        }
        else if (state != ArrayStateEnum::BROKEN)
        {
            POS_TRACE_ERROR((int)POS_EVENT_ID::ARRAY_STATE_CHANGE_ERROR,
                "Unknown State Transition " + state.ToString());
        }
        else
        {
            POS_TRACE_DEBUG((int)POS_EVENT_ID::ARRAY_STATE_BROKEN,
                "Rebuild done on exist broken state");
        }
    }
}

void
ArrayState::StateChanged(StateContext* prev, StateContext* next)
{
    unique_lock<mutex> lock(mtx);
    cv.notify_all();
}

void
ArrayState::_SetState(ArrayStateEnum newState)
{
    if (state != newState)
    {
        if (newState == ArrayStateEnum::DEGRADED)
        {
            iStateControl->Invoke(degradedState);
        }
        else if (newState == ArrayStateEnum::REBUILD)
        {
            iStateControl->Invoke(rebuildingState);
        }
        else if (newState == ArrayStateEnum::BROKEN)
        {
            iStateControl->Invoke(stopState);
        }

        state = newState;
        POS_TRACE_INFO((int)POS_EVENT_ID::ARRAY_STATE_CHANGED,
            "Array state changed to {}",
            state.ToString());
    }
}

bool
ArrayState::_WaitState(StateContext* goal)
{
    int timeout_sec = 3;
    std::unique_lock<std::mutex> lock(mtx);
    while (iStateControl->GetState() != goal)
    {
        if (cv.wait_for(lock, chrono::seconds(timeout_sec)) == cv_status::timeout)
        {
            return false;
        }
    }
    return true;
}

} // namespace pos

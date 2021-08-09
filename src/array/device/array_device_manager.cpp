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

#include "array_device_manager.h"

#include <algorithm>
#include <vector>

#include "src/device/base/ublock_device.h"
#include "src/device/device_manager.h"
#include "src/include/array_config.h"
#include "src/include/pos_event_id.h"
#include "src/logger/logger.h"
#include "src/volume/volume_list.h"

namespace pos
{
ArrayDeviceManager::ArrayDeviceManager(DeviceManager* sysDevMgr)
:
#ifdef _ADMIN_ENABLED
  IArrayDevMgr(sysDevMgr),
#endif
  sysDevMgr_(sysDevMgr)
{
}

ArrayDeviceManager::~ArrayDeviceManager(void)
{
    delete devs_;
}

int
ArrayDeviceManager::Import(DeviceSet<string> nameSet)
{
    int ret = 0;
    ArrayDeviceList* devs = new ArrayDeviceList();

    for (string devName : nameSet.nvm)
    {
        printf("==> ArrayDeviceManager::Import(DeviceSet<string> nameSet)  [%s]\n", devName.c_str());
        POS_TRACE_ERROR((int)POS_EVENT_ID::ARRAY_DEBUG_MSG, "ERROR Import().... ", devName);

        DevName name(devName);
        UblockSharedPtr uBlock = sysDevMgr_->GetDev(name);
        if (nullptr == uBlock)
        {
            delete devs;
            ret = (int)POS_EVENT_ID::ARRAY_DEVICE_NOT_FOUND;
            return ret;
        }
        if (uBlock->GetType() != DeviceType::NVRAM)
        {
            delete devs;
            ret = (int)POS_EVENT_ID::ARRAY_DEVICE_TYPE_ERROR;
            return ret;
        }
        ret = devs->SetNvm(new ArrayDevice(uBlock));
        if (ret != 0)
        {
            delete devs;
            return ret;
        }
    }

    for (string devName : nameSet.data)
    {
        DevName name(devName);
        UblockSharedPtr uBlock = sysDevMgr_->GetDev(name);
        if (nullptr == uBlock)
        {
            delete devs;
            ret = (int)POS_EVENT_ID::ARRAY_DEVICE_NOT_FOUND;
            return ret;
        }
        if (uBlock->GetType() != DeviceType::SSD)
        {
            delete devs;
            ret = (int)POS_EVENT_ID::ARRAY_DEVICE_TYPE_ERROR;
            return ret;
        }
        ret = devs->AddData((new ArrayDevice(uBlock)));
        if (ret != 0)
        {
            delete devs;
            return ret;
        }
    }
    for (string devName : nameSet.spares)
    {
        DevName name(devName);
        UblockSharedPtr uBlock = sysDevMgr_->GetDev(name);
        if (nullptr == uBlock)
        {
            delete devs;
            ret = (int)POS_EVENT_ID::ARRAY_DEVICE_NOT_FOUND;
            return ret;
        }
        if (uBlock->GetType() != DeviceType::SSD)
        {
            delete devs;
            ret = (int)POS_EVENT_ID::ARRAY_DEVICE_TYPE_ERROR;
            return ret;
        }
        ret = devs->AddSpare(new ArrayDevice(uBlock));
        if (ret != 0)
        {
            delete devs;
            return ret;
        }
    }
    ret = _CheckConstraints(devs);
    if (ret == 0)
    {
        devs_ = devs;
    }
    else
    {
        delete devs;
    }

    return ret;
}

int
ArrayDeviceManager::Import(DeviceSet<DeviceMeta> metaSet, uint32_t& missingCnt, uint32_t& brokenCnt)
{
    int ret = 0;
    ArrayDeviceList* devs = new ArrayDeviceList();

    for (DeviceMeta meta : metaSet.nvm)
    {
        DevUid uid(meta.uid);
        UblockSharedPtr uBlock = sysDevMgr_->GetDev(uid);
        if (nullptr == uBlock)
        {
            delete devs;
            ret = (int)POS_EVENT_ID::ARRAY_DEVICE_NVM_NOT_FOUND;
            return ret;
        }
        devs->SetNvm(new ArrayDevice(uBlock));
    }

    for (DeviceMeta meta : metaSet.data)
    {
        ArrayDevice* dev = nullptr;
        DevUid uid(meta.uid);

        if (ArrayDeviceState::FAULT == meta.state)
        {
            dev = new ArrayDevice(nullptr, ArrayDeviceState::FAULT);
            brokenCnt++;
        }
        else
        {
            UblockSharedPtr uBlock = sysDevMgr_->GetDev(uid);
            if (uBlock == nullptr)
            {
                missingCnt++;
                meta.state = ArrayDeviceState::FAULT;
            }
            else if (ArrayDeviceState::REBUILD == meta.state)
            {
                brokenCnt++;
                POS_TRACE_DEBUG((int)POS_EVENT_ID::ARRAY_DEVICE_REBUILD_STATE,
                    "Rebuilding device found {}", meta.uid);
            }

            dev = new ArrayDevice(uBlock, meta.state);
        }

        devs->AddData(dev);
    }

    for (DeviceMeta meta : metaSet.spares)
    {
        DevUid uid(meta.uid);
        UblockSharedPtr uBlock = sysDevMgr_->GetDev(uid);
        if (nullptr != uBlock)
        {
            devs->AddSpare(new ArrayDevice(uBlock));
        }
    }

    devs_ = devs;
    return ret;
}

int
ArrayDeviceManager::AddSpare(string devName)
{
    DevName name(devName);
    UblockSharedPtr spare = sysDevMgr_->GetDev(name);

    if (spare == nullptr)
    {
        return (int)POS_EVENT_ID::ARRAY_DEVICE_WRONG_NAME;
    }

    if (spare->GetClass() != DeviceClass::SYSTEM)
    {
        return (int)POS_EVENT_ID::ARRAY_DEVICE_ALREADY_ADDED;
    }

    if (false == spare->IsAlive())
    {
        return -2; // TODO
    }

    ArrayDevice* baseline = _GetBaseline(devs_->GetDevs().data);
    if (baseline->GetUblock()->GetSize() != spare->GetSize())
    {
        return (int)POS_EVENT_ID::ARRAY_SSD_SAME_CAPACITY_ERROR;
    }

    devs_->AddSpare(new ArrayDevice(spare));
    return 0;
}

int
ArrayDeviceManager::_CheckDevs(const ArrayDeviceSet& devSet)
{
    int ret = _CheckFaultTolerance(devSet);
    if (ret != 0)
    {
        return ret;
    }

    for (ArrayDevice* dev : devSet.nvm)
    {
        if (nullptr != dev->GetUblock() && false == dev->GetUblock()->IsAlive())
        {
            int eventId = (int)POS_EVENT_ID::ARRAY_DEVICE_NOT_FOUND;
            POS_TRACE_WARN(eventId, "Device not found");
            return eventId;
        }
    }
    for (ArrayDevice* dev : devSet.data)
    {
        if (nullptr != dev->GetUblock() && false == dev->GetUblock()->IsAlive())
        {
            int eventId = (int)POS_EVENT_ID::ARRAY_DEVICE_NOT_FOUND;
            POS_TRACE_WARN(eventId, "Device not found");
            return eventId;
        }
    }
    for (ArrayDevice* dev : devSet.spares)
    {
        if (nullptr == dev->GetUblock() || false == dev->GetUblock()->IsAlive())
        {
            int eventId = (int)POS_EVENT_ID::ARRAY_DEVICE_NOT_FOUND;
            POS_TRACE_WARN(eventId, "Device not found");
            return eventId;
        }
    }
    return 0;
}

void
ArrayDeviceManager::Clear(void)
{
    if (devs_ != nullptr)
    {
        devs_->Clear();
        delete devs_;
        devs_ = nullptr;
    }
}

DeviceSet<DeviceMeta>
ArrayDeviceManager::ExportToMeta(void)
{
    DeviceSet<ArrayDevice*> _d = devs_->GetDevs();
    DeviceSet<DeviceMeta> metaSet;
    string sn = _d.nvm.at(0)->GetUblock()->GetSN();
    DeviceMeta nvmMeta(sn, _d.nvm.at(0)->GetState());
    metaSet.nvm.push_back(nvmMeta);

    for (ArrayDevice* dev : _d.data)
    {
        DeviceMeta deviceMeta;
        deviceMeta.state = dev->GetState();
        if (ArrayDeviceState::FAULT == deviceMeta.state)
        {
            deviceMeta.uid = "";
        }
        else
        {
            deviceMeta.uid = dev->GetUblock()->GetSN();
        }
        metaSet.data.push_back(deviceMeta);
    }
    for (ArrayDevice* dev : _d.spares)
    {
        DeviceMeta deviceMeta(dev->GetUblock()->GetSN(), dev->GetState());
        metaSet.spares.push_back(deviceMeta);
    }

    return metaSet;
}

DeviceSet<string>
ArrayDeviceManager::ExportToName(void)
{
    if (devs_ == nullptr)
    {
        return DeviceSet<string>();
    }

    return devs_->ExportNames();
}

DeviceSet<ArrayDevice*>&
ArrayDeviceManager::Export(void)
{
    return devs_->GetDevs();
}

int
ArrayDeviceManager::RemoveSpare(string devName)
{
    ArrayDeviceType devType;
    ArrayDevice* dev = nullptr;
    DevName name(devName);
    UblockSharedPtr uBlock = sysDevMgr_->GetDev(name);
    tie(dev, devType) = this->GetDev(uBlock);
    if (devType != ArrayDeviceType::SPARE)
    {
        return (int)POS_EVENT_ID::ARRAY_DEVICE_REMOVE_FAIL;
    }
    return devs_->RemoveSpare(dev);
}

int
ArrayDeviceManager::RemoveSpare(ArrayDevice* dev)
{
    return devs_->RemoveSpare(dev);
}

int
ArrayDeviceManager::ReplaceWithSpare(ArrayDevice* target)
{
    return devs_->SpareToData(target);
}

ArrayDevice*
ArrayDeviceManager::GetFaulty(void)
{
    for (ArrayDevice* dev : devs_->GetDevs().data)
    {
        if (ArrayDeviceState::FAULT == dev->GetState())
        {
            return dev;
        }
    }

    return nullptr;
}

ArrayDevice*
ArrayDeviceManager::GetRebuilding(void)
{
    for (ArrayDevice* dev : devs_->GetDevs().data)
    {
        if (ArrayDeviceState::REBUILD == dev->GetState())
        {
            return dev;
        }
    }

    return nullptr;
}

int
ArrayDeviceManager::_CheckConstraints(ArrayDeviceList* devs)
{
    ArrayDeviceSet devSet = devs->GetDevs();

    int ret = _CheckDevsCount(devSet);
    if (0 != ret)
    {
        return ret;
    }

    ret = _CheckDevs(devSet);
    if (0 != ret)
    {
        return ret;
    }

    ret = _CheckNvmCapacity(devSet);
    if (0 != ret)
    {
        return ret;
    }

    ret = _CheckSsdsCapacity(devSet);

    return ret;
}

int
ArrayDeviceManager::_CheckDevsCount(ArrayDeviceSet devSet)
{
    uint32_t nvmCnt = devSet.nvm.size();

    uint32_t activeDataCnt = 0;
    uint32_t faultDataCnt = 0;
    for (ArrayDevice* dev : devSet.data)
    {
        if (dev->GetUblock() != nullptr)
        {
            activeDataCnt++;
        }
        else
        {
            faultDataCnt++;
        }
    }
    uint32_t spareCnt = devSet.spares.size();
    uint32_t totalCnt = activeDataCnt + spareCnt;

    int ret = 0;
    uint32_t errEventId = (uint32_t)POS_EVENT_ID::ARRAY_DEVICE_COUNT_ERROR;

    uint32_t maxNvmCnt = ArrayConfig::NVM_DEVICE_COUNT;
    uint32_t minDataCnt = ArrayConfig::MINIMUM_DATA_DEVICE_COUNT;
    uint32_t maxDataCnt = ArrayConfig::MAXIMUM_DEVICE_COUNT;

    if (nvmCnt != maxNvmCnt)
    {
        POS_TRACE_ERROR(errEventId,
            "The number of nvm device must be {}", maxNvmCnt);
        ret = errEventId;
    }
    else if (activeDataCnt + faultDataCnt < minDataCnt)
    {
        POS_TRACE_ERROR(errEventId,
            "The number of data disks must be {} or more", minDataCnt);
        ret = errEventId;
    }
    else if (totalCnt > maxDataCnt)
    {
        POS_TRACE_ERROR(errEventId,
            "The number of disks must be no more {}", maxDataCnt);
        ret = errEventId;
    }

    return ret;
}

int
ArrayDeviceManager::_CheckFaultTolerance(DeviceSet<ArrayDevice*> devSet)
{
    int faultDevCnt = 0;
    for (ArrayDevice* dev : devSet.data)
    {
        if (dev->GetState() != ArrayDeviceState::NORMAL)
        {
            faultDevCnt++;
        }
    }

    const int brokenCnt = 2;
    if (faultDevCnt >= brokenCnt)
    {
        uint32_t eventId = (uint32_t)POS_EVENT_ID::ARRAY_BROKEN_ERROR;
        POS_TRACE_ERROR(eventId,
            "Array cannot be configured due to faults of {} data devices",
            faultDevCnt);
        return eventId;
    }

    return 0;
}

int
ArrayDeviceManager::_CheckNvmCapacity(const DeviceSet<ArrayDevice*>& devSet)
{
    ArrayDevice* nvm = devSet.nvm.at(0);
    uint32_t logicalChunkCount = devSet.data.size() - ArrayConfig::PARITY_COUNT;
    uint64_t minNvmSize = _ComputeMinNvmCapacity(logicalChunkCount);

    if (nvm->GetUblock()->GetSize() < minNvmSize)
    {
        int eventId = (int)POS_EVENT_ID::ARRAY_NVM_CAPACITY_ERROR;
        POS_TRACE_WARN(eventId, "NVM device size error");
        return eventId;
    }

    return 0;
}

uint64_t
ArrayDeviceManager::_ComputeMinNvmCapacity(const uint32_t logicalChunkCount)
{
    uint64_t writeBufferStripeSize = 0;
    writeBufferStripeSize = static_cast<uint64_t>(logicalChunkCount) *
        ArrayConfig::BLOCK_SIZE_BYTE * ArrayConfig::BLOCKS_PER_CHUNK;
    uint64_t writeBufferStripeCount = 0;
    writeBufferStripeCount =
        MAX_VOLUME_COUNT   // in volume_list.h
        + MAX_VOLUME_COUNT // for GC stripe
        + 1;               // for Rebuild
    uint64_t minSize = 0;
    minSize =
        ArrayConfig::META_NVM_SIZE + (writeBufferStripeSize * writeBufferStripeCount);
    return minSize;
}

int
ArrayDeviceManager::_CheckSsdsCapacity(const ArrayDeviceSet& devSet)
{
    ArrayDevice* baseline = _GetBaseline(devSet.data);
    uint64_t capacityBytes = baseline->GetUblock()->GetSize();

    if (capacityBytes < ArrayConfig::MINIMUM_SSD_SIZE_BYTE || capacityBytes > ArrayConfig::MAXIMUM_SSD_SIZE_BYTE)
    {
        uint32_t eventId = (uint32_t)POS_EVENT_ID::ARRAY_SSD_CAPACITY_ERROR;
        POS_TRACE_ERROR(eventId,
            "SSD capacity is not valid. Valid capacity is from {}GB to {}TB",
            ArrayConfig::MINIMUM_SSD_SIZE_BYTE / SIZE_GB,
            ArrayConfig::MAXIMUM_SSD_SIZE_BYTE / SIZE_TB);
        return eventId;
    }
    for (ArrayDevice* dev : devSet.data)
    {
        if (ArrayDeviceState::NORMAL == dev->GetState() && capacityBytes != dev->GetUblock()->GetSize())
        {
            int eventId = (int)POS_EVENT_ID::ARRAY_SSD_SAME_CAPACITY_ERROR;
            POS_TRACE_WARN(eventId, "SSDs must be the same sizes");
            return eventId;
        }
    }
    for (ArrayDevice* dev : devSet.spares)
    {
        if (capacityBytes != dev->GetUblock()->GetSize())
        {
            int eventId = (int)POS_EVENT_ID::ARRAY_SSD_SAME_CAPACITY_ERROR;
            POS_TRACE_WARN(eventId, "SSDs must be the same sizes");
            return eventId;
        }
    }

    return 0;
}

tuple<ArrayDevice*, ArrayDeviceType>
ArrayDeviceManager::GetDev(UblockSharedPtr uBlock)
{
    if (uBlock == nullptr)
    {
        return make_tuple(nullptr, ArrayDeviceType::NONE);
    }
    DeviceSet<ArrayDevice*> _d = devs_->GetDevs();
    for (ArrayDevice* dev : _d.nvm)
    {
        if (dev->GetUblock() == uBlock)
        {
            return make_tuple(dev, ArrayDeviceType::NVM);
        }
    }
    for (ArrayDevice* dev : _d.data)
    {
        if (dev->GetUblock() == uBlock)
        {
            return make_tuple(dev, ArrayDeviceType::DATA);
        }
    }
    for (ArrayDevice* dev : _d.spares)
    {
        if (dev->GetUblock() == uBlock)
        {
            return make_tuple(dev, ArrayDeviceType::SPARE);
        }
    }
    return make_tuple(nullptr, ArrayDeviceType::NONE);
}

tuple<ArrayDevice*, ArrayDeviceType>
ArrayDeviceManager::GetDev(string devSn)
{
    DevUid sn(devSn);
    UblockSharedPtr dev = sysDevMgr_->GetDev(sn);
    return GetDev(dev);
}

ArrayDevice*
ArrayDeviceManager::_GetBaseline(const vector<ArrayDevice*>& devs)
{
    ArrayDevice* baseline = nullptr;
    for (ArrayDevice* dev : devs)
    {
        if (ArrayDeviceState::FAULT != dev->GetState())
        {
            baseline = dev;
            break;
        }
    }
    if (nullptr == baseline)
    {
        assert(0);
    }

    return baseline;
}

void
ArrayDeviceManager::SetArrayDeviceList(ArrayDeviceList* arrayDeviceList)
{
    this->devs_ = arrayDeviceList;
}
} // namespace pos

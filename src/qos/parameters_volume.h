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

#pragma once
#include <map>

#include "src/qos/parameters_reactor.h"

namespace pos
{
/* --------------------------------------------------------------------------*/
/**
 * @Synopsis
 *
 */
/* --------------------------------------------------------------------------*/
class VolumeParameter
{
public:
    VolumeParameter(void);
    ~VolumeParameter(void);
    void Reset(void);
    void IncreaseBandwidth(uint64_t bw);
    void IncreaseIops(uint64_t iops);
    uint64_t GetBandwidth(void);
    uint64_t GetIops(void);
    void SetAvgBandwidth(uint64_t bw);
    void SetAvgIops(uint64_t iops);
    uint64_t GetAvgBandwidth(void);
    uint64_t GetAvgIops(void);

    bool IsReactorExists(uint32_t reactor);
    ReactorParameter& GetReactorParameter(uint32_t reactor);
    void InsertReactorParameter(uint32_t reactor, const ReactorParameter& reactorParameter);
    std::map<uint32_t, ReactorParameter>& GetReactorParameterMap(void);

private:
    std::map<uint32_t, ReactorParameter> reactorParameterMap;
    uint64_t bandwidth;
    uint64_t avgBandwidth;
    uint64_t avgIops;
    uint64_t iops;
};
} // namespace pos

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

#include <unistd.h>

#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "spdk/bdev.h"
#include "spdk/bdev_module.h"
#include "spdk/env.h"
#include "spdk/event.h"
#include "spdk/ioat.h"
#include "spdk/stdinc.h"
#include "src/lib/singleton.h"

using SpdkNvmfStartCompletedCallback_t = std::function<int(void)>;

using namespace std;

namespace pos
{
class Spdk
{
public:
    Spdk();
    ~Spdk(void);

    bool Init(int argc, char** argv);
    void Finalize();

    static void SetStartCompletedHandler(SpdkNvmfStartCompletedCallback_t handler);
    static SpdkNvmfStartCompletedCallback_t GetStartCompletedHandler();

private:
    std::thread* spdkThread;
    static SpdkNvmfStartCompletedCallback_t startCompletedHandler;
    static std::atomic<bool> spdkInitialized;

    static void _InitWorker(int argc, char** argv);
    static void _AppStartedCallback(void* arg1);
    static int
    _AppParseCallback(int ch, char* arg)
    {
        return 0;
    }
    static void
    _AppUsageCallback(void)
    {
    }
};

using SpdkSingleton = Singleton<Spdk>;

} // namespace pos

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

#include "get_journal_status_wbt_command.h"

#include <string>

#include "src/journal_service/journal_service.h"

namespace pos
{
GetJournalStatusWbtCommand::GetJournalStatusWbtCommand(void)
: WbtCommand(GET_JOURNAL_STATUS, "get_journal_status")
{
}

GetJournalStatusWbtCommand::~GetJournalStatusWbtCommand(void)
{
}

int
GetJournalStatusWbtCommand::Execute(Args& argv, JsonElement& elem)
{
    JsonElement journalStatusElem("journalStatus");
    std::string arrayName = _GetParameter(argv, "array");

    bool isEnabled = JournalServiceSingleton::Instance()->IsEnabled(arrayName);
    journalStatusElem.SetAttribute(JsonAttribute("enabled", std::to_string(isEnabled)));
    if (isEnabled == true)
    {
        IJournalStatusProvider* status = JournalServiceSingleton::Instance()->GetStatusProvider(arrayName);
        ElementList statusList = status->GetJournalStatus();

        for (auto element : statusList)
        {
            journalStatusElem.SetElement(element);
        }
    }

    elem.SetElement(journalStatusElem);
    return 0;
}

} // namespace pos

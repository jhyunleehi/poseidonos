#include <gmock/gmock.h>

#include <list>
#include <string>
#include <vector>

#include "src/rebuild/raid5_rebuild.h"

namespace pos
{
class MockRaid5Rebuild : public Raid5Rebuild
{
public:
    using Raid5Rebuild::Raid5Rebuild;
    MOCK_METHOD(bool, Read, (), (override));
    MOCK_METHOD(bool, Write, (uint32_t targetId, UbioSmartPtr ubio), (override));
    MOCK_METHOD(bool, Complete, (uint32_t targetId, UbioSmartPtr ubio), (override));
};

} // namespace pos

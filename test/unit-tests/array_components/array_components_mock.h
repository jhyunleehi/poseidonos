#include <gmock/gmock.h>

#include <list>
#include <string>
#include <vector>

#include "src/array_components/array_components.h"

namespace pos
{
class MockArrayComponents : public ArrayComponents
{
public:
    using ArrayComponents::ArrayComponents;
    MOCK_METHOD(int, Create, (DeviceSet<string> nameSet, string dataRaidType), (override));
    MOCK_METHOD(int, Load, (), (override));
    MOCK_METHOD(int, Mount, (), (override));
    MOCK_METHOD(int, Unmount, (), (override));
    MOCK_METHOD(int, Delete, (), (override));
    MOCK_METHOD(int, PrepareRebuild, (), (override));
    MOCK_METHOD(void, RebuildDone, (), (override));
    MOCK_METHOD(Array*, GetArray, (), (override));
};

} // namespace pos

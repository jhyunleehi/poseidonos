#include <gmock/gmock.h>

#include <list>
#include <string>
#include <vector>

#include "src/debug/memory_checker.h"

namespace pos
{
class MockMemoryChecker : public MemoryChecker
{
public:
    using MemoryChecker::MemoryChecker;
};

} // namespace pos

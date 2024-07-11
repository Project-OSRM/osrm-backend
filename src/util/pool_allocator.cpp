#include "util/pool_allocator.hpp"

namespace osrm::util
{
     std::vector<void*> Cleanup::allocated_blocks_;
        std::mutex Cleanup::mutex_;
} // namespace osrm::util
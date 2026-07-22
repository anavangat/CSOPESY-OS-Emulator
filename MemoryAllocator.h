#pragma once
#include "IMemoryAllocator.h"
#include <vector>
#include <unordered_map>
#include <mutex>

class MemoryAllocator : public IMemoryAllocator {
public:
    MemoryAllocator(size_t maxOverallMem, MemoryAllocatorType type);
    ~MemoryAllocator() override = default;

    void* allocate(size_t size) override;
    void deallocate(void* ptr) override;
    std::string visualizeMemory() override;

private:
    std::unordered_map<void*, MemoryBlock> allocatedBlocks;
    std::vector<uint8_t> physicalMemoryPool;
    mutable std::mutex allocatorMutex;
};
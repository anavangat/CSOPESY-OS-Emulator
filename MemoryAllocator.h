#pragma once
#include "IMemoryAllocator.h"
#include <vector>
#include <unordered_map>
#include <mutex>

class MemoryAllocator : public IMemoryAllocator {
public:
    //(default 16 bytes)
    MemoryAllocator(size_t maxOverallMem, size_t frameSize = 16);
    ~MemoryAllocator() override = default;

    // Overridden IMemoryAllocator Pure Virtual Methods
    void* allocate(size_t size) override;
    void deallocate(void* ptr) override;
    std::string visualizeMemory() override;

private:
    size_t frameSize;
    size_t totalFrames;

    // Simulated contiguous Physical Memory RAM Pool
    std::vector<uint8_t> physicalMemoryPool;

    // Frame Table: Track physical frame state. Index = Frame ID, Value = Alloc ID (-1 if free)
    std::vector<int> frameTable;

    // Page Table: Maps returned Virtual Pointer Handle -> List of physical Frame IDs
    std::unordered_map<void*, std::vector<size_t>> pageTable;

    // Allocation Tracking: Maps Virtual Pointer Handle -> Requested size in bytes
    std::unordered_map<void*, size_t> allocationSizes;

    // Counter to uniquely identify allocations inside physical frames
    int nextAllocationId = 1;

    mutable std::mutex allocatorMutex;
};
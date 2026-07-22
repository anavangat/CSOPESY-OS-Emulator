#include "MemoryAllocator.h"
#include <sstream>
#include <iomanip>
#include <algorithm>

MemoryAllocator::MemoryAllocator(size_t maxOverallMem, MemoryAllocatorType type) {
    maximumSize = maxOverallMem;
    currentAllocatedSize = 0;
    memoryAllocatorType = type;

    // Allocate continuous backing byte buffer for simulated physical RAM
    physicalMemoryPool.resize(maximumSize, 0);
}

void* MemoryAllocator::allocate(size_t size) {
    std::lock_guard<std::mutex> lock(allocatorMutex);

    if (size == 0 || (currentAllocatedSize + size) > maximumSize) {
        return nullptr; // Out of memory or invalid request size
    }

    // Collect and sort all currently occupied memory blocks by starting address
    std::vector<MemoryBlock> occupiedBlocks;
    for (const auto& pair : allocatedBlocks) {
        occupiedBlocks.push_back(pair.second);
    }
    std::sort(occupiedBlocks.begin(), occupiedBlocks.end());

    // First-Fit search for a suitable gap in the physical memory pool
    size_t candidateStart = 0;

    for (const auto& block : occupiedBlocks) {
        if (block.start - candidateStart >= size) {
            break; // Found a large enough gap!
        }
        candidateStart = block.start + block.size;
    }

    // Check trailing space at the end of memory
    if (candidateStart + size > maximumSize) {
        return nullptr; // No contiguous block found large enough for this size
    }

    // Convert candidate offset to a real simulated pointer address
    void* allocatedPtr = static_cast<void*>(physicalMemoryPool.data() + candidateStart);

    // Save allocation record
    MemoryBlock newBlock{ candidateStart, size };
    allocatedBlocks[allocatedPtr] = newBlock;
    currentAllocatedSize += size;

    return allocatedPtr;
}

void MemoryAllocator::deallocate(void* ptr) {
    std::lock_guard<std::mutex> lock(allocatorMutex);

    if (ptr == nullptr) return;

    auto it = allocatedBlocks.find(ptr);
    if (it != allocatedBlocks.end()) {
        currentAllocatedSize -= it->second.size;
        allocatedBlocks.erase(it);
    }
}

std::string MemoryAllocator::visualizeMemory() {
    std::lock_guard<std::mutex> lock(allocatorMutex);

    std::stringstream ss;
    ss << "---- Memory Visualization ----\n";
    ss << "Type: " << (memoryAllocatorType == PAGING ? "PAGING" : "FLAT_MEMORY_ALLOCATOR") << "\n";
    ss << "Allocated: " << currentAllocatedSize << " / " << maximumSize << " Bytes\n";

    std::vector<MemoryBlock> blocks;
    for (const auto& pair : allocatedBlocks) {
        blocks.push_back(pair.second);
    }
    std::sort(blocks.begin(), blocks.end());

    for (const auto& block : blocks) {
        ss << "Block [" << std::setw(6) << block.start << " - " 
           << std::setw(6) << (block.start + block.size - 1) 
           << "] Size: " << block.size << " Bytes\n";
    }

    ss << "------------------------------\n";
    return ss.str();
}
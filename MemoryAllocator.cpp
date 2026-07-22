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
        return nullptr; // Out of memory
    }

    std::vector<MemoryBlock> occupiedBlocks;
    for (const auto& pair : allocatedBlocks) {
        occupiedBlocks.push_back(pair.second);
    }
    std::sort(occupiedBlocks.begin(), occupiedBlocks.end());

    size_t candidateStart = 0;

    for (const auto& block : occupiedBlocks) {
        if (block.start - candidateStart >= size) {
            break; // g kasya pa
        }
        candidateStart = block.start + block.size;
    }

    if (candidateStart + size > maximumSize) {
        return nullptr; // walang kasya
    }

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
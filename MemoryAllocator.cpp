#include "MemoryAllocator.h"
#include <sstream>
#include <iomanip>
#include <algorithm>

MemoryAllocator::MemoryAllocator(size_t maxOverallMem, size_t frameSize)
    : frameSize(frameSize) 
{
    maximumSize = maxOverallMem;
    currentAllocatedSize = 0;
    memoryAllocatorType = PAGING; //basta paging

    // Allocate physical memory pool byte array
    physicalMemoryPool.resize(maximumSize, 0);

    // Calculate frame count and initialize Frame Table
    totalFrames = maximumSize / frameSize;
    frameTable.assign(totalFrames, -1); // -1 signifies a free frame
}

void* MemoryAllocator::allocate(size_t size) {
    std::lock_guard<std::mutex> lock(allocatorMutex);

    if (size == 0 || (currentAllocatedSize + size) > maximumSize) {
        return nullptr; // Out of memory or invalid size
    }

    //Convert requested size in bytes into total physical frames needed
    size_t framesNeeded = (size + frameSize - 1) / frameSize;

    //Scan global Frame Table to collect ALL available free physical frame indices
    std::vector<size_t> freeFrameIndices;
    for (size_t i = 0; i < totalFrames; ++i) {
        if (frameTable[i] == -1) {
            freeFrameIndices.push_back(i);
        }
    }

    //Verify total free physical frames are sufficient (does NOT need to be contiguous!)
    if (freeFrameIndices.size() < framesNeeded) {
        return nullptr; // Insufficient physical frames available
    }

    //Claim the required frames (scattered non-contiguously across physical memory)
    std::vector<size_t> assignedFrames;
    int allocId = nextAllocationId++;

    for (size_t i = 0; i < framesNeeded; ++i) {
        size_t targetFrame = freeFrameIndices[i];
        frameTable[targetFrame] = allocId;
        assignedFrames.push_back(targetFrame);
    }

    void* ptr = static_cast<void*>(physicalMemoryPool.data() + (assignedFrames[0] * frameSize));

    pageTable[ptr] = assignedFrames;
    allocationSizes[ptr] = size;
    currentAllocatedSize += size;

    return ptr;
}

// -----------------------------------------------------------------------------
// FULL PAGING DEALLOCATION ALGORITHM
// -----------------------------------------------------------------------------
void MemoryAllocator::deallocate(void* ptr) {
    std::lock_guard<std::mutex> lock(allocatorMutex);

    if (ptr == nullptr) return;

    auto pageIt = pageTable.find(ptr);
    if (pageIt == pageTable.end()) return; // Invalid/unknown pointer

    // 1. Release assigned physical frames back to free status (-1)
    for (size_t frameIdx : pageIt->second) {
        frameTable[frameIdx] = -1;
    }

    // 2. Update memory statistics
    currentAllocatedSize -= allocationSizes[ptr];

    // 3. Remove entry mappings from Page Table
    pageTable.erase(pageIt);
    allocationSizes.erase(ptr);
}

// -----------------------------------------------------------------------------
// PAGING VISUALIZATION
// -----------------------------------------------------------------------------
std::string MemoryAllocator::visualizeMemory() {
    std::lock_guard<std::mutex> lock(allocatorMutex);

    std::stringstream ss;
    ss << "---- Full Paging Memory Visualization ----\n";
    ss << "Type: PAGING\n";
    ss << "Usage: " << currentAllocatedSize << " / " << maximumSize << " Bytes\n";
    ss << "Total Frames: " << totalFrames << " (Frame Size: " << frameSize << " Bytes)\n";
    ss << std::string(45, '-') << "\n";

    for (size_t i = 0; i < totalFrames; ++i) {
        size_t startAddr = i * frameSize;
        size_t endAddr = startAddr + frameSize - 1;

        ss << "Frame " << std::setw(3) << std::setfill('0') << i << " ["
           << std::setw(5) << std::setfill('0') << startAddr << " - "
           << std::setw(5) << std::setfill('0') << endAddr << "] | ";

        if (frameTable[i] == -1) {
            ss << "Free Frame\n";
        } else {
            ss << "Occupied (Alloc ID: " << frameTable[i] << ")\n";
        }
    }

    ss << "------------------------------------------\n";
    return ss.str();
}
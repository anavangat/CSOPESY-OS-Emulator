#include "MemoryAllocator.h"
#include <sstream>
#include <set>
#include <iomanip>

MemoryAllocator::MemoryAllocator(int maxOverallMem, int memPerFrame, int memPerProc)
    : maxOverallMem(maxOverallMem), memPerFrame(memPerFrame), memPerProc(memPerProc) 
{
    totalFrames = maxOverallMem / memPerFrame;
    framesPerProcess = memPerProc / memPerFrame;
    frameTable.assign(totalFrames, -1);
}

bool MemoryAllocator::allocate(int pid) {
    std::lock_guard<std::mutex> lock(allocatorMutex);

    std::vector<int> freeFrameIndices;
    for (int i = 0; i < totalFrames; ++i) {
        if (frameTable[i] == -1) {
            freeFrameIndices.push_back(i);
        }
    }

    if (static_cast<int>(freeFrameIndices.size()) < framesPerProcess) {
        return false; // Not enough physical frames available
    }

    for (int i = 0; i < framesPerProcess; ++i) {
        int targetFrame = freeFrameIndices[i];
        frameTable[targetFrame] = pid;
    }

    return true;
}

void MemoryAllocator::deallocate(int pid) {
    std::lock_guard<std::mutex> lock(allocatorMutex);

    for (int i = 0; i < totalFrames; ++i) {
        if (frameTable[i] == pid) {
            frameTable[i] = -1; // Release frame back to free pool
        }
    }
}

bool MemoryAllocator::isAllocated(int pid) const {
    std::lock_guard<std::mutex> lock(allocatorMutex);
    for (int i = 0; i < totalFrames; ++i) {
        if (frameTable[i] == pid) return true;
    }
    return false;
}

int MemoryAllocator::getProcessCount() const {
    std::lock_guard<std::mutex> lock(allocatorMutex);
    std::set<int> distinctPIDs;
    for (int pid : frameTable) {
        if (pid != -1) distinctPIDs.insert(pid);
    }
    return static_cast<int>(distinctPIDs.size());
}

int MemoryAllocator::getExternalFragmentation() const {
    return 0; 
}

std::string MemoryAllocator::generateMemoryStamp(const std::string& timestamp) const {
    std::lock_guard<std::mutex> lock(allocatorMutex);

    std::stringstream ss;
    ss << "----start----\n";
    ss << "Timestamp: " << timestamp << "\n";
    ss << "Process Count: " << getProcessCount() << "\n";
    ss << "External Fragmentation: 0 KB (Paging Scheme)\n";
    ss << std::string(40, '-') << "\n";

    for (int i = 0; i < totalFrames; ++i) {
        int startAddress = i * memPerFrame;
        int endAddress = ((i + 1) * memPerFrame) - 1;

        ss << "Frame " << std::setw(4) << std::setfill('0') << i << " ["
           << std::setw(5) << std::setfill('0') << startAddress << " - " 
           << std::setw(5) << std::setfill('0') << endAddress << "] | ";

        if (frameTable[i] == -1) {
            ss << "Free Frame\n";
        } else {
            ss << "PID: " << frameTable[i] << "\n";
        }
    }
    ss << "----end----\n";
    return ss.str();
}
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

    int contiguousFreeCount = 0;
    int startFrameIndex = -1;

    for (int i = 0; i < totalFrames; ++i) {
        if (frameTable[i] == -1) {
            if (contiguousFreeCount == 0) {
                startFrameIndex = i;
            }
            contiguousFreeCount++;

            if (contiguousFreeCount == framesPerProcess) {
                for (int j = startFrameIndex; j < startFrameIndex + framesPerProcess; ++j) {
                    frameTable[j] = pid;
                }
                return true;
            }
        } else {
            contiguousFreeCount = 0;
            startFrameIndex = -1;
        }
    }

    return false;
}

void MemoryAllocator::deallocate(int pid) {
    std::lock_guard<std::mutex> lock(allocatorMutex);

    for (int i = 0; i < totalFrames; ++i) {
        if (frameTable[i] == pid) {
            frameTable[i] = -1;
        }
    }
}

bool MemoryAllocator::isAllocated(int pid) const {
    std::lock_guard<std::mutex> lock(allocatorMutex);

    for (int i = 0; i < totalFrames; ++i) {
        if (frameTable[i] == pid) {
            return true;
        }
    }
    return false;
}

int MemoryAllocator::getProcessCount() const {
    std::lock_guard<std::mutex> lock(allocatorMutex);

    std::set<int> distinctPIDs;
    for (int pid : frameTable) {
        if (pid != -1) {
            distinctPIDs.insert(pid);
        }
    }
    return static_cast<int>(distinctPIDs.size());
}

int MemoryAllocator::getExternalFragmentation() const {
    std::lock_guard<std::mutex> lock(allocatorMutex);

    int externalFragmentationFrames = 0;
    int currentFreeBlockSize = 0;

    for (int i = 0; i < totalFrames; ++i) {
        if (frameTable[i] == -1) {
            currentFreeBlockSize++;
        } else {
            if (currentFreeBlockSize > 0 && currentFreeBlockSize < framesPerProcess) {
                externalFragmentationFrames += currentFreeBlockSize;
            }
            currentFreeBlockSize = 0;
        }
    }
    if (currentFreeBlockSize > 0 && currentFreeBlockSize < framesPerProcess) {
        externalFragmentationFrames += currentFreeBlockSize;
    }

    return externalFragmentationFrames * memPerFrame;
}

std::string MemoryAllocator::generateMemoryStamp(const std::string& timestamp) const {
    std::lock_guard<std::mutex> lock(allocatorMutex);

    std::stringstream ss;
    ss << "----start----\n";
    ss << "Timestamp: " << timestamp << "\n";
    ss << "Process Count: " << getProcessCount() << "\n";
    ss << "External Fragmentation: " << getExternalFragmentation() << " KB\n";
    ss << std::string(40, '-') << "\n";

    int blockStart = 0;
    while (blockStart < totalFrames) {
        int currentOwner = frameTable[blockStart];
        int blockEnd = blockStart;

        while (blockEnd + 1 < totalFrames && frameTable[blockEnd + 1] == currentOwner) {
            blockEnd++;
        }

        int startAddress = blockStart * memPerFrame;
        int endAddress = ((blockEnd + 1) * memPerFrame) - 1;

        ss << "Address: " << std::setw(5) << std::setfill('0') << startAddress 
           << " - " << std::setw(5) << std::setfill('0') << endAddress << " | ";
        
        if (currentOwner == -1) {
            ss << "Free Block\n";
        } else {
            ss << "PID: " << currentOwner << "\n";
        }

        blockStart = blockEnd + 1;
    }
    ss << "----end----\n";
    return ss.str();
}
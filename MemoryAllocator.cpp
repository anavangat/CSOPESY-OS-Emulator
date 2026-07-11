#include "MemoryAllocator.h"
#include <sstream>
#include <set>

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

int MemoryAllocator::getProcessCountUnlocked() const {
    std::set<int> distinctPIDs;
    for (int pid : frameTable) {
        if (pid != -1) {
            distinctPIDs.insert(pid);
        }
    }
    return static_cast<int>(distinctPIDs.size());
}

int MemoryAllocator::getExternalFragmentationUnlocked() const {
    // Every allocation in this project is a fixed 'memPerProc' block, so
    // there's no "usable hole vs. wasted sliver" distinction to make here:
    // ANY byte not currently owned by a resident process counts as
    // external fragmentation. (Verified against the assignment's own
    // worked example: two 4096-byte holes, each individually big enough
    // to fit another process, are BOTH counted -> 8192 total, not 0.)
    int freeFrames = 0;
    for (int pid : frameTable) {
        if (pid == -1) {
            freeFrames++;
        }
    }
    return freeFrames * memPerFrame;
}

int MemoryAllocator::getProcessCount() const {
    std::lock_guard<std::mutex> lock(allocatorMutex);
    return getProcessCountUnlocked();
}

int MemoryAllocator::getExternalFragmentation() const {
    std::lock_guard<std::mutex> lock(allocatorMutex);
    return getExternalFragmentationUnlocked();
}

// int MemoryAllocator::getProcessCount() const {
//     std::lock_guard<std::mutex> lock(allocatorMutex);

//     std::set<int> distinctPIDs;
//     for (int pid : frameTable) {
//         if (pid != -1) {
//             distinctPIDs.insert(pid);
//         }
//     }
//     return static_cast<int>(distinctPIDs.size());
// }

// int MemoryAllocator::getExternalFragmentation() const {
//     std::lock_guard<std::mutex> lock(allocatorMutex);

//     int externalFragmentationFrames = 0;
//     int currentFreeBlockSize = 0;

//     for (int i = 0; i < totalFrames; ++i) {
//         if (frameTable[i] == -1) {
//             currentFreeBlockSize++;
//         } else {
//             if (currentFreeBlockSize > 0 && currentFreeBlockSize < framesPerProcess) {
//                 externalFragmentationFrames += currentFreeBlockSize;
//             }
//             currentFreeBlockSize = 0;
//         }
//     }
//     if (currentFreeBlockSize > 0 && currentFreeBlockSize < framesPerProcess) {
//         externalFragmentationFrames += currentFreeBlockSize;
//     }

//     return externalFragmentationFrames * memPerFrame;
// }

// std::string MemoryAllocator::generateMemoryStamp(const std::string& timestamp) const {
//     std::lock_guard<std::mutex> lock(allocatorMutex);

//     std::stringstream ss;
//     ss << "----start----\n";
//     ss << "Timestamp: " << timestamp << "\n";
//     ss << "Process Count: " << getProcessCount() << "\n";
//     ss << "External Fragmentation: " << getExternalFragmentation() << " KB\n";
//     ss << std::string(40, '-') << "\n";

//     int blockStart = 0;
//     while (blockStart < totalFrames) {
//         int currentOwner = frameTable[blockStart];
//         int blockEnd = blockStart;

//         while (blockEnd + 1 < totalFrames && frameTable[blockEnd + 1] == currentOwner) {
//             blockEnd++;
//         }

//         int startAddress = blockStart * memPerFrame;
//         int endAddress = ((blockEnd + 1) * memPerFrame) - 1;

//         ss << "Address: " << std::setw(5) << std::setfill('0') << startAddress 
//            << " - " << std::setw(5) << std::setfill('0') << endAddress << " | ";
        
//         if (currentOwner == -1) {
//             ss << "Free Block\n";
//         } else {
//             ss << "PID: " << currentOwner << "\n";
//         }

//         blockStart = blockEnd + 1;
//     }
//     ss << "----end----\n";
//     return ss.str();
// }

std::string MemoryAllocator::generateMemoryStamp(const std::string& timestamp) const {
    std::lock_guard<std::mutex> lock(allocatorMutex);

    std::vector<std::string> sections;

    // Section 1: header block (3 lines, no blank line between them)
    std::stringstream header;
    header << "Timestamp: " << timestamp << "\n";
    header << "Number of processes in memory: " << getProcessCountUnlocked() << "\n";
    header << "Total external fragmentation in KB: " << getExternalFragmentationUnlocked();
    sections.push_back(header.str());

    // Section 2: top-of-memory marker
    sections.push_back("----end---- = " + std::to_string(maxOverallMem));

    // Sections 3..n: one section per resident process, walked from the
    // highest address down to the lowest. Free runs are never printed
    // directly -- their size is implied by the gap between two printed
    // boundary numbers (or between a marker and the nearest block), same
    // as in the assignment's reference mockup.
    int frameIndex = totalFrames - 1;
    while (frameIndex >= 0) {
        int owner = frameTable[frameIndex];
        int runEnd = frameIndex; // highest frame index in this run

        while (frameIndex - 1 >= 0 && frameTable[frameIndex - 1] == owner) {
            frameIndex--;
        }
        int runStart = frameIndex; // lowest frame index in this run

        if (owner != -1) {
            int upperAddress = (runEnd + 1) * memPerFrame;
            int lowerAddress = runStart * memPerFrame;

            std::stringstream block;
            block << upperAddress << "\n";
            block << "P" << owner << "\n";
            block << lowerAddress;
            sections.push_back(block.str());
        }

        frameIndex = runStart - 1;
    }

    // Final section: bottom-of-memory marker
    sections.push_back("----start----- = 0");

    std::stringstream out;
    for (size_t i = 0; i < sections.size(); ++i) {
        out << sections[i];
        if (i + 1 < sections.size()) {
            out << "\n\n";
        }
    }
    out << "\n";
    return out.str();
}
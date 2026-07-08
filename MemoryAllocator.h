#pragma once
#include <vector>
#include <string>
#include <mutex>

class MemoryAllocator {
    public:
        MemoryAllocator(int maxOverallMem, int memPerFrame, int memPerProc);
        ~MemoryAllocator() = default;

        bool allocate(int pid);
        void deallocate(int pid);
        bool isAllocated(int pid) const;

        int getProcessCount() const;
        int getExternalFragmentation() const;
        std::string generateMemoryStamp(const std::string& timeStamp) const;
    
    private:
        int maxOverallMem;
        int memPerFrame;
        int memPerProc;

        int totalFrames;
        int framesPerProcess;
        
        std::vector<int> frameTable;
        mutable std::mutex allocatorMutex; // Mutex for thread safety
};
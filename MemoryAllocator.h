#pragma once
#include <vector>
#include <string>
#include <mutex>
#include <unordered_map>
#include <queue>
#include <cstdint>

class MemoryAllocator {
    public:
        MemoryAllocator(int maxOverallMem, int memPerFrame);
        ~MemoryAllocator() = default;

        int getMaxOverallMem() const;
        int getUsedMemory() const;
        int getFreeMemory() const;
        int getTotalFrames() const;
        int getOccupiedFrames() const;

        int getNumPagedIn() const { return numPagedIn; }
        int getNumPagedOut() const { return numPagedOut; }

        bool allocate(int pid, int memoryRequired);
        void deallocate(int pid);

        bool read(int pid, int virtualAddress, uint16_t& value);
        bool write(int pid, int virtualAddress, uint16_t value);
        


        bool isAllocated(int pid) const;

        int getProcessCount() const;
        int getExternalFragmentation() const;
        std::string generateMemoryStamp(const std::string& timeStamp) const;
    
    private:
        int maxOverallMem;
        int memPerFrame;
        //int memPerProc;
        uint64_t accessCounter = 0;

        int totalFrames;

        int getProcessCountUnlocked() const;
        int getExternalFragmentationUnlocked() const;

		struct PageEntry { // Page table entry for each process
			bool valid = false; // Indicates if the page is valid (allocated to RAM and can be accessed)
			bool dirty = false; // Indicates if the page has been modified (written to) since it was loaded into RAM
			int frameNumber = -1; // The frame number in RAM where the page is stored
			int backingStoreRecordNumber = -1; // The index in the backing store (disk) where the page is stored when not in RAM

        };

		std::unordered_map<int, std::vector<PageEntry>> pageTables; // Maps process IDs to their page tables

        struct Frame {
			bool occupied = false; //Checks if the frame is occupied
			int pid = -1;// checks which process is occupying the frame
			int pageNumber = -1; //which page of the process is occupying the frame
			//bool dirty = false; // checks if page which is occupying the frame is dirty or not // page is dirty if it has been modified since it was loaded into memory, and clean if it has not been modified and exactly the same as the page on disk 
        
			uint64_t lastAccessedTick = 0; // The tick at which the frame was last accessed (for LRU replacement)
        };
        
        std::vector<Frame> frameTable;

		struct BackingStoreRecord // information about the pages stored in the backing store (disk) for each process
        {
			int recordNumber; // The index in the backing store (disk) where the page is stored when not in RAM
			bool inUse; // Indicates if the record index is currently in use (allocated to a process)
			int pid; // id of the process that owns this page
			int pageNumber; // page number of the page in the backing store
			//std::string pageType; // type of the page (code, data, stack, etc.)
			std::vector<uint8_t> data; // The actual data of the page stored in the backing store
        };

		std::vector<BackingStoreRecord> backingStore; // Vector to store the backing store records


        struct BackingStoreIndexEntry { // Represents an entry in the backing store for a page

			int recordNumber; // The index of the record in the backing store
        };

		std::unordered_map<int, std::unordered_map <int, BackingStoreIndexEntry>> backingStoreIndex; // Maps process IDs to their backing store index entries // checks process id and page number to find the record number in the backing store

		std::queue<int> freeFrames; // Queue of free frame numbers for allocation

		int numPagedIn = 0; // Number of page ins to RAM performed
		int numPagedOut = 0; // Number of page outs to backing store performed

        std::vector<uint8_t> physicalMemory; // Simulated physical memory as a vector of bytes

        mutable std::mutex allocatorMutex; // Mutex for thread safety

		int findFreeFrame(); // Finds a free frame in the frame table and returns its index, or -1 if no free frame is available
		int selectVictimFrame();// Selects a victim frame for page replacement using a page replacement algorithm (e.g., LRU) and returns its index

		int pageIn(int pid, int pageNumber); // Page in the page for the specified process and page number from the backing store to RAM
		void pageOut(int frameNumber); // Page out the page in the specified frame to the backing store

        int translateAddress(int pid, int virtualAddress);

		const std::string backingStoreFileName = "csopesy-backing-store.txt"; // Name of the backing store file

		void writePageToFile(int recordNumber, int recordPid, int pageNumber, const std::vector<uint8_t>& data); // Writes the page data to the backing store file at the specified record number

		bool readPageFromFile(int recordNumber, std::vector<uint8_t>& data); // Reads the page data from the backing store file at the specified record number

		int recordLineWidth() const; // Returns the number of bytes per line in the backing store file
};
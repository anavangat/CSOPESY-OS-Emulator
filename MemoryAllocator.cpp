#include "MemoryAllocator.h"
#include <sstream>
#include <set>
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <cstdio>

MemoryAllocator::MemoryAllocator(int maxOverallMem, int memPerFrame)
    : maxOverallMem(maxOverallMem), memPerFrame(memPerFrame), totalFrames (maxOverallMem / memPerFrame), accessCounter(0)
{
    
    //framesPerProcess = memPerProc / memPerFrame;

    //frameTable.assign(totalFrames, -1);

	frameTable.resize(totalFrames); // Initialize frameTable with totalFrames elements, all set to -1 (indicating free frames)
	physicalMemory.resize(maxOverallMem, 0); // Initialize physicalMemory with maxOverallMem bytes, all set to 0

    for (int i = 0; i < totalFrames; i++)
		freeFrames.push(i); // Initialize the freeFrames list with all frame numbers (0 to totalFrames - 1)

	std::ofstream freshFile(backingStoreFileName, std::ios::binary |  std::ios::trunc); // Create a new empty backing store file (truncate if it already exists)
	freshFile.close(); // Close the file after creating it

	backingStoreFile.open(backingStoreFileName, std::ios::binary | std::ios::in | std::ios::out); // Hold the handle open; every record access seeks within it instead of reopening

    
}

static const int PID_FIELD_WIDTH = 6; // supports PIDs up to 999999 before the field overflows
static const int PAGE_FIELD_WIDTH = 4; // supports page numbers up to 9999 before overflowing
static const int CHARS_PER_BYTE = 4; // each byte -> "NNN " (3 digits + 1 space)


int MemoryAllocator::recordLineWidth() const {  
    return 4 + PID_FIELD_WIDTH + 1      // "PID:" + digits + space
        + 5 + PAGE_FIELD_WIDTH + 1       // "PAGE:" + digits + space
        + 5 + (memPerFrame * CHARS_PER_BYTE) // "DATA:" + byte values
        + 1;                              // trailing newline
}


void MemoryAllocator::writePageToFile(int recordNumber, int recordPid, int pageNumber, const std::vector<uint8_t>& data) {

    if (recordNumber < 0)
        return; // Invalid record number, do nothing

    if (!backingStoreFile.is_open()) {

		std::ofstream createFile(backingStoreFileName, std::ios::binary); // Create the file if it doesn't exist
        createFile.close();

		backingStoreFile.open(backingStoreFileName, std::ios::binary | std::ios::in | std::ios::out); // Reopen the file after creating it

		if (!backingStoreFile.is_open()) {
			return; // Handle error: unable to open or create the backing store file
		}
    }

	backingStoreFile.clear(); // A prior short read past EOF leaves failbit set on the shared handle, which would silently discard this write
	std::fstream& file = backingStoreFile;

    std::vector<uint8_t> source(data); // Copy so we can safely pad without mutating the caller's buffer
    if (static_cast<int>(source.size()) < memPerFrame)
        source.resize(memPerFrame, 0); // defensive pad, same intent as the old binary version

    // Build the human-readable line, e.g.: "PID:     1 PAGE:   0 DATA:007 042 255 ...\n"
    std::ostringstream line;
    line << "PID:" << std::setw(PID_FIELD_WIDTH) << recordPid
        << " PAGE:" << std::setw(PAGE_FIELD_WIDTH) << pageNumber
        << " DATA:";

    char byteBuf[8];
    for (int i = 0; i < memPerFrame; i++) {
        std::snprintf(byteBuf, sizeof(byteBuf), "%03d ", source[i]); // e.g. 255 -> "255 ", 7 -> "007 "
        line << byteBuf;
    }

    std::string lineStr = line.str();
    int lineWidth = recordLineWidth();

    // Pad or (defensively) truncate so every line is exactly lineWidth characters —
    // this fixed width is what makes recordNumber * lineWidth a reliable seek offset.
    if (static_cast<int>(lineStr.size()) < lineWidth - 1)
        lineStr.append(lineWidth - 1 - lineStr.size(), ' ');
    lineStr += "\n";
    if (static_cast<int>(lineStr.size()) > lineWidth)
        lineStr.resize(lineWidth); // PID/page overflowed its field width; truncate rather than corrupt the layout

    std::streamoff offset = static_cast<std::streamoff>(recordNumber) * lineWidth; // Calculate the byte offset in the file for the given record number
    file.seekp(offset); // Move the write pointer to the calculated offset

    file.write(lineStr.data(), lineWidth); // Write the fixed-width text line to the file

    file.flush(); // Push the record to disk so the file stays inspectable, no need to reopen

}

bool MemoryAllocator::readPageFromFile(int recordNumber, std::vector<uint8_t>& data) {

	data.assign(memPerFrame, 0); // Initialize the data vector with memPerFrame bytes, all set to 0

	if (recordNumber < 0)
		return false; // Invalid record number, return false)

	if (!backingStoreFile.is_open()) {
		return false; // Handle error: unable to open the backing store file
	}

	backingStoreFile.clear(); // Clear any eof/fail bits left by an earlier short read before seeking on the shared handle
	std::fstream& file = backingStoreFile;

    int lineWidth = recordLineWidth();
    std::streamoff offset = static_cast<std::streamoff>(recordNumber) * lineWidth; // Calculate the byte offset in the file for the given record number

    std::vector<char> raw(lineWidth, '0'); // default '0' so an unwritten record decodes to all-zero bytes
    file.seekg(offset); // Move the read pointer to the calculated offset
    file.read(raw.data(), lineWidth); // Read the fixed-width text line from the file
    // A short/failed read (record's offset is past current EOF) just leaves the tail as '0' chars.

    file.clear(); // Drop the failbit a short read just set, so the next write on this handle isn't silently dropped

    std::string lineStr(raw.begin(), raw.end());
    size_t dataPos = lineStr.find("DATA:");
    if (dataPos == std::string::npos)
        return true; // nothing written at this offset yet; data stays zero-filled from the assign() above

    size_t cursor = dataPos + 5; // skip past the "DATA:" label itself
    for (int i = 0; i < memPerFrame; i++) {
        if (cursor + 3 > lineStr.size())
            break;
        char digits[4] = { lineStr[cursor], lineStr[cursor + 1], lineStr[cursor + 2], '\0' };
        data[i] = static_cast<uint8_t>(std::atoi(digits)); // decode "NNN" back into one byte
        cursor += 4; // 3 digits + 1 separating space
    }

    return true;

}
bool MemoryAllocator::allocate(int pid, int memoryRequired) {

	std::lock_guard<std::mutex> lock(allocatorMutex); // Lock the mutex to ensure thread safety during allocation

	if (pageTables.count(pid) > 0){ // Check if the process is already allocated
		return false; // Process is already allocated, return false
	}

	int pageCount = (memoryRequired + memPerFrame - 1) / memPerFrame; // Calculate the number of pages needed for the process

	if (pageCount > totalFrames) {
		return false; // The process needs more pages than physical memory has frames
	}

	pageTables[pid].resize(pageCount); // Initialize the page table for the process with the required number of pages)

	for(int page = 0; page < pageCount; page++){ 

		int recordNumber = -1; // Initialize the record number for the backing store

		for (int i = 0; i < backingStore.size(); i++) { // Search for a free record in the backing store
			if (!backingStore[i].inUse) { // If the record is not in use
				recordNumber = i; // Assign the record number
				break; // Exit the loop
			}
		}

		if (recordNumber == -1) { // If no free record was found in the backing store
            BackingStoreRecord record;

            record.recordNumber = static_cast<int>(backingStore.size()); // Assign the next available record number
			record.inUse = true; // Mark the record as in use
            record.pid = pid; // Set the process ID
            record.pageNumber = page; // Set the page number
            record.data.assign(memPerFrame, 0); // Initialize the page data with zeros

            backingStore.push_back(record); // Add the record to the backing store

			recordNumber = record.recordNumber; // Update the record number to the newly created record's number

		}
		else {
            // Reuse an existing record
            backingStore[recordNumber].inUse = true;
            backingStore[recordNumber].pid = pid;
            backingStore[recordNumber].pageNumber = page;
            backingStore[recordNumber].data.assign(memPerFrame, 0);
		}

		
		backingStoreIndex[pid][page].recordNumber = recordNumber; // Update the backing store index for the process and page


        pageTables[pid][page].valid = false;
        pageTables[pid][page].dirty = false;
        pageTables[pid][page].frameNumber = -1;
		pageTables[pid][page].backingStoreRecordNumber = recordNumber; // Update the page table entry with the backing store record number

    
		writePageToFile(recordNumber, pid, page, backingStore[recordNumber].data); // Write the page data to the backing store file)
    }

   

	return true; // Allocation successful

	
}

void MemoryAllocator::deallocate(int pid) {
    std::lock_guard<std::mutex> lock(allocatorMutex);

    for (auto& record : backingStore) // Iterate through the backing store record
    {
		if (record.pid == pid) // If the record belongs to the process being deallocated
		{
			record.inUse = false; // Mark the record as not in use
			record.pid = -1; // Reset the process ID
			record.pageNumber = -1; // Reset the page number
			//record.data.clear(); // Clear the data associated with the record
            record.data.assign(memPerFrame, 0);
			writePageToFile(record.recordNumber, -1, -1, record.data); // Write the cleared data to the backing store file)
		}
    }

	for (int i = 0; i < totalFrames; i++) // Iterate through the frame table
	{
		if (frameTable[i].occupied && frameTable[i].pid == pid) // If the frame is occupied by the process being deallocated
		{
			frameTable[i].occupied = false; // Mark the frame as not occupied
			frameTable[i].pid = -1; // Reset the process ID
			frameTable[i].pageNumber = -1; // Reset the page number
			freeFrames.push(i); // Add the frame back to the free frames queue
			frameTable[i].lastAccessedTick = 0; // Reset the last accessed tick
		}
	}
	pageTables.erase(pid); // Remove the page table for the process

	backingStoreIndex.erase(pid); // Remove the backing store index entries for the process)

}


int MemoryAllocator::translateAddress(int pid, int virtualAddress) {
	//std::lock_guard<std::mutex> lock(allocatorMutex); 
	auto ptIt = pageTables.find(pid); // Find the page table for the given process ID
	if (ptIt == pageTables.end()) // If the process ID is not found in the page tables
        return -1;

	int pageNumber = virtualAddress / memPerFrame; // Calculate the page number from the virtual address
	int offset = virtualAddress % memPerFrame; // Calculate the offset within the page

    std::vector<PageEntry>& pageTable = ptIt->second;

	if (pageNumber < 0 || pageNumber >= static_cast<int>(pageTable.size())) // Check if the page number is valid
        return -1;

    // Page fault
    if (!pageTable[pageNumber].valid)
    {
        if (pageIn(pid, pageNumber) == -1)
            return -1;
    }

    PageEntry& entry = pageTable[pageNumber];

    // Update LRU
    frameTable[entry.frameNumber].lastAccessedTick = ++accessCounter;

    return entry.frameNumber * memPerFrame + offset;
}

bool MemoryAllocator::read(int pid, int virtualAddress, uint16_t& value)
{
    std::lock_guard<std::mutex> lock(allocatorMutex);

    int lowAddress = translateAddress(pid, virtualAddress);
    if (lowAddress == -1)
        return false;
    uint8_t lowByte = physicalMemory[lowAddress];   // consume before the next fault

    int highAddress = translateAddress(pid, virtualAddress + 1);
    if (highAddress == -1)
        return false;
    uint8_t highByte = physicalMemory[highAddress];

    value = static_cast<uint16_t>(lowByte) | (static_cast<uint16_t>(highByte) << 8);
    return true;
}

void MemoryAllocator::markPageDirty(int pid, int pageNumber)
{
    auto ptIt = pageTables.find(pid);
    if (ptIt == pageTables.end())
        return;
    if (pageNumber < 0 || pageNumber >= static_cast<int>(ptIt->second.size()))
        return;

    PageEntry& entry = ptIt->second[pageNumber];
    entry.dirty = true;
    if (entry.frameNumber >= 0 && entry.frameNumber < totalFrames)
        frameTable[entry.frameNumber].lastAccessedTick = ++accessCounter;
}

bool MemoryAllocator::write(int pid, int virtualAddress, uint16_t value)
{
    std::lock_guard<std::mutex> lock(allocatorMutex);

    int lowAddress = translateAddress(pid, virtualAddress);
    if (lowAddress == -1)
        return false;
    physicalMemory[lowAddress] = static_cast<uint8_t>(value & 0xFF);
    markPageDirty(pid, virtualAddress / memPerFrame);   // MUST precede the next fault

    int highAddress = translateAddress(pid, virtualAddress + 1);
    if (highAddress == -1)
        return false;
    physicalMemory[highAddress] = static_cast<uint8_t>((value >> 8) & 0xFF);
    markPageDirty(pid, (virtualAddress + 1) / memPerFrame);

    return true;
}


int MemoryAllocator::findFreeFrame()
{
    if (freeFrames.empty())
		return -1; // No free frames available


	int frame = freeFrames.front(); // Get the first free frame from the queue
	freeFrames.pop(); // Remove the frame from the free frames queue

    return frame;

}

int MemoryAllocator::selectVictimFrame()
{
	int victimFrame = -1; // Initialize the victim frame to -1 (indicating no victim found yet)
	uint64_t oldestAccessTick = UINT64_MAX; // Initialize the oldest access tick to the maximum possible value
	for (int i = 0; i < totalFrames; ++i) // Iterate through all frames
	{
		if (frameTable[i].occupied && frameTable[i].lastAccessedTick < oldestAccessTick) // If the frame is occupied and has an older access tick
		{
			oldestAccessTick = frameTable[i].lastAccessedTick; // Update the oldest access tick
			victimFrame = i; // Update the victim frame to the current frame index
		}
	}
	return victimFrame; // Return the index of the selected victim frame //lower accessed tick means it was used longer ago, so it is a good candidate for replacement
}

int MemoryAllocator::pageIn(int pid, int pageNumber)
{
    //std::lock_guard<std::mutex> lock(allocatorMutex); // Lock the mutex to ensure thread safety during page-in operation

	auto ptIt = pageTables.find(pid); // Find the page table for the given process ID
	if (ptIt == pageTables.end()) // If the page table is not found
		return -1; // Return -1 indicating failure

	std::vector<PageEntry>& pageTable = ptIt->second; // Get a reference to the page table for the process

	if (pageNumber < 0 || pageNumber >= static_cast<int>(pageTable.size())) // Check if the page number is valid
        return -1;

	PageEntry& entry = pageTable[pageNumber]; // Get a reference to the page entry for the specified page number

    if (entry.valid) { // If the page is already valid (in memory)
        int frameNumber = entry.frameNumber; // Get the frame number from the page entry
        if (frameNumber >= 0 && frameNumber < totalFrames) // Check if the frame number is valid
            frameTable[frameNumber].lastAccessedTick = ++accessCounter; // Update the last accessed tick for the frame
		return frameNumber; // Return the frame number
    }

	int frameNumber = findFreeFrame(); // Try to find a free frame in memory

    if (frameNumber == -1) { // If no free frame is available
        frameNumber = selectVictimFrame(); // Select a victim frame to evict
        if (frameNumber == -1) // If no victim frame could be selected
            return -1; // Return -1 indicating failure

		pageOut(frameNumber); // Page out the victim frame to backing store

		frameNumber = findFreeFrame(); // Try to find a free frame again after paging out
		if (frameNumber == -1) // If still no free frame is available
			return -1; // Return -1 indicating failure
    }

	int recordNumber = entry.backingStoreRecordNumber; // Get the backing store record number for the page entry
	if (recordNumber < 0 || recordNumber >= static_cast<int>(backingStore.size())) // Check if the record number is valid
		return -1; // Return -1 indicating failure

	BackingStoreRecord& record = backingStore[recordNumber]; // Get a reference to the backing store record
	if (!record.inUse) // If the backing store record is not in use
		return -1; // Return -1 indicating failure

    std::vector<uint8_t> diskData;
	readPageFromFile(recordNumber, diskData); // Read the page data from the backing store file
	record.data = diskData; // Store the read data in the backing store record


	int start = frameNumber * memPerFrame; // Calculate the starting index in physical memory for the frame

	std::copy( // Copy the data from the backing store record to physical memory
		record.data.begin(), // Start of the source range (backing store record data)
		record.data.begin() + memPerFrame, // End of the source range (backing store record data)
		physicalMemory.begin() + start// Start of the destination range (physical memory)
    );

	// Update the page entry to reflect that the page is now valid and in memory
    frameTable[frameNumber].occupied = true;
    frameTable[frameNumber].pid = pid;
    frameTable[frameNumber].pageNumber = pageNumber;
    frameTable[frameNumber].lastAccessedTick = ++accessCounter;


	// Update the page entry to reflect that the page is now valid and in memory
    entry.valid = true;
    entry.dirty = false;
    entry.frameNumber = frameNumber;

	numPagedIn++; // Increment the count of pages that have been paged in
    return frameNumber;

}

void MemoryAllocator::pageOut(int frameNumber)
{
    if (frameNumber < 0 || frameNumber >= totalFrames) // Check if the frame number is valid
        return;

    Frame& frame = frameTable[frameNumber]; // Get a reference to the frame entry
    if (!frame.occupied) // If the frame is not occupied
        return;

    int pid = frame.pid; // Get the process ID from the frame entry
    int pageNumber = frame.pageNumber; // Get the page number from the frame entry

    auto ptIt = pageTables.find(pid); // Find the page table for the given process ID
    if (ptIt == pageTables.end()) // If the page table is not found
        return;

    std::vector<PageEntry>& pageTable = ptIt->second; // Get a reference to the page table for the process
    if (pageNumber < 0 || pageNumber >= static_cast<int>(pageTable.size())) // Check if the page number is valid
        return;

    PageEntry& entry = pageTable[pageNumber]; // Get a reference to the page entry

    if (entry.dirty) // If the page is dirty (modified) 
    {
		int recordNumber = entry.backingStoreRecordNumber; // Get the backing store record number for the page entry
        if (recordNumber >= 0 && recordNumber < static_cast<int>(backingStore.size())) // Check if the record number is valid 
        {
			BackingStoreRecord& record = backingStore[recordNumber]; // Get a reference to the backing store record
            if (record.inUse)//     If the backing store record is in use
            {
				if (static_cast<int>(record.data.size()) < memPerFrame) // Check if the backing store record data size is less than the expected memory per frame
					record.data.assign(memPerFrame, 0); // Resize the data to match the expected size

				int start = frameNumber * memPerFrame; // Calculate the starting index in physical memory for the frame

				std::copy( // Copy the data from physical memory to the backing store record
                    physicalMemory.begin() + start,
                    physicalMemory.begin() + start + memPerFrame,
                    record.data.begin()
                );
                
				writePageToFile(recordNumber, pid, pageNumber, record.data); // Write the page data to the backing store file
            
            }
        }

		entry.dirty = false; // Mark the page entry as not dirty since it has been written back to backing store


    }
	// Mark the page entry as invalid since it is being paged out
    entry.valid = false;
    entry.frameNumber = -1;

	// Mark the frame as unoccupied since it is being paged out
    frame.occupied = false;
    frame.pid = -1;
    frame.pageNumber = -1;
    frame.lastAccessedTick = 0;

	// Increment the count of pages that have been paged out
    freeFrames.push(frameNumber);
    numPagedOut++;

}


/* VMSTAT GUY DO THESE
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

int MemoryAllocator::getMaxOverallMem() const {
    return maxOverallMem;
}

int MemoryAllocator::getUsedMemory() const {
    std::lock_guard<std::mutex> lock(allocatorMutex);
    int occupiedFrames = 0;
    for (int pid : frameTable) {
        if (pid != -1) occupiedFrames++;
    }
    return occupiedFrames * memPerFrame;
}

int MemoryAllocator::getFreeMemory() const {
    std::lock_guard<std::mutex> lock(allocatorMutex);
    int freeFrames = 0;
    for (int pid : frameTable) {
        if (pid == -1) freeFrames++;
    }
    return freeFrames * memPerFrame;
}

int MemoryAllocator::getTotalFrames() const {
    return totalFrames;
}

int MemoryAllocator::getOccupiedFrames() const {
    std::lock_guard<std::mutex> lock(allocatorMutex);
    int occupied = 0;
    for (int pid : frameTable) {
        if (pid != -1) occupied++;
    }
    return occupied;
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

*/

//Sure thing nelson!

bool MemoryAllocator::isAllocated(int pid) const {
    std::lock_guard<std::mutex> lock(allocatorMutex);
    return pageTables.find(pid) != pageTables.end();
}

int MemoryAllocator::getMaxOverallMem() const {
    return maxOverallMem;
}

int MemoryAllocator::getUsedMemory() const {
    std::lock_guard<std::mutex> lock(allocatorMutex);
    int occupiedFrames = 0;
    for (int i = 0; i < totalFrames; ++i) {
        if (frameTable[i].occupied) {
            occupiedFrames++;
        }
    }
    return occupiedFrames * memPerFrame;
}

int MemoryAllocator::getFreeMemory() const {
    std::lock_guard<std::mutex> lock(allocatorMutex);
    return static_cast<int>(freeFrames.size()) * memPerFrame;
}

int MemoryAllocator::getProcessCountUnlocked() const {
    return static_cast<int>(pageTables.size());
}

int MemoryAllocator::getExternalFragmentationUnlocked() const {
    return static_cast<int>(freeFrames.size()) * memPerFrame;
}

std::string MemoryAllocator::generateMemoryStamp(const std::string& timestamp) const {
    std::lock_guard<std::mutex> lock(allocatorMutex);

    std::vector<std::string> sections;

    std::stringstream header;
    header << "Timestamp: " << timestamp << "\n";
    header << "Number of processes in memory: " << getProcessCountUnlocked() << "\n";
    header << "Total external fragmentation in KB: " << getExternalFragmentationUnlocked();
    sections.push_back(header.str());

    sections.push_back("----end---- = " + std::to_string(maxOverallMem));

    int frameIndex = totalFrames - 1;
    while (frameIndex >= 0) {
        int owner = frameTable[frameIndex].pid;
        int runEnd = frameIndex;

        while (frameIndex - 1 >= 0 && frameTable[frameIndex - 1].pid == owner) {
            frameIndex--;
        }
        int runStart = frameIndex;

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
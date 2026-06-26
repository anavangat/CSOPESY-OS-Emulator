#include "LogUtils.h"
#include <fstream>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <mutex>
#include <chrono>
#include <string>
#include <vector>
#include <memory>
#include <filesystem>
#include <unordered_set>

// Global mutex to prevent file write collisions from multiple CPU worker threads.
// Also serializes the "has this process already written its header this run?" check below.
std::mutex logFileMutex;

namespace {

    // Current wall-clock time, used for each PRINT log line: (MM/DD/YYYY HH:MM:SSam/pm)
    std::string getCurrentTimestamp() {
        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        std::tm now_tm{};

        std::tm* ltm = std::localtime(&now_c);
        if (ltm) {
            now_tm = *ltm;
        }

        char buf[100];
        std::strftime(buf, sizeof(buf), "(%m/%d/%Y %I:%M:%S%p)", &now_tm);
        return std::string(buf);
    }

    // Same format, but for a stored std::time_t (used for a process's arrival time
    // in screen -ls / report-util, rather than "now").
    std::string formatTimestamp(std::time_t t) {
        std::tm tmStruct{};
        std::tm* ltm = std::localtime(&t);
        if (ltm) {
            tmStruct = *ltm;
        }
        char buf[100];
        std::strftime(buf, sizeof(buf), "(%m/%d/%Y %I:%M:%S%p)", &tmStruct);
        return std::string(buf);
    }

} // namespace

void LogUtils::print_command(int tick, const Process& process, int core_id) {
    // Tracks which processes (by name) already had their header written THIS run,
    // so the header is emitted exactly once per process and a stale file left over
    // from a previous run doesn't get silently appended to.
    static std::unordered_set<std::string> initializedLogs;

    std::lock_guard<std::mutex> lock(logFileMutex); // serializes all log writes across worker threads
    std::string rawLogMessage = getCurrentTimestamp() + " Core:" + std::to_string(core_id) +
        " \"Hello world from " + process.getName() + "!\"";

    const_cast<Process&>(process).addInMemoryLog(tick, core_id, rawLogMessage); //added a LogEntry class soo probably need to change this part
#if ENABLE_PROCESS_LOGGING
    std::filesystem::create_directories("logs");

    const std::string filePath = "logs/" + process.getName() + ".txt";
    const bool isFirstWriteThisRun = initializedLogs.find(process.getName()) == initializedLogs.end();

    // Truncate on the first write this run (clears any leftover file from a prior run);
    // append for every subsequent write within this run, same as before.
    std::ofstream outFile(filePath, isFirstWriteThisRun ? std::ios::trunc : std::ios::app);
    if (outFile.is_open()) {
        if (isFirstWriteThisRun) {
            outFile << "Process name: " << process.getName() << std::endl;
            outFile << "Logs:" << std::endl;
            outFile << std::endl;
            initializedLogs.insert(process.getName());
        }

        outFile << rawLogMessage << std::endl;
        outFile.close();
    }
#endif
}

void LogUtils::printProcessLine(std::ostream& os, const std::shared_ptr<Process>& p) {
    if (!p) return;

    const int total = p->getTotalInstructions();
    const int remaining = p->getRemainingInstructions();
    const int executed = total - remaining;
    std::string progressStr = std::to_string(executed) + "/" + std::to_string(total);

    std::string coreStr = "N/A";

    if (p->getState() == Process::RUNNING || p->getState() == Process::FINISHED){
        if (p-> getCoreID() != -1){
            coreStr = std::to_string(p->getCoreID());
        }
    }

    std::string stateStr;
    switch (p-> getState()) {
        case Process::READY:
            stateStr = "Ready";
            break;
        case Process::RUNNING:
            stateStr = "Running";
            break;
        case Process::FINISHED:
            stateStr = "Finished";
            break;
        case Process::WAITING:
            stateStr = "Waiting";
            break;
        default: 
            stateStr = "Unknown";
            break;
    }

    os << std::left << std::setw(15) << p->getName()
        << std::setw(25) << formatTimestamp(p->getArrivalTime())
        << std::setw(12) << stateStr
        << std::setw(15) << progressStr
        << coreStr << std::endl;
    /**os << std::left << std::setw(13) << p->getName()
        << std::setw(26) << formatTimestamp(p->getArrivalTime());

    if (p->getState() == Process::FINISHED) {
        os << std::left << std::setw(11) << "Finished";
    }
    else {
        os << std::left << std::setw(11) << ("Core: " + std::to_string(p->getCoreID()));
    }

    os << executed << " / " << total << std::endl;**/
}

void LogUtils::printScreenList(std::ostream& os,
    const std::vector<std::shared_ptr<Process>>& running,
    const std::vector<std::shared_ptr<Process>>& finished) {
    
    os << std::left << std::setw(15) << "Process Name"
       << std::setw(25) << "Arrival Time"
       << std::setw(12) << "Status"
       << std::setw(15) << "Instructions"
       << "Core" << std::endl;
    os << std::string(75, '-') << std::endl;

    for (const auto& p : running) {
        printProcessLine(os, p);
    }
    for (const auto& p : finished) {
        printProcessLine(os, p);
    }
    /**    os << std::string(45, '-') << std::endl;

    os << "Running processes:" << std::endl;
    for (const auto& p : running) {
        printProcessLine(os, p);
    }
    os << std::endl;

    os << "Finished processes:" << std::endl;
    for (const auto& p : finished) {
        printProcessLine(os, p);
    }**/

}

void LogUtils::dump_emulator_log(const std::vector<std::shared_ptr<Process>>& running,
    const std::vector<std::shared_ptr<Process>>& finished) {
    std::ofstream logFile("csopesy-log.txt");
    if (!logFile.is_open()) {
        std::cerr << "Error: Could not create csopesy-log.txt" << std::endl;
        return;
    }

    size_t activeCoresCount = running.size();
    
    int maxCoreIDFound = -1;
    for (const auto& p : running) {
        if (p->getCoreID() > maxCoreIDFound) maxCoreIDFound = p->getCoreID();
    }

    int estimatedTotalCores = (maxCoreIDFound >= 4) ? maxCoreIDFound + 1 : 4; 
    int availableCores = estimatedTotalCores - static_cast<int>(activeCoresCount);
    if (availableCores < 0) availableCores = 0;

    double cpuUtilization = (static_cast<double>(activeCoresCount) / estimatedTotalCores) * 100.0;

    logFile << "CSOPESY Emulator Execution Report" << std::endl;
    logFile << "Generated on: " << getCurrentTimestamp() << std::endl;
    logFile << std::string (75, '-') << std::endl;

    logFile << "CPU Utilization: " << std::fixed << std::setprecision(2) << cpuUtilization << "%" << std::endl;
    logFile << "Core Used: " << activeCoresCount << " | Cores Available: " << availableCores <<std::endl;
    logFile << std::string (75, '-') << std::endl;
    
    printScreenList(logFile, running, finished);

    logFile.close();
    std::cout << "Report generated: csopesy-log.txt" << std::endl;
}
#include "LogUtils.h"
#include <fstream>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <mutex>
#include <chrono>
#include <string>
#include <filesystem>

// Global mutex to prevent file write collisions from multiple CPU threads
std::mutex logFileMutex;

std::string getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm now_tm{};

    std::tm* ltm = std::localtime(&now_c);
    if (ltm) {
        now_tm = *ltm;
    }

    char buf[100];
    // Format: (MM/DD/YYYY HH:MM:SSam/pm)
    std::strftime(buf, sizeof(buf), "(%m/%d/%Y %I:%M:%S%p)", &now_tm);
    return std::string(buf);
}

void LogUtils::print_command(const Process& process, int core_id) {
#if ENABLE_PROCESS_LOGGING
    std::lock_guard<std::mutex> lock(logFileMutex); // Thread safety

    std::filesystem::create_directories("logs");
    
    std::ofstream outFile("logs/" + process.getName() + ".txt", std::ios::app);
    if (outFile.is_open()) {
        outFile << getCurrentTimestamp() << " Core:" << core_id 
                << " \"Hello world from " << process.getName() << "!\"" << std::endl;
        outFile.close();
    }
#endif
}

void LogUtils::dump_emulator_log(const std::vector<std::shared_ptr<Process>>& ready, const std::vector<std::shared_ptr<Process>>& running, const std::vector<std::shared_ptr<Process>>& finished) {
    std::ofstream logFile("csopesy-log.txt");
    if (!logFile.is_open()) {
        std::cerr << "Error: Could not create csopesy-log.txt" << std::endl;
        return;
    }

    logFile << "CSOPESY Emulator Execution Report" << std::endl;
    logFile << "Generated on: " << getCurrentTimestamp() << std::endl;
    logFile << std::string(70, '-') << std::endl;

    printTableHeaders(logFile);
    logFile << std::string(70, '-') << std::endl;

    for (const auto& p : ready) {
        formatProcessRow(logFile, p);
    }
    for (const auto& p : running) {
        formatProcessRow(logFile, p);
    }
    for (const auto& p : finished) {
        formatProcessRow(logFile, p);
    }

    logFile << std::string(70, '-') << std::endl;
    logFile.close();
    std::cout << "Report generated: csopesy-log.txt" << std::endl;
}

void LogUtils::printTableHeaders(std::ostream& os) {
    os << std::left << std::setw(20) << "Process Name" 
       << std::setw(10) << "ID" 
       << std::setw(15) << "Instructions" 
       << std::setw(15) << "Status"
       << "Core" << std::endl;
}

void LogUtils::formatProcessRow(std::ostream& os, const std::shared_ptr<Process> p) {
    int total = p->getTotalInstructions();
    int remaining = p->getRemainingInstructions();
    int executed = total - remaining;
    
    std::string insCount = std::to_string(executed) + "/" + std::to_string(total);
    
    std::string statusStr;
    switch (p->getState()) {
        case Process::READY:    statusStr = "Ready"; break;
        case Process::RUNNING:  statusStr = "Running"; break;
        case Process::WAITING:  statusStr = "Waiting"; break;
        case Process::FINISHED: statusStr = "Finished"; break;
        default:                statusStr = "Unknown"; break;
    }

    std::string coreStr = "N/A";
    if (p->getState() == Process::RUNNING || p->getState() == Process::FINISHED) {
        int core = p->getCoreID();
        if (core != -1){
            coreStr = std::to_string(core);
        }
    }

    os << std::left << std::setw(20) << p->getName()
       << std::setw(10) << p->getPid()
       << std::setw(15) << insCount 
       << std::setw(15) << statusStr
       << coreStr << std::endl;
}
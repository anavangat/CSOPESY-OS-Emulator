#ifndef LOG_UTILS_H
#define LOG_UTILS_H

#include <string>
#include <vector>
#include <ostream>
#include <memory>
#include "Process.h"

// Toggle this to 0 to disable per-process .txt file creation for performance testing
#define ENABLE_PROCESS_LOGGING 0

class LogUtils {
public:

    //Format: (MM/DD/YYYY HH:MM:SSam/pm) Core:N "Hello world from <name>!"

    static void print_command(int tick, const Process& process, int core_id);

    // Writes the "screen -ls" style Running/Finished report to csopesy-log.txt.

    static void dump_emulator_log(int numCpu,
                                  const std::vector<std::pair<std::shared_ptr<Process>, int>>& running,
                                  const std::vector<std::shared_ptr<Process>>& finished);

    // Shared formatter used by both "screen -ls" (console, via std::cout) and
    // "report-util" (file, via an ofstream) so the two stay visually identical.

    static void printScreenList(std::ostream& os, int numCpu,
                                const std::vector<std::pair<std::shared_ptr<Process>, int>>& running,
                                const std::vector<std::shared_ptr<Process>>& finished);

private:
    // Formats a single process row: "<name>   <timestamp>   <Core: N | Finished>   <x / total>"
    static void printProcessLine(std::ostream& os, const std::shared_ptr<Process>& p, bool isFinished, int coreID);
};

#endif
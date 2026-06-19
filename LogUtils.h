#ifndef LOG_UTILS_H
#define LOG_UTILS_H

#include <string>
#include <vector>
#include <ostream>
#include <memory>
#include "Process.h"

// Toggle this to 0 to disable per-process .txt file creation for performance testing
#define ENABLE_PROCESS_LOGGING 1

class LogUtils {
public:

    //Format: (MM/DD/YYYY HH:MM:SSam/pm) Core:N "Hello world from <name>!"

    static void print_command(const Process& process, int core_id);

    //Creates csopesy-log.txt with a table of all processes.
  
    static void dump_emulator_log(const std::vector<std::shared_ptr<Process>>& ready,
                                  const std::vector<std::shared_ptr<Process>>& running, 
                                  const std::vector<std::shared_ptr<Process>>& finished);

    //Shared Formatter
    static void formatProcessRow(std::ostream& os, const std::shared_ptr<Process> p);
    static void printTableHeaders(std::ostream& os);
};

#endif
#include <iostream>
#include <iomanip>
#include <string>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <cstdint>
#include <limits>
#include <vector>
#include <memory>
#include <atomic>
#include <thread>
#include <chrono>
#include "LogUtils.h"
#include "MemoryConfigUtils.h"
#include "InstructionParser.h"
#include "Process.h"
#include "AScheduler.h"
#include "FCFS_Scheduler.h"
#include "RR_Scheduler.h"

void printHeader() {
	std::cout << " ____  ____  ____  ____  _____ ____ ___  _" << std::endl;
	std::cout << "/   _\\/ ___\\/  _ \\/  __\\/  __// ___\\\\  \\//" << std::endl;
	std::cout << "|  /  |    \\| / \\||  \\/||  \\  |    \\ \\  / " << std::endl;
	std::cout << "|  \\__\\___ || \\_/||  __/|  /_ \\___ | / / " << std::endl;
	std::cout << "\\____/\\____/\\____/\\_/   \\____\\\\____//_/ " << std::endl;
	std::cout << "---------------------------------------------" << std::endl;
	std::cout << "Hello!, Welcome to the CSOPESY OS Emulator\n" << std::endl;
}

struct Config {
	uint32_t numCpu = 4;
	std::string scheduler = "fcfs";
	uint32_t quantumCycles = 5;
	uint32_t batchProcessFreq = 1;
	uint32_t minIns = 1000;
	uint32_t maxIns = 2000;
	uint32_t delaysPerExec = 0;
	uint32_t maxOverallMem = 16384;
	uint32_t memPerFrame = 64;
	uint32_t minMemPerProc = 64;
	uint32_t maxMemPerProc = 4096;
};

// Read and parse config.txt. Returns true if file loaded (even if some values invalid/defaulted).
Config parseConfig(const std::string& path) {
	Config cfg;
	std::ifstream in(path);
	if (!in.is_open()) {
		std::cout << "Could not open " << path << ". Using defaults." << std::endl;
		return cfg;
	}

	std::string line;
	while (std::getline(in, line)) {
		std::istringstream ss(line);
		std::string key;
		if (!(ss >> key)) continue; // empty line

		if (key == "num-cpu") {
			uint64_t v=0; ss >> v;
			if (v >= 1 && v <= 128) cfg.numCpu = static_cast<uint32_t>(v);
			else std::cout << "num-cpu out of range (1..128). Using: " << cfg.numCpu << std::endl;
		}
		else if (key == "scheduler") {
			std::string val; ss >> val;
			if (val == "\"fcfs\"" || val == "\"rr\"") {
				val = val.substr(1, val.length() - 2);
				cfg.scheduler = val;
			}
			else std::cout << "scheduler invalid (expected 'fcfs' or 'rr'). Using: " << cfg.scheduler << std::endl;
		}
		else if (key == "quantum-cycles") {
			uint64_t v=0; ss >> v;
			if (v >= 1 && v <= std::numeric_limits<uint32_t>::max()) cfg.quantumCycles = static_cast<uint32_t>(v);
			else std::cout << "quantum-cycles out of range. Using: " << cfg.quantumCycles << std::endl;
		}
		else if (key == "batch-process-freq") {
			uint64_t v=0; ss >> v;
			if (v >= 1 && v <= std::numeric_limits<uint32_t>::max()) cfg.batchProcessFreq = static_cast<uint32_t>(v);
			else std::cout << "batch-process-freq out of range. Using: " << cfg.batchProcessFreq << std::endl;
		}
		else if (key == "min-ins") {
			uint64_t v=0; ss >> v;
			if (v >= 1 && v <= std::numeric_limits<uint32_t>::max()) cfg.minIns = static_cast<uint32_t>(v);
			else std::cout << "min-ins out of range. Using: " << cfg.minIns << std::endl;
		}
		else if (key == "max-ins") {
			uint64_t v=0; ss >> v;
			if (v >= 1 && v <= std::numeric_limits<uint32_t>::max()) cfg.maxIns = static_cast<uint32_t>(v);
			else std::cout << "max-ins out of range. Using: " << cfg.maxIns << std::endl;
		}
		else if (key == "delays-per-exec") {
			uint64_t v=0; ss >> v;
			if (v <= std::numeric_limits<uint32_t>::max()) cfg.delaysPerExec = static_cast<uint32_t>(v);
			else std::cout << "delays-per-exec out of range. Using: " << cfg.delaysPerExec << std::endl;
		}
		else if (key == "max-overall-mem") {
			uint64_t v = 0; ss >> v;
			if (v <= std::numeric_limits<uint32_t>::max() && MemoryConfigUtils::isValidMemoryValue(static_cast<uint32_t>(v))) cfg.maxOverallMem = static_cast<uint32_t>(v);
			else std::cout << "max-overall-mem invalid (must be a power of two between 64 and 65536). Using: " << cfg.maxOverallMem << std::endl;
		}
		else if (key == "mem-per-frame") {
			uint64_t v = 0; ss >> v;
			if (v <= std::numeric_limits<uint32_t>::max() && MemoryConfigUtils::isValidMemoryValue(static_cast<uint32_t>(v))) cfg.memPerFrame = static_cast<uint32_t>(v);
			else std::cout << "mem-per-frame invalid (must be a power of two between 64 and 65536). Using: " << cfg.memPerFrame << std::endl;
		}
		else if (key == "min-mem-per-proc") {
			uint64_t v = 0; ss >> v;
			if (v <= std::numeric_limits<uint32_t>::max() && MemoryConfigUtils::isValidMemoryValue(static_cast<uint32_t>(v))) cfg.minMemPerProc = static_cast<uint32_t>(v);
			else std::cout << "min-mem-per-proc invalid (must be a power of two between 64 and 65536). Using: " << cfg.minMemPerProc << std::endl;
		}
		else if (key == "max-mem-per-proc") {
			uint64_t v = 0; ss >> v;
			if (v <= std::numeric_limits<uint32_t>::max() && MemoryConfigUtils::isValidMemoryValue(static_cast<uint32_t>(v))) cfg.maxMemPerProc = static_cast<uint32_t>(v);
			else std::cout << "max-mem-per-proc invalid (must be a power of two between 64 and 65536). Using: " << cfg.maxMemPerProc << std::endl;
		}
		// ignore unknown keys
	}

	// Ensure minIns <= maxIns
	if (cfg.minIns > cfg.maxIns) {
		std::cout << "min-ins > max-ins, swapping values." << std::endl;
		std::swap(cfg.minIns, cfg.maxIns);
	}

	bool memConfigValid = true;

	if (cfg.memPerFrame == 0) {
		std::cout << "mem-per-frame is 0; cannot compute frame counts." << std::endl;
		memConfigValid = false;
	}
	else {
		if (cfg.minMemPerProc % cfg.memPerFrame != 0) {
			std::cout << "min-mem-per-proc is not a multiple of mem-per-frame; the remainder would be unaddressable." << std::endl;
			memConfigValid = false;
		}
		if (cfg.maxMemPerProc % cfg.memPerFrame != 0) {
			std::cout << "max-mem-per-proc is not a multiple of mem-per-frame; the remainder would be unaddressable." << std::endl;
			memConfigValid = false;
		}
		if (cfg.maxOverallMem % cfg.memPerFrame != 0) {
			std::cout << "max-overall-mem is not a multiple of mem-per-frame; the remainder would be unaddressable." << std::endl;
			memConfigValid = false;
		}
	}

	if (cfg.minMemPerProc > cfg.maxMemPerProc) {
		std::cout << "min-mem-per-proc > max-mem-per-proc, swapping values." << std::endl;
		std::swap(cfg.minMemPerProc, cfg.maxMemPerProc);
	}

	// Under demand paging a process larger than physical memory is legal -- it simply pages
	// against the backing store. The old "max-mem-per-proc > max-overall-mem" guard was a
	// Phase 1 flat-allocator relic: it rejected spec-legal configs (all four parameters are
	// independently allowed the full [2^6, 2^16] range) and, because the built-in defaults
	// are 16384 / 65536, it also fired on the fallback config it reverts to.
	//if (cfg.maxMemPerProc > cfg.maxOverallMem) {
	//	std::cout << "max-mem-per-proc > max-overall-mem; this would prevent any process from being allocated." << std::endl;
	//	memConfigValid = false;
	//}

	if (!memConfigValid) {
		std::cout << "Reverting memory settings to defaults: max-overall-mem 16384, mem-per-frame 64, min-mem-per-proc 64, max-mem-per-proc 4096." << std::endl;
		cfg.maxOverallMem = 16384;
		cfg.memPerFrame = 64;
		cfg.minMemPerProc = 64;
		cfg.maxMemPerProc = 4096;
	}

	std::cout << "Loaded configuration:" << std::endl;
	std::cout << "  num-cpu: " << cfg.numCpu << std::endl;
	std::cout << "  scheduler: " << cfg.scheduler << std::endl;
	std::cout << "  quantum-cycles: " << cfg.quantumCycles << std::endl;
	std::cout << "  batch-process-freq: " << cfg.batchProcessFreq << std::endl;
	std::cout << "  min-ins: " << cfg.minIns << std::endl;
	std::cout << "  max-ins: " << cfg.maxIns << std::endl;
	std::cout << "  delays-per-exec: " << cfg.delaysPerExec << std::endl;
	std::cout << "  max-overall-mem: " << cfg.maxOverallMem << std::endl;
	std::cout << "  mem-per-frame: " << cfg.memPerFrame << std::endl;
	std::cout << "  min-mem-per-proc: " << cfg.minMemPerProc << std::endl;
	std::cout << "  max-mem-per-proc: " << cfg.maxMemPerProc << std::endl;
	
	return cfg;
}

// Renders the "process-smi" view for one process inside its screen.
void printProcessSmi(const std::shared_ptr<Process>& process) {
	std::cout << "Process name: " << process->getName() << std::endl;
	std::cout << "ID: " << process->getPid() << std::endl;
	std::cout << "Logs:" << std::endl;

	const auto logs = process->getInMemoryLogs(); // returns a mutex-locked copy; safe to iterate
	for (const auto& entry : logs) {
		std::cout << entry.getEvent() << std::endl;
	}
	std::cout << std::endl;

	if (process->isFinished()) {
		std::cout << "Finished!" << std::endl;
	}
	else {
		const int total = process->getTotalInstructions();
		const int executed = total - process->getRemainingInstructions(); // == program counter
		std::cout << "Current instruction line: " << executed << std::endl;
		std::cout << "Lines of code: " << total << std::endl;
	}
	std::cout << std::endl;
}

// Takes over input until the user types "exit", emulating attaching to a process screen.
void runProcessScreen(const std::shared_ptr<Process>& process) {
	system("cls");
	std::cout << "Attached to process: " << process->getName() << "\n" << std::endl;
	printProcessSmi(process);

	std::string input;
	while (true) {
		std::cout << "root:\\> ";
		if (!std::getline(std::cin, input)) break; // EOF safety

		if (input == "process-smi") {
			printProcessSmi(process);
		}
		else if (input == "exit") {
			break; // return to the main menu
		}
		else {
			std::cout << "Command not recognized inside screen. Available: process-smi, exit\n" << std::endl;
		}
	}
	system("cls");
}

// Updated process-smi Handler
void handleProcessSmi(AScheduler* scheduler) {
    if (!scheduler) return;

    auto& allocator = scheduler->getMemoryAllocator();
    int cpuUtil = scheduler->getCpuUtilization();
    int usedMemKB = allocator.getUsedMemory();
    int totalMemKB = allocator.getMaxOverallMem();
    int memUtil = (totalMemKB > 0) ? (usedMemKB * 100) / totalMemKB : 0;

    std::cout << "-----------------------------------------------------------------" << std::endl;
    std::cout << "| PROCESS-SMI V01.00 Driver Version: 01.00                       |" << std::endl;
    std::cout << "-----------------------------------------------------------------" << std::endl;
    std::cout << "CPU-Util: " << cpuUtil << "%" << std::endl;
    std::cout << "Memory Usage: " << usedMemKB << "KB / " << totalMemKB << "KB" << std::endl;
    std::cout << "Memory Util: " << memUtil << "%" << std::endl;
    std::cout << "=================================================================" << std::endl;
    std::cout << "Running processes and memory usage:" << std::endl;
    std::cout << "-----------------------------------------------------------------" << std::endl;

    auto runningSnapshot = scheduler->getRunningSnapshot();
    if (runningSnapshot.empty()) {
        std::cout << "(No active running processes)" << std::endl;
    } else {
        for (const auto& pair : runningSnapshot) {
            auto process = pair.first;
            // Uses actual memory allocated for each process
            std::cout << process->getName() << " \t" << process->getMemoryRequired() << "KB" << std::endl;
        }
    }
    std::cout << "-----------------------------------------------------------------" << std::endl;
}

// Updated vmstat Handler
void handleVmStat(AScheduler* scheduler, int currentTick) {
    if (!scheduler) return;

    auto& allocator = scheduler->getMemoryAllocator();
    int totalMem = allocator.getMaxOverallMem();
    int usedMem = allocator.getUsedMemory();
    int freeMem = allocator.getFreeMemory();
    
    int pagedIn = allocator.getNumPagedIn();
    int pagedOut = allocator.getNumPagedOut();

    // CPU tick statistics from AScheduler
    int activeTicks = scheduler->getActiveCpuTicks();
    int idleTicks = scheduler->getIdleCpuTicks();
    int totalTicks = scheduler->getTotalCpuTicks();

    std::cout << std::setw(12) << totalMem << " K total memory" << std::endl;
    std::cout << std::setw(12) << usedMem << " K used memory" << std::endl;
    //std::cout << std::setw(12) << usedMem << " K active memory" << std::endl;
    //std::cout << std::setw(12) << freeMem << " K inactive memory" << std::endl;
    std::cout << std::setw(12) << freeMem << " K free memory" << std::endl;
    std::cout << std::setw(12) << totalTicks << " total cpu ticks" << std::endl;
    std::cout << std::setw(12) << activeTicks << " active cpu ticks" << std::endl;
    std::cout << std::setw(12) << idleTicks << " idle cpu ticks" << std::endl;
    std::cout << std::setw(12) << pagedIn << " pages paged in" << std::endl;
    std::cout << std::setw(12) << pagedOut << " pages paged out" << std::endl;
}

int main() {
	std::string command;
	bool initialized = false;
	Config cfg;
	bool cfgLoaded = false;

	std::atomic<int> cpuTick{ 0 };
	std::thread cpuTickThread([&cpuTick]() {
		while (true) {
			cpuTick++;
			// Change
			std::this_thread::sleep_for(std::chrono::milliseconds(1)); // 1 tick = 1ms
		}
	});

	std::unique_ptr<AScheduler> scheduler;

	printHeader();

	while (true) {
		std::cout << "Type Command Here: ";
		std::getline(std::cin, command);

		if (command == "exit") {
			if (scheduler) {
				scheduler->stop();
			}
			if (cpuTickThread.joinable()) {
				cpuTickThread.detach();
			}
			break;
		}

		if (!initialized) {
			if (command == "initialize") {
				cfg = parseConfig("config.txt");
				cfgLoaded = true;

				if (cfg.scheduler == "fcfs") {
					scheduler = std::make_unique<FCFS_Scheduler>(cfg.numCpu, cfg.batchProcessFreq, cfg.minIns, cfg.maxIns, cfg.delaysPerExec, cpuTick, cfg.maxOverallMem, cfg.memPerFrame, cfg.minMemPerProc, cfg.maxMemPerProc);
				}
				else if (cfg.scheduler == "rr") {
					 scheduler = std::make_unique<RR_Scheduler>(cfg.numCpu, cfg.batchProcessFreq, cfg.minIns, cfg.maxIns, cfg.delaysPerExec, cpuTick, cfg.quantumCycles, cfg.maxOverallMem, cfg.memPerFrame, cfg.minMemPerProc, cfg.maxMemPerProc);
				}

				scheduler->start();

				std::cout << "Initialize done" << std::endl;
				initialized = true;
			}
			else {
				std::cout << "Please initialize everything first" << std::endl;
			}
			std::cout << "---------------------------------------------\n" << std::endl;
			continue;
		}

		// Tokenize the input so sub-arguments are supported (e.g. "screen -ls").
		// cmd = first token (the actual command), arg = second token (its sub-argument, if any).
		std::vector<std::string> tokens;
		{
			std::istringstream tokenStream(command);
			std::string tok;
			while (tokenStream >> tok) tokens.push_back(tok);
		}
		std::string cmd = tokens.empty() ? "" : tokens[0];
		std::string arg = tokens.size() > 1 ? tokens[1] : "";

		if (cmd == "initialize") {
			std::cout << "System is already initialized" << std::endl;
			std::cout << "---------------------------------------------\n" << std::endl;
			initialized = true;
		}
		else if (cmd == "screen") {
			if (arg == "-ls") {
				auto runningProcesses = scheduler->getRunningSnapshot();
				auto finishedProcesses = scheduler->getProcessesByState(Process::ProcessState::FINISHED);
				LogUtils::printScreenList(std::cout, scheduler->getNumCpu(), runningProcesses, finishedProcesses);
				std::cout << "---------------------------------------------\n" << std::endl;
			}
			else if (arg == "-s") {
				std::string name = tokens.size() > 2 ? tokens[2] : "";
				std::string sizeToken = tokens.size() > 3 ? tokens[3] : "";
				uint32_t memorySize = 0;

				if (name.empty() || sizeToken.empty()) {
					std::cout << "Usage: screen -s <process name> <process memory size>" << std::endl;
					std::cout << "---------------------------------------------\n" << std::endl;
				}
				// Spec: [2^6, 2^16] bytes AND an exact power of two, else one fixed message.
				else if (!InstructionParser::parseUnsignedInteger(sizeToken, memorySize) ||
					!MemoryConfigUtils::isValidMemoryValue(memorySize)) {
					std::cout << "invalid memory allocation" << std::endl;
					std::cout << "---------------------------------------------\n" << std::endl;
				}
				else {
					auto process = scheduler->createUserProcess(name, static_cast<int>(memorySize));
					if (!process) {
						std::cout << "Process " << name << " already exists." << std::endl;
						std::cout << "---------------------------------------------\n" << std::endl;
					}
					else {
						runProcessScreen(process); // blocks until the user types "exit"
						printHeader();             // redraw main menu in the same window
					}
				}
			}
			else if (arg == "-c") {
				std::string name = tokens.size() > 2 ? tokens[2] : "";
				std::string payload;
				uint32_t memorySize = 0;

				// The MO2 grammar is: screen -c <name> <mem> "<instructions>", but both worked
				// examples in the handout omit the size. Token 3 is read as the memory size only
				// when it is a plain integer; otherwise the size is rolled like scheduler-start.
				bool hasExplicitSize = tokens.size() > 3 &&
					InstructionParser::parseUnsignedInteger(tokens[3], memorySize);

				if (name.empty()) {
					std::cout << "Usage: screen -c <process name> <process memory size> \"<instructions>\"" << std::endl;
				}
				else if (hasExplicitSize && !MemoryConfigUtils::isValidMemoryValue(memorySize)) {
					std::cout << "invalid memory allocation" << std::endl;
				}
				else if (!InstructionParser::extractQuotedPayload(command, payload)) {
					std::cout << "invalid command" << std::endl; // no quoted instruction block
				}
				else {
					std::vector<std::shared_ptr<Instruction>> parsedInstructions;
					if (!InstructionParser::parseAll(0, payload, parsedInstructions)) {
						// count outside [1, 50], unknown opcode, bad hex, or a malformed argument
						std::cout << "invalid command" << std::endl;
					}
					else {
						auto process = scheduler->createUserProcessWithInstructions(
							name,
							hasExplicitSize ? static_cast<int>(memorySize) : 0,
							parsedInstructions);

						if (!process) {
							std::cout << "Process " << name << " already exists." << std::endl;
						}
						else {
							std::cout << "Process " << name << " created with "
								<< process->getTotalInstructions() << " instruction(s) and "
								<< process->getMemoryRequired() << " bytes of memory." << std::endl;
						}
					}
				}
				std::cout << "---------------------------------------------\n" << std::endl;
			}
			else if (arg == "-r") {
				std::string name = tokens.size() > 2 ? tokens[2] : "";
				if (name.empty()) {
					std::cout << "Usage: screen -r <process name>" << std::endl;
					std::cout << "---------------------------------------------\n" << std::endl;
				}
				else {
					auto process = scheduler->getProcessByName(name);
					if (!process) {
						std::cout << "Process " << name << " not found." << std::endl;
						std::cout << "---------------------------------------------\n" << std::endl;
					}
					// MO2: report how and when the process died. This MUST be checked before the
					// FINISHED branch, because AScheduler marks a violated process FINISHED.
					else if (process->hasMemoryViolation()) {
						std::cout << "Process " << name
							<< " shut down due to memory access violation error that occurred at "
							<< process->getViolationTimestamp() << ". "
							<< process->getViolationAddress() << " invalid." << std::endl;
						std::cout << "---------------------------------------------\n" << std::endl;
					}
					// Spec: a normally finished process is treated identically to a missing one.
					else if (process->getState() == Process::FINISHED) {
						std::cout << "Process " << name << " not found." << std::endl;
						std::cout << "---------------------------------------------\n" << std::endl;
					}
					else {
						runProcessScreen(process);
						printHeader();
					}
				}
			}
			else {
				std::cout << "Unknown screen option. Use: screen -s <name>, screen -r <name>, or screen -ls." << std::endl;
				std::cout << "---------------------------------------------\n" << std::endl;
			}
		}
		else if (cmd == "process-smi") {
            handleProcessSmi(scheduler.get());
            std::cout << "\n" << std::endl;
        }
        else if (cmd == "vmstat") {
            handleVmStat(scheduler.get(), cpuTick.load());
            std::cout << "---------------------------------------------\n" << std::endl;
        }
		else if (cmd == "scheduler-start") {
			scheduler->startDummyProcessGeneration();
			std::cout << "---------------------------------------------\n" << std::endl;
		}
		else if (cmd == "scheduler-stop") {
			scheduler->stopDummyProcessGeneration();
			std::cout << "---------------------------------------------\n" << std::endl;
		}
		else if (cmd == "report-util") {
			std::cout << "Generating execution report..." << std::endl;
			auto runningProcesses = scheduler->getRunningSnapshot();
			auto finishedProcesses = scheduler->getProcessesByState(Process::ProcessState::FINISHED);
			LogUtils::dump_emulator_log(scheduler->getNumCpu(), runningProcesses, finishedProcesses);
			std::cout << "---------------------------------------------\n" << std::endl;
		}
		else if (cmd == "clear") {
			system("cls");
			printHeader();
		}
		else {
			std::cout << "Command is not recognized. Please type again" << std::endl;
			std::cout << "---------------------------------------------\n" << std::endl;
		}
	}
	return 0;
}

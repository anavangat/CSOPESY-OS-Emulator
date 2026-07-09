#include <iostream>
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
		// ignore unknown keys
	}

	// Ensure minIns <= maxIns
	if (cfg.minIns > cfg.maxIns) {
		std::cout << "min-ins > max-ins, swapping values." << std::endl;
		std::swap(cfg.minIns, cfg.maxIns);
	}

	std::cout << "Loaded configuration:" << std::endl;
	std::cout << "  num-cpu: " << cfg.numCpu << std::endl;
	std::cout << "  scheduler: " << cfg.scheduler << std::endl;
	std::cout << "  quantum-cycles: " << cfg.quantumCycles << std::endl;
	std::cout << "  batch-process-freq: " << cfg.batchProcessFreq << std::endl;
	std::cout << "  min-ins: " << cfg.minIns << std::endl;
	std::cout << "  max-ins: " << cfg.maxIns << std::endl;
	std::cout << "  delays-per-exec: " << cfg.delaysPerExec << std::endl;

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
					scheduler = std::make_unique<FCFS_Scheduler>(cfg.numCpu, cfg.batchProcessFreq, cfg.minIns, cfg.maxIns, cfg.delaysPerExec, cpuTick, 16384, 16, 4096); //NOTE: REPLACE THE INTS WITH THE VALUES IN CONFIG
				}
				else if (cfg.scheduler == "rr") {
					 scheduler = std::make_unique<RR_Scheduler>(cfg.numCpu, cfg.batchProcessFreq, cfg.minIns, cfg.maxIns, cfg.delaysPerExec, cpuTick, cfg.quantumCycles, 16384, 16, 4096);
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
				if (name.empty()) {
					std::cout << "Usage: screen -s <process name>" << std::endl;
					std::cout << "---------------------------------------------\n" << std::endl;
				}
				else {
					auto process = scheduler->createUserProcess(name);
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
			else if (arg == "-r") {
				std::string name = tokens.size() > 2 ? tokens[2] : "";
				if (name.empty()) {
					std::cout << "Usage: screen -r <process name>" << std::endl;
					std::cout << "---------------------------------------------\n" << std::endl;
				}
				else {
					auto process = scheduler->getProcessByName(name);
					// Spec: a finished process is treated identically to a missing one.
					if (!process || process->getState() == Process::FINISHED) {
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

#include <iostream>
#include <string>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <cstdint>
#include <limits>

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
	uint32_t numCpu = 1;
	std::string scheduler = "fcfs";
	uint32_t quantumCycles = 1;
	uint32_t batchProcessFreq = 1;
	uint32_t minIns = 1;
	uint32_t maxIns = 1;
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
			if (val == "fcfs" || val == "rr") cfg.scheduler = val;
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

int main() {
	std::string command;
	bool initialized = false;
	Config cfg;
	bool cfgLoaded = false;

	printHeader();

	while (true) {
		std::cout << "Type Command Here: ";
		std::getline(std::cin, command);

		if (command == "exit") {
			break;
		}

		if (!initialized) {
			if (command == "initialize") {
				cfg = parseConfig("config.txt");
				cfgLoaded = true;
				std::cout << "Initialize done" << std::endl;
				initialized = true;
			}
			else {
				std::cout << "Please initialize everything first" << std::endl;
			}
			std::cout << "---------------------------------------------\n" << std::endl;
			continue;
		}

		if (command == "initialize") {
			std::cout << "System is already initialized" << std::endl;
			std::cout << "---------------------------------------------\n" << std::endl;
			initialized = true;
		}
		else if (command == "screen") {
			std::cout << "Screen command recognized. Doing something....." << std::endl;
			std::cout << "---------------------------------------------\n" << std::endl;
		}
		else if (command == "scheduler-start") {
			std::cout << "Scheduler-start command recognized. Doing something....." << std::endl;
			std::cout << "---------------------------------------------\n" << std::endl;
		}
		else if (command == "scheduler-stop") {
			std::cout << "Scheduler-stop command recognized. Doing something....." << std::endl;
			std::cout << "---------------------------------------------\n" << std::endl;
		}
		else if (command == "report-util") {
			std::cout << "Report-util command recognized. Doing something....." << std::endl;
			std::cout << "---------------------------------------------\n" << std::endl;
		}
		else if (command == "clear") {
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

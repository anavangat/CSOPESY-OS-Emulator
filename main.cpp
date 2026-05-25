#include <iostream>
#include <string>
#include <cstdlib>

void printHeader() {
	std::cout << " ____  ____  ____  ____  _____ ____ ___  _" << std::endl;
	std::cout << "/   _\\/ ___\\/  _ \\/  __\\/  __// ___\\\\  \\//" << std::endl;
	std::cout << "|  /  |    \\| / \\||  \\/||  \\  |    \\ \\  / " << std::endl;
	std::cout << "|  \\__\\___ || \\_/||  __/|  /_ \\___ | / / " << std::endl;
	std::cout << "\\____/\\____/\\____/\\_/   \\____\\\\____//_/ " << std::endl;
	std::cout << "---------------------------------------------" << std::endl;
	std::cout << "Hello!, Welcome to the CSOPESY OS Emulator\n" << std::endl;
}

int main() {
	std::string command;
	bool initialized = false;

	printHeader();

	while (true) {
		std::cout << "Type Command Here: ";
		std::getline(std::cin, command);

		if (command == "exit") {
			break;
		}

		if (!initialized && command != "initialize") {
			std::cout << "Please initialize everything first" << std::endl;
			std::cout << "---------------------------------------------\n" << std::endl;
			continue;
		}

		if (command == "initialize") {
			std::cout << "Initialize done" << std::endl;
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

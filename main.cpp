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

	bool running = true;

	printHeader();

	while (running) {

		std::cout << "Type Command Here: ";
		getline(std::cin, command);

		if (command == "initialize") {
			std::cout << "Initialize command recognized. Doing something....." << std::endl;
			std::cout << "---------------------------------------------\n" << std::endl;
		}
		else if (command == "screen") {
			std::cout << "Screen command recognized. Doing something....." << std::endl;
			std::cout << "---------------------------------------------\n" << std::endl;
		}
		else if (command == "scheduler-start") {
			std::cout << "Scheduler-star command recognized. Doing something....." << std::endl;
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
			system("cls"); //clears console screen but only for Windows
			printHeader();
		}

		else if (command == "exit") {
			running = false;

		}
		else
		{
			std::cout << "Command is not recognized. Please type again" << std::endl;
			std::cout << "---------------------------------------------\n" << std::endl;
		}
	}
    return 0;



}


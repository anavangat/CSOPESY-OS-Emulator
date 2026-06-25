#pragma once
#include <string>

class LogEntry
{
public:

	LogEntry(int tick, int coreID, int pid, const std::string& event)
		: tick(tick), coreID(coreID), pid(pid), event(event) {
	}

	int getTick() const { return tick; }
	int getCoreID() const { return coreID; }
	int getPid() const { return pid; }
	const std::string& getEvent() const { return event; }

private:
	int tick;
	int coreID;
	int pid;
	std::string event;
	
};


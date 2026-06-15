#pragma once
#include <string>
#include <unordered_map>
#include <cstdint>

class SymbolTable
{
public: 
	void setVariable(const std::string& name, int value) { 
		table[name] = value; 
	} 
	
	int getVariable(const std::string& name) {
		if (table.find(name) != table.end()) { 
			return table[name]; 
		} 
		return 0; // Default or error value
	}
	
	bool hasVariable(const std::string& name) const { 
		return table.find(name) != table.end(); 
	} 

private: 
	std::unordered_map<std::string, uint16_t> table; 
};


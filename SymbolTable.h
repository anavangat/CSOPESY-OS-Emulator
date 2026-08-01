#pragma once
#include <string>
#include <unordered_map>
#include <cstdint>

class SymbolTable
{
public: 

	static constexpr uint16_t MAX_VARIABLES = 32; // Maximum number of variables allowed in the symbol table

	/*
	bool declareVariable(const std::string& name, uint16_t value) // Declare a new variable with the given name and initial value
	{
		if (table.find(name) != table.end()) // Check if the variable already exists
			return false; // Variable already declared
		
		if (table.size() >= MAX_VARIABLES) // Check if the maximum number of variables has been reached
			return false; // Cannot declare more variables

		table.emplace(name, value); // Add the new variable to the symbol table
		return true;
	}

	*/
	void setVariable(const std::string& name, uint16_t value) { 

		if (table.find(name) != table.end()) { // Check if the variable already exists
			table[name] = value;
			return; // Variable already declared

			}

		else if (table.size() < MAX_VARIABLES) {
			table[name] = value;
		}
			return; // Cannot declare more variables

	} 
	
	uint16_t getVariable(const std::string& name) {
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


#pragma once
#include <vector>
#include <memory>
#include "Instruction.h"
#include "SymbolTable.h"
class ForInstruction : public Instruction
{
public:
	ForInstruction(int pid, const std::vector<std::shared_ptr<Instruction>>& body, int repeats);
	void execute(Process& process, SymbolTable& symbolTable) override;

	const std::vector<std::shared_ptr<Instruction>>& getBody() const;
	int getRepeats() const;

	static void flatten(const std::shared_ptr<Instruction>& instruction, std::vector < std::shared_ptr<Instruction>>& out);
	static int flattenedSize(const std::shared_ptr<Instruction>& instruction);

private:
	std::vector<std::shared_ptr<Instruction>> body;
	int repeats;
};


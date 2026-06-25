#include "ForInstruction.h"

ForInstruction::ForInstruction(int pid, const std::vector<std::shared_ptr<Instruction>>& body, int repeats)
	: Instruction(pid, InstructionType::FOR), body(body), repeats(repeats) {
}

void ForInstruction::execute(Process& process, SymbolTable& symbolTable) {
	Instruction::execute(process, symbolTable); // Call base class execute

	for (int i = 0; i < repeats; i++) {
		for (const auto& instruction : body) {
			if (instruction) {
				instruction->execute(process, symbolTable);
			}
		}
	}
}

const std::vector<std::shared_ptr<Instruction>>& ForInstruction::getBody() const {
	return body;
}

int ForInstruction::getRepeats() const {
	return repeats;
}

void ForInstruction::flatten(const std::shared_ptr<Instruction>& instruction, std::vector<std::shared_ptr<Instruction>>& out) {
	if (!instruction) {
		return;
	}
	if (instruction->getInstructionType() == Instruction::FOR) {
		auto forInstruction = std::dynamic_pointer_cast<ForInstruction>(instruction);
		if (forInstruction) {
			for (int i = 0; i < forInstruction->getRepeats(); i++) {
				for (const auto& innerInstruction : forInstruction->getBody()) {
					flatten(innerInstruction, out);
				}
			}
		}
	}
	else {
		out.push_back(instruction);
	}
}


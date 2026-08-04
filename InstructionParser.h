#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include "Instruction.h"

// Parses the quoted instruction payload supplied by the "screen -c" command into concrete
// Instruction objects. Follows the project convention of reporting failures through bool
// return values rather than exceptions; every false return maps to "invalid command".
class InstructionParser
{
public:
	static constexpr int MIN_INSTRUCTION_COUNT = 1;
	static constexpr int MAX_INSTRUCTION_COUNT = 50;
	static constexpr int MAX_FOR_DEPTH = 3; // mirrors AScheduler::maxForDepth

	// Pulls the payload sitting between the first and the last double quote of the raw CLI
	// line, then unescapes \" so PRINT(\"...\") survives the trip through std::getline.
	static bool extractQuotedPayload(const std::string& rawCommand, std::string& payload);

	// Splits on ';' but only at bracket/paren depth 0 and outside string literals, so a
	// FOR([DECLARE x 1; PRINT("hi")], 2) body is not torn apart.
	static std::vector<std::string> splitInstructions(const std::string& payload);

	// "0x500" / "0X500" -> 1280. Rejects empty bodies, non-hex digits and overflow.
	static bool parseHexAddress(const std::string& token, int& address);

	// Plain decimal parse used by the screen -s / screen -c memory-size arguments.
	static bool parseUnsignedInteger(const std::string& token, uint32_t& value);

	// Full parse. Returns false on a malformed token, an unknown opcode, unbalanced
	// brackets, a FOR nested deeper than MAX_FOR_DEPTH, or an instruction count outside
	// [MIN_INSTRUCTION_COUNT, MAX_INSTRUCTION_COUNT].
	static bool parseAll(int pid, const std::string& payload,
		std::vector<std::shared_ptr<Instruction>>& instructions);

private:
	static std::string trim(const std::string& text);
	static bool isValidIdentifier(const std::string& token);
	static bool parseUint16(const std::string& token, uint16_t& value);
	static std::vector<std::string> tokenize(const std::string& text);
	static bool extractCallBody(const std::string& text, const std::string& opcode, std::string& inner);

	// Turns a numeric ADD/SUBTRACT operand into a named constant, emitting one DECLARE into
	// the prelude the first time that constant is seen. Returns "" when the token is neither
	// a valid identifier nor an integer literal.
	static std::string resolveOperand(int pid, const std::string& token,
		std::vector<std::shared_ptr<Instruction>>& prelude,
		std::vector<std::string>& declaredConstants);

	static std::shared_ptr<Instruction> parseOne(int pid, const std::string& text, int depth,
		std::vector<std::shared_ptr<Instruction>>& prelude,
		std::vector<std::string>& declaredConstants);
	static std::shared_ptr<Instruction> parsePrint(int pid, const std::string& text);
	static std::shared_ptr<Instruction> parseFor(int pid, const std::string& text, int depth,
		std::vector<std::shared_ptr<Instruction>>& prelude,
		std::vector<std::string>& declaredConstants);
};
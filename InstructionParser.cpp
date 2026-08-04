#include "InstructionParser.h"
#include <algorithm>
#include <cctype>
#include <sstream>
#include "PrintInstruction.h"
#include "DeclareInstruction.h"
#include "AddInstruction.h"
#include "SubtractInstruction.h"
#include "SleepInstruction.h"
#include "ForInstruction.h"
#include "ReadInstruction.h"
#include "WriteInstruction.h"

std::string InstructionParser::trim(const std::string& text) {
	size_t first = text.find_first_not_of(" \t\r\n");
	if (first == std::string::npos) {
		return "";
	}
	size_t last = text.find_last_not_of(" \t\r\n");
	return text.substr(first, last - first + 1);
}

bool InstructionParser::isValidIdentifier(const std::string& token) {
	if (token.empty()) {
		return false;
	}
	if (!(std::isalpha(static_cast<unsigned char>(token[0])) || token[0] == '_')) {
		return false; // must not start with a digit
	}
	for (char c : token) {
		if (!(std::isalnum(static_cast<unsigned char>(c)) || c == '_')) {
			return false;
		}
	}
	return true;
}

bool InstructionParser::parseUnsignedInteger(const std::string& token, uint32_t& value) {
	if (token.empty() || token.size() > 10) {
		return false; // >10 digits cannot fit in uint32_t
	}
	for (char c : token) {
		if (!std::isdigit(static_cast<unsigned char>(c))) {
			return false;
		}
	}
	uint64_t parsed = 0;
	for (char c : token) {
		parsed = parsed * 10 + static_cast<uint64_t>(c - '0');
		if (parsed > 0xFFFFFFFFull) {
			return false;
		}
	}
	value = static_cast<uint32_t>(parsed);
	return true;
}

bool InstructionParser::parseUint16(const std::string& token, uint16_t& value) {
	uint32_t parsed = 0;
	if (!parseUnsignedInteger(token, parsed)) {
		return false;
	}
	// Spec: uint16 values are clamped to [0, 65535] rather than rejected or wrapped.
	value = (parsed > 65535u) ? static_cast<uint16_t>(65535) : static_cast<uint16_t>(parsed);
	return true;
}

bool InstructionParser::parseHexAddress(const std::string& token, int& address) {
	if (token.size() < 3 || token[0] != '0' || (token[1] != 'x' && token[1] != 'X')) {
		return false;
	}
	uint64_t parsed = 0;
	for (size_t i = 2; i < token.size(); i++) {
		char c = token[i];
		int digit;
		if (std::isdigit(static_cast<unsigned char>(c))) {
			digit = c - '0';
		}
		else if (c >= 'a' && c <= 'f') {
			digit = c - 'a' + 10;
		}
		else if (c >= 'A' && c <= 'F') {
			digit = c - 'A' + 10;
		}
		else {
			return false;
		}
		parsed = parsed * 16 + static_cast<uint64_t>(digit);
		if (parsed > 0x7FFFFFFFull) {
			return false; // absurd address; the bounds check downstream expects a sane int
		}
	}
	address = static_cast<int>(parsed);
	return true;
}

bool InstructionParser::extractQuotedPayload(const std::string& rawCommand, std::string& payload) {
	size_t first = rawCommand.find('"');
	size_t last = rawCommand.rfind('"');
	if (first == std::string::npos || last == std::string::npos || last <= first) {
		return false; // no quoted instruction block at all
	}

	std::string raw = rawCommand.substr(first + 1, last - first - 1);

	// Undo the shell-style escaping used in the spec's own sample: PRINT(\"Result: \" + varC)
	std::string unescaped;
	unescaped.reserve(raw.size());
	for (size_t i = 0; i < raw.size(); i++) {
		if (raw[i] == '\\' && i + 1 < raw.size() && raw[i + 1] == '"') {
			unescaped += '"';
			i++;
		}
		else {
			unescaped += raw[i];
		}
	}

	payload = trim(unescaped);
	return !payload.empty();
}

std::vector<std::string> InstructionParser::splitInstructions(const std::string& payload) {
	std::vector<std::string> parts;
	std::string current;
	int depth = 0;
	bool inQuote = false;

	for (char c : payload) {
		if (c == '"') {
			inQuote = !inQuote;
		}
		else if (!inQuote && (c == '(' || c == '[')) {
			depth++;
		}
		else if (!inQuote && (c == ')' || c == ']')) {
			depth--;
		}

		if (c == ';' && !inQuote && depth == 0) {
			parts.push_back(trim(current));
			current.clear();
		}
		else {
			current += c;
		}
	}
	parts.push_back(trim(current));

	// A trailing semicolon is tolerated; blank segments in the middle are not, so they are
	// kept here and rejected later by parseOne as an unknown opcode.
	while (!parts.empty() && parts.back().empty()) {
		parts.pop_back();
	}
	return parts;
}

std::vector<std::string> InstructionParser::tokenize(const std::string& text) {
	std::vector<std::string> tokens;
	std::istringstream stream(text);
	std::string token;
	while (stream >> token) {
		tokens.push_back(token);
	}
	return tokens;
}

bool InstructionParser::extractCallBody(const std::string& text, const std::string& opcode, std::string& inner) {
	std::string trimmed = trim(text);
	if (trimmed.size() <= opcode.size() || trimmed.compare(0, opcode.size(), opcode) != 0) {
		return false;
	}
	size_t open = trimmed.find('(', opcode.size());
	size_t close = trimmed.rfind(')');
	if (open == std::string::npos || close == std::string::npos || close <= open) {
		return false;
	}
	if (!trim(trimmed.substr(opcode.size(), open - opcode.size())).empty()) {
		return false; // junk between the opcode and its '('
	}
	if (!trim(trimmed.substr(close + 1)).empty()) {
		return false; // junk after the closing ')'
	}
	inner = trim(trimmed.substr(open + 1, close - open - 1));
	return true;
}

std::string InstructionParser::resolveOperand(int pid, const std::string& token,
	std::vector<std::shared_ptr<Instruction>>& prelude,
	std::vector<std::string>& declaredConstants) {

	if (isValidIdentifier(token)) {
		return token;
	}

	uint16_t literal = 0;
	if (!parseUint16(token, literal)) {
		return ""; // neither a variable name nor an integer literal
	}

	// AddInstruction/SubtractInstruction only accept variable names, so an integer operand is
	// materialised once as a named constant and declared ahead of the user's program.
	std::string constantName = "__const" + std::to_string(literal);
	if (std::find(declaredConstants.begin(), declaredConstants.end(), constantName) == declaredConstants.end()) {
		declaredConstants.push_back(constantName);
		prelude.push_back(std::make_shared<DeclareInstruction>(pid, constantName, literal));
	}
	return constantName;
}

std::shared_ptr<Instruction> InstructionParser::parsePrint(int pid, const std::string& text) {
	std::string inner;
	if (!extractCallBody(text, "PRINT", inner) || inner.empty()) {
		return nullptr;
	}

	if (inner[0] == '"') {
		size_t closing = inner.find('"', 1);
		if (closing == std::string::npos) {
			return nullptr; // unterminated string literal
		}
		std::string literal = inner.substr(1, closing - 1);
		std::string rest = trim(inner.substr(closing + 1));

		if (rest.empty()) {
			return std::make_shared<PrintInstruction>(pid, literal);
		}
		if (rest[0] != '+') {
			return nullptr;
		}
		std::string variableName = trim(rest.substr(1));
		if (!isValidIdentifier(variableName)) {
			return nullptr;
		}
		return std::make_shared<PrintInstruction>(pid, literal, variableName);
	}

	// PRINT(varA) -- no literal prefix, just the variable's value.
	if (!isValidIdentifier(inner)) {
		return nullptr;
	}
	return std::make_shared<PrintInstruction>(pid, "", inner);
}

std::shared_ptr<Instruction> InstructionParser::parseFor(int pid, const std::string& text, int depth,
	std::vector<std::shared_ptr<Instruction>>& prelude,
	std::vector<std::string>& declaredConstants) {

	if (depth >= MAX_FOR_DEPTH) {
		return nullptr; // nesting cap, same value the random generator uses
	}

	std::string inner;
	if (!extractCallBody(text, "FOR", inner) || inner.empty() || inner[0] != '[') {
		return nullptr;
	}

	// Walk to the ']' that matches the opening '[', ignoring brackets inside string literals.
	int bracketDepth = 0;
	bool inQuote = false;
	size_t closing = std::string::npos;
	for (size_t i = 0; i < inner.size(); i++) {
		char c = inner[i];
		if (c == '"') {
			inQuote = !inQuote;
		}
		else if (!inQuote && c == '[') {
			bracketDepth++;
		}
		else if (!inQuote && c == ']') {
			bracketDepth--;
			if (bracketDepth == 0) {
				closing = i;
				break;
			}
		}
	}
	if (closing == std::string::npos) {
		return nullptr; // unbalanced brackets
	}

	std::string bodyText = inner.substr(1, closing - 1);
	std::string rest = trim(inner.substr(closing + 1));
	if (rest.empty() || rest[0] != ',') {
		return nullptr;
	}

	uint32_t repeats = 0;
	if (!parseUnsignedInteger(trim(rest.substr(1)), repeats) || repeats == 0) {
		return nullptr;
	}

	std::vector<std::shared_ptr<Instruction>> body;
	for (const auto& part : splitInstructions(bodyText)) {
		auto child = parseOne(pid, part, depth + 1, prelude, declaredConstants);
		if (!child) {
			return nullptr;
		}
		body.push_back(child);
	}
	if (body.empty()) {
		return nullptr;
	}

	return std::make_shared<ForInstruction>(pid, body, static_cast<int>(repeats));
}

std::shared_ptr<Instruction> InstructionParser::parseOne(int pid, const std::string& text, int depth,
	std::vector<std::shared_ptr<Instruction>>& prelude,
	std::vector<std::string>& declaredConstants) {

	std::string trimmed = trim(text);
	if (trimmed.empty()) {
		return nullptr;
	}

	// PRINT and FOR carry parentheses, so they are matched on their prefix before tokenising.
	if (trimmed.compare(0, 5, "PRINT") == 0) {
		return parsePrint(pid, trimmed);
	}
	if (trimmed.compare(0, 3, "FOR") == 0) {
		return parseFor(pid, trimmed, depth, prelude, declaredConstants);
	}

	std::vector<std::string> tokens = tokenize(trimmed);
	if (tokens.empty()) {
		return nullptr;
	}
	const std::string& opcode = tokens[0];

	if (opcode == "DECLARE") {
		if (tokens.size() != 3 || !isValidIdentifier(tokens[1])) {
			return nullptr;
		}
		uint16_t value = 0;
		if (!parseUint16(tokens[2], value)) {
			return nullptr;
		}
		return std::make_shared<DeclareInstruction>(pid, tokens[1], value);
	}

	if (opcode == "ADD" || opcode == "SUBTRACT") {
		if (tokens.size() != 4 || !isValidIdentifier(tokens[1])) {
			return nullptr; // destination must always be a real variable
		}
		std::string left = resolveOperand(pid, tokens[2], prelude, declaredConstants);
		std::string right = resolveOperand(pid, tokens[3], prelude, declaredConstants);
		if (left.empty() || right.empty()) {
			return nullptr;
		}
		if (opcode == "ADD") {
			return std::make_shared<AddInstruction>(pid, tokens[1], left, right);
		}
		return std::make_shared<SubtractInstruction>(pid, tokens[1], left, right);
	}

	if (opcode == "READ") {
		if (tokens.size() != 3 || !isValidIdentifier(tokens[1])) {
			return nullptr;
		}
		int address = 0;
		if (!parseHexAddress(tokens[2], address)) {
			return nullptr;
		}
		return std::make_shared<ReadInstruction>(pid, tokens[1], address);
	}

	if (opcode == "WRITE") {
		if (tokens.size() != 3) {
			return nullptr;
		}
		int address = 0;
		if (!parseHexAddress(tokens[1], address)) {
			return nullptr;
		}
		uint16_t literal = 0;
		if (parseUint16(tokens[2], literal)) {
			return std::make_shared<WriteInstruction>(pid, address, literal);
		}
		if (isValidIdentifier(tokens[2])) {
			return std::make_shared<WriteInstruction>(pid, address, tokens[2]);
		}
		return nullptr;
	}

	if (opcode == "SLEEP") {
		if (tokens.size() != 2) {
			return nullptr;
		}
		uint32_t ticks = 0;
		if (!parseUnsignedInteger(tokens[1], ticks)) {
			return nullptr;
		}
		if (ticks > 255) {
			ticks = 255; // SleepInstruction stores a uint8_t tick count
		}
		return std::make_shared<SleepInstruction>(pid, static_cast<uint8_t>(ticks));
	}

	return nullptr; // unknown opcode
}

bool InstructionParser::parseAll(int pid, const std::string& payload,
	std::vector<std::shared_ptr<Instruction>>& instructions) {

	instructions.clear();

	std::vector<std::string> statements = splitInstructions(payload);
	int statementCount = static_cast<int>(statements.size());
	if (statementCount < MIN_INSTRUCTION_COUNT || statementCount > MAX_INSTRUCTION_COUNT) {
		return false; // spec: "Throws 'invalid command' if the instruction size is not met."
	}

	std::vector<std::shared_ptr<Instruction>> prelude;
	std::vector<std::string> declaredConstants;
	std::vector<std::shared_ptr<Instruction>> parsed;

	for (const auto& statement : statements) {
		auto instruction = parseOne(pid, statement, 0, prelude, declaredConstants);
		if (!instruction) {
			return false;
		}
		parsed.push_back(instruction);
	}

	// Integer literals used as ADD/SUBTRACT operands are declared ahead of the user program.
	instructions.insert(instructions.end(), prelude.begin(), prelude.end());
	instructions.insert(instructions.end(), parsed.begin(), parsed.end());
	return true;
}
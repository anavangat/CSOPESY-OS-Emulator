#pragma once
#include <cstdint>
#include <cstdlib>
#include <vector>

class MemoryConfigUtils
{
public:
	static constexpr uint32_t MIN_MEMORY_VALUE = 64;
	static constexpr uint32_t MAX_MEMORY_VALUE = 65536;

	static bool isPowerOfTwo(uint32_t value) {
		return (value > 0) && ((value & (value - 1)) == 0);
	}

	static bool isValidMemoryValue(uint32_t value) {
		return value >= MIN_MEMORY_VALUE && value <= MAX_MEMORY_VALUE && isPowerOfTwo(value);
	}

	static uint32_t rollMemoryPerProcess(uint32_t min, uint32_t max) {
		// Ensure the range is valid
		if (min > max) {
			return min;
		}

		// Generate a list of valid power-of-two candidates within the range
		std::vector<uint32_t> candidates;
		for (uint32_t value = min; value <= max; value <<= 1) {
			candidates.push_back(value);
		}

		// Randomly select one of the candidates
		size_t randomIndex = static_cast<size_t>(std::rand()) % candidates.size();
		return candidates[randomIndex];
	}
};


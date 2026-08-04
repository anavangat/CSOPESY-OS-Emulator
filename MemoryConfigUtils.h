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

	static uint32_t nextPowerOfTwo(uint32_t value) {
		if (value <= MIN_MEMORY_VALUE) {
			return MIN_MEMORY_VALUE;
		}
		if (value >= MAX_MEMORY_VALUE) {
			return MAX_MEMORY_VALUE;
		}

		uint32_t result = MIN_MEMORY_VALUE;
		while (result < value) {
			result <<= 1;
		}
		return result;
	}

	static uint32_t rollMemoryPerProcess(uint32_t min, uint32_t max) {
		// Normalise both bounds into the spec range [2^6, 2^16] first. Without this a config
		// such as "min-mem-per-proc 0" produced a value <<= 1 loop that never advanced, and a
		// non-power-of-two min produced an empty candidate list -> modulo by zero.
		uint32_t low = nextPowerOfTwo(min);
		uint32_t high = nextPowerOfTwo(max);

		if (low > high) {
			uint32_t temp = low;
			low = high;
			high = temp;
		}

		// Generate a list of valid power-of-two candidates within the range
		std::vector<uint32_t> candidates;
		for (uint32_t value = low; value <= high; value <<= 1) {
			candidates.push_back(value);
		}

		if (candidates.empty()) {
			return MIN_MEMORY_VALUE; // defensive; unreachable once both bounds are normalised
		}

		// Randomly select one of the candidates
		size_t randomIndex = static_cast<size_t>(std::rand()) % candidates.size();
		return candidates[randomIndex];
	}
};
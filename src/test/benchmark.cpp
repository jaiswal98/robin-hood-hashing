#include "test_base.h"

#include <map>
#include <unordered_map>

struct NullHash {
	size_t operator()(uint64_t val) const {
		return static_cast<size_t>(val);
	}
};

TEMPLATE_TEST_CASE("benchmark random insert erase", "[!benchmark]", (robin_hood::flat_map<uint64_t, uint64_t>),
				   (robin_hood::node_map<uint64_t, uint64_t>), (std::unordered_map<uint64_t, uint64_t>), (std::map<uint64_t, uint64_t>)) {
	Rng rng(123);
	TestType map;

	static const size_t maxVal = 2000;

	BENCHMARK("Random insert erase") {
		for (uint64_t i = 0; i < 100'000'000; ++i) {
			map[rng(maxVal)] = i;
			map.erase(rng(maxVal));
		}
	}

	uint64_t sum_key = 0;
	uint64_t sum_val = 0;
	BENCHMARK("iterating") {
		for (int i = 0; i < 1000; ++i) {
			for (auto const& kv : map) {
				sum_key += kv.first;
				sum_val += kv.second;
			}
		}
	}
	REQUIRE(map.size() == 993);
	REQUIRE(sum_key + sum_val == 0x5a5017eef9c8);
	BENCHMARK("clear") {
		map.clear();
	}
}

TEMPLATE_TEST_CASE("benchmark hashing", "[!benchmark]", (std::hash<std::string>), (robin_hood::hash<std::string>)) {
	size_t h = 0;
	Rng rng(123);
	auto hasher = TestType{};
	for (size_t s : {8, 11, 100, 1024}) {
		std::string str(s, 'x');
		for (size_t i = 0; i < str.size(); ++i) {
			str[i] = rng.uniform<unsigned char>();
		}
		BENCHMARK("std::string length " + std::to_string(str.size())) {
			for (size_t i = 0; i < 10000000000 / s; ++i) {
				h += hasher(str);
			}
		}
	}
	std::cout << h << std::endl;
}

// dummy map for overhead calculation. Makes use of key so it can't be optimized away.
template <typename Key, typename Val>
class DummyMapForOverheadCalculation {
public:
	DummyMapForOverheadCalculation()
		: m_val{} {}

	Val& operator[](Key const& key) {
		return m_val[key & 1];
	}

	size_t erase(Key const& key) {
		auto r = m_val[key & 1];
		m_val[key & 1] = 0;
		return r;
	}

	void clear() {}

private:
	Val m_val[2];
};

TEMPLATE_TEST_CASE("benchmark int", "[!benchmark]", (robin_hood::flat_map<int, int>), (robin_hood::node_map<int, int>),
				   (std::unordered_map<int, int>)) {
	Rng rng(123);
	TestType map;

	uint64_t verifier = 0;
	BENCHMARK("Random insert erase") {
		for (int n = 1; n < 10'000; ++n) {
			for (int i = 0; i < 10'000; ++i) {
				map[rng.uniform<int>((uint64_t)n)] = i;
				verifier += map.erase(rng.uniform<int>((uint64_t)n));
			}
		}
	}

	REQUIRE(verifier == 50024052);

	BENCHMARK("clear") {
		map.clear();
	}
}

// benchmark adapted from https://github.com/attractivechaos/udb2
// this implementation should have less overhead, because sfc64 and it's uniform() is extremely fast.
TEMPLATE_TEST_CASE("25% distinct", "[!benchmark]", (robin_hood::unordered_map<int, int>), (std::unordered_map<int, int>)) {
	Rng rng(123);

	int checksum = 0;
	size_t const upper = 50'000'000;
	size_t const lower = 10'000'000;
	size_t const num_steps = 5;
	size_t const step_width = (upper - lower) / num_steps;

	for (size_t n = lower; n <= upper; n += step_width) {
		size_t const max_rng = n / 4;
		BENCHMARK(std::to_string(n) + " ") {
			TestType map;
			for (size_t i = 0; i < n; ++i) {
				checksum += ++map[rng.uniform<int>(max_rng)];
			}
		}
	}

	REQUIRE(checksum == 539967125);
}
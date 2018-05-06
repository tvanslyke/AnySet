#include "any-set.h"

TEST_CASE("Erase", "[erase]") {

	using namespace te;
	SECTION("Erasing all elements in a set produces the empty set.") {
		any_set_t set1(string_names.begin(), string_names.end());
		any_set_t set2(string_names.begin(), string_names.end());
		any_set_t set3(string_names.begin(), string_names.end());
		any_set_t clear_set(string_names.begin(), string_names.end());
		for(auto pos = set1.begin(); pos != set1.end(); pos = set1.erase(pos))
		{ /* LOOP */ }
		REQUIRE(set1.empty());
		set2.erase(set2.begin(), set2.end());
		REQUIRE(set2.empty());
		for(const auto& name: string_names)
		{
			auto old_size = set3.size();
			auto count = set3.erase(name);
			REQUIRE(count == 1u);
			REQUIRE(set3.size() == (old_size - 1));
		}
		REQUIRE(set3.empty());
		clear_set.clear();
		REQUIRE(set1 == clear_set);
		REQUIRE(set2 == clear_set);
		REQUIRE(set3 == clear_set);
	}

	SECTION("Erase returns 0 when an element doesn't exist, 1 otherwise") {
		any_set_t set(string_names.begin(), string_names.end());
		REQUIRE(set.size() != 0u);
		REQUIRE(set.erase(string_names.front()) == 1u);
		REQUIRE(set.erase(string_names.front()) == 0u);
		REQUIRE(set.erase(string_names.back()) == 1u);
		REQUIRE(set.erase(string_names.back()) == 0u);
		REQUIRE(set.erase(std::size_t(1)) == 0u);
		set.insert(string_names.front());
		REQUIRE(set.erase(string_names.front()) == 1u);
		REQUIRE(set.erase(string_names.front()) == 0u);
	}

	SECTION("Erasure of an existing element produces 0 for subsequent .count() calls") {
		any_set_t set(string_names.begin(), string_names.end());
		for(const auto& name: string_names)
		{
			REQUIRE(set.count(name) == 1u);
			REQUIRE(set.erase(name) == 1u);
			REQUIRE(set.count(name) == 0u);
			auto [_, success] = set.insert(name);
			REQUIRE(success);
			REQUIRE(set.count(name) == 1u);
		}
	}
}

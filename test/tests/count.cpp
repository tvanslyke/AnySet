#include "any-set.h"

TEST_CASE("Count", "[count]") {

	using namespace te;
	using namespace std::literals;
	SECTION("An empty set always returns count() == 0") {
		any_set_t set;
		REQUIRE(set.count("asdf"s) == 0u);
		REQUIRE(set.count(1) == 0u);
		REQUIRE(set.count(1ull) == 0u);
	}

	SECTION("Count of an inserted element is 1") {
		any_set_t set;
		set.insert("asdf"s);
		REQUIRE(set.count("asdf"s) == 1u);
		set.insert(1);
		REQUIRE(set.count(1) == 1u);
		set.insert(1ull);
		REQUIRE(set.count(1ull) == 1u);
	}

	SECTION("Count of an erased element is 0") {
		any_set_t set;
		set.insert("asdf"s);
		set.insert(1);
		set.erase("asdf"s);
		set.insert(1ull);
		REQUIRE(set.count("asdf"s) == 0u);
		set.erase(1);
		REQUIRE(set.count(1) == 0u);
		set.erase(1ull);
		REQUIRE(set.count(1ull) == 0u);
	}

	SECTION("Count of values not inserted is 0") {
		any_set_t set;
		set.rehash(8);
		for(std::size_t i = 0; i < 8u; ++i)
			set.insert(i);
		for(std::size_t i = 0; i < 8u; ++i)
		{
			REQUIRE(set.count(i) == 1u);
		}
		for(std::size_t i = 8u; i < 16u; ++i)
		{
			REQUIRE(set.count(i) == 0u);
		}
	}
}

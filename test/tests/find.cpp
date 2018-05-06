#include "any-set.h"

TEST_CASE("Find", "[find]") {

	using namespace te;
	using namespace std::literals;
	SECTION("An empty set always returns find() == end()") {
		any_set_t set;
		REQUIRE(set.find("asdf"s) == set.cend());
		REQUIRE(set.find(1) == set.cend());
		REQUIRE(set.find(1ull) == set.cend());
	}

	SECTION("Finding an inserted element returns an iterator to the element") {
		any_set_t set;
		set.insert("asdf"s);
		REQUIRE(set.find("asdf"s) != set.cend());
		REQUIRE(*set.find("asdf"s) == "asdf"s);
		set.insert(1);
		REQUIRE(set.find(1) != set.cend());
		REQUIRE(*set.find(1) == 1);
		set.insert(1ull);
		REQUIRE(set.find(1ull) != set.cend());
		REQUIRE(*set.find(1ull) == 1ull);
	}

	SECTION("Finding an erased element returns end()") {
		any_set_t set;
		set.insert("asdf"s);
		set.insert(1);
		set.insert(1ull);
		set.erase("asdf"s);
		REQUIRE(set.find("asdf"s) == set.cend());
		set.erase(1);
		REQUIRE(set.find(1) == set.cend());
		set.erase(1ull);
		REQUIRE(set.find(1ull) == set.cend());
	}

	SECTION("Finding a valu not inserted returns end()") {
		any_set_t set;
		set.rehash(8);
		for(std::size_t i = 0; i < 8u; ++i)
			set.insert(i);
		for(std::size_t i = 0; i < 8u; ++i)
		{
			REQUIRE(set.find(i) != set.cend());
			REQUIRE(*set.find(i) == i);
			REQUIRE(*set.find(i) != (i - 1));
		}
		for(std::size_t i = 8u; i < 16u; ++i)
		{
			REQUIRE(set.find(i) == set.cend());
		}
	}
}

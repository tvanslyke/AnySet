#include "any-set.h"

TEST_CASE("Empty", "[empty]") {

	using namespace te;
	SECTION("A default-constructed set is always empty") {
		any_set_t set;
		REQUIRE(set.empty());
	}

	SECTION("A set is only elements if it constains zero elements") {
		any_set_t set(string_names.begin(), string_names.end());
		REQUIRE(not set.empty());
		set.clear();
		REQUIRE(set.empty());
		set.insert(string_names.begin(), string_names.end());
		REQUIRE(not set.empty());
		for(const auto& name : string_names)
			set.erase(name);
		REQUIRE(set.empty());
		set.clear();
		REQUIRE(set.empty());
		set.rehash(16);
		REQUIRE(set.empty());
	}
}

#include "any-set.h"

TEST_CASE("Eq", "[eq]") {

	using namespace te;
	SECTION("A non-empty set is never equal to the empty set") {
		any_set_t set;
		any_set_t other(string_names.begin(), string_names.end());
		REQUIRE(set.empty());
		REQUIRE(not other.empty());
		REQUIRE(not (set == other));
		REQUIRE(set != other);
	}

	SECTION("Two sets constructed from the same elements compare equal.") {
		any_set_t set(string_names.begin(), string_names.end());
		any_set_t other(string_names.begin(), string_names.end());
		REQUIRE(set == other);
		REQUIRE(not (set != other));
	}
	
	SECTION("Copy construction implies equality.") {
		any_set_t set(string_names.begin(), string_names.end());
		any_set_t other(set);
		REQUIRE(set == other);
		REQUIRE(not (set != other));
	}

	SECTION("Copy assignment implies equality.") {
		any_set_t set(string_names.begin(), string_names.end());
		any_set_t other;
		other.insert(1);
		other = set;
		REQUIRE(set == other);
		REQUIRE(not (set != other));
	}

	SECTION("Rehashing does not affect equality.") {
		any_set_t set(string_names.begin(), string_names.end());
		any_set_t other(set);
		REQUIRE(set == other);
		REQUIRE(not (set != other));
		set.rehash(128);
		REQUIRE(set == other);
		REQUIRE(not (set != other));
		set.insert(20);
		REQUIRE(not (set == other));
		REQUIRE(set != other);
		other.insert(20);
		REQUIRE(set == other);
		REQUIRE(not (set != other));
	}
}

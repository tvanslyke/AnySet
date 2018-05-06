#include "any-set.h"

TEST_CASE("Clear", "[clear]") {

	using namespace te;
	SECTION("Clearing a default-constructed set ") {
		any_set_t set;
		REQUIRE(set.size() == 0u);
		set.clear();
		REQUIRE(set.size() == 0u);
	}

	SECTION("Clearing a range-constructed set") {
		any_set_t set(string_names.begin(), string_names.end());
		REQUIRE(set.size() != 0u);
		set.clear();
		REQUIRE(set.size() == 0u);
	}

	SECTION("Clearing an empty set after rehashing") {
		any_set_t set;
		set.rehash(16);
		REQUIRE(set.size() == 0u);
		REQUIRE(set.bucket_count() == 16u);
		set.clear();
		REQUIRE(set.size() == 0u);
		REQUIRE(set.bucket_count() == 16u);
	}

}

#include "any-set.h"

TEST_CASE("Max Load Factor", "[max-load-factor]") {

	using namespace te;
	using namespace std::literals;
	SECTION("The max_load_factor() member function can be used to both set and query the max load factor") {
		any_set_t set;
		REQUIRE(set.max_load_factor() >= 0.0);
		set.max_load_factor(3.1415f);
		REQUIRE(set.max_load_factor() == 3.1415f);
	}
}

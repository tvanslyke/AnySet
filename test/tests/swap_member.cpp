#include "any-set.h"
#include <numeric>

TEST_CASE("Swap", "[swap-member]") {

	using namespace te;
	SECTION("Swapping does the obvious thing") {
		{
			any_set_t set1{1, 2, 3, 4};
			any_set_t set2{0, 1, 2, 3};
			REQUIRE(set1 != set2);
			auto cp1 = set1;
			auto cp2 = set2;
			set1.swap(set2);
			REQUIRE(set1 == cp2);
			REQUIRE(set2 == cp1);
			cp1.swap(cp2);
			REQUIRE(set1 == cp1);
			REQUIRE(set2 == cp2);
		}
	}
}

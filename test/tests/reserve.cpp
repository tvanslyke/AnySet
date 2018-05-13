#include "any-set.h"

TEST_CASE("Reserve", "[reserve]") {

	using namespace te;

	SECTION("Reserving N ensures a rehash won't occur unless the set's size exceeds N elements") {
		any_set_t set{1, 2, 3, 4, 5, 6, 7, 8};
		set.rehash(8);
		REQUIRE(set.load_factor() == 1.0);
		for(std::size_t i = 9; i < 33; ++i)
		{
			set.reserve(set.size() + 1);
			auto buck_count = set.bucket_count();
			auto sz = set.size();
			set.insert(i);
			REQUIRE(set.size() == (sz + 1));
			REQUIRE(set.bucket_count() == buck_count);
		}
	}
}

#include "any-set.h"

TEST_CASE("Max Bucket Count", "[max-bucket-count]") {

	using namespace te;
	using namespace std::literals;
	SECTION("Max bucket count is positive and nonzero") {
		assert(any_set_t{}.max_size() > 0u);
	}
}

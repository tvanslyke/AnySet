#include "any-set.h"


TEST_CASE("Bucket Size", "[bucket-size]") {

	using namespace te;
	{
		any_set_t set;
		set.rehash(8);
		REQUIRE(set.bucket_count() == 8);
		for(std::size_t i = 0; i < set.bucket_count(); ++i)
		{
			REQUIRE(set.bucket_size(i) == 0u);
		}
		set.insert(std::size_t(0));
		REQUIRE(set.bucket_size(0) == 1u);
		set.insert(std::size_t(set.bucket_count()));
		REQUIRE(set.bucket_size(0) == 2u);
		size_type new_count = 2 * set.bucket_count();
		set.rehash(new_count);
		REQUIRE(set.bucket_size(0) == 1u);
		REQUIRE(set.bucket_size(new_count / 2) == 1u);
		set.insert(string_names.begin(), string_names.end());
		for(std::size_t i = 0; i < set.bucket_count(); ++i)
		{
			REQUIRE(set.bucket_size(i) == std::distance(set.begin(i), set.end(i)));
		}
	}
}

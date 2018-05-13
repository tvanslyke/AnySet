#include "any-set.h"


TEST_CASE("Bucket", "[bucket]") {

	using namespace te;
	SECTION("Default-constructed set has 1 bucket, all keys map to bucket 0") {
		any_set_t set;
		REQUIRE(set.bucket_count() == 1u);
		for(std::size_t i = 0; i < 10; ++i)
		{
			REQUIRE(set.bucket(i) == 0);
		}
		REQUIRE(set.bucket(std::string("some arbitrary string")) == 0);
	}
	
	SECTION("Set with 2^N buckets maps keys to the range [0, 2^N)") {
		any_set_t set;
		set.rehash(16u);
		REQUIRE(set.bucket_count() == 16u);
		for(std::size_t i = 0; i < 32; ++i)
		{
			REQUIRE(set.bucket(i) == (i % 16u));
		}
	}
}

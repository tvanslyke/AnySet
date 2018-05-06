#include "any-set.h"
#include <numeric>

TEST_CASE("Bucket Count", "[bucket-count]") {

	using namespace te;
	SECTION("Default-constructed set has 1 bucket") {
		any_set_t set;
		REQUIRE(set.bucket_count() == 1);
	}
	
	SECTION("Range-contructed set has N or more buckets") {
		{
			std::array<int, 3> init{0, 1, 2};
			any_set_t set(begin(init), end(init));
			REQUIRE(set.bucket_count() >= init.size());
		}
		{
			std::array<int, 4> init{0, 1, 2, 3};
			any_set_t set(begin(init), end(init));
			REQUIRE(set.bucket_count() >= init.size());
		}
		{
			std::array<int, 5> init{0, 1, 2, 3, 4};
			any_set_t set(begin(init), end(init));
			REQUIRE(set.bucket_count() >= init.size());
		}
	}
	
	SECTION("Number of buckets is always a power of 2 greater than zero") {
		auto is_power_of_two = [](size_type bc, size_type minm = 1) {
			return ((bc & (bc - 1)) == 0u);
		};
		{
			any_set_t set;
			set.rehash(128);
			REQUIRE(is_power_of_two(set.bucket_count()));
			REQUIRE(set.bucket_count() == 128u);
		}
		{
			any_set_t set;
			set.rehash(63);
			REQUIRE(is_power_of_two(set.bucket_count()));
			REQUIRE(set.bucket_count() == 64u);
		}
		{
			std::vector<std::size_t> v(64);
			std::iota(begin(v), end(v), 0u);
			any_set_t set(begin(v), end(v));
			REQUIRE(is_power_of_two(set.bucket_count()));
			REQUIRE(set.bucket_count() == 64u);
			set.insert(100);
			REQUIRE(is_power_of_two(set.bucket_count()));
			REQUIRE(set.bucket_count() == 128u);
		}
	}
}

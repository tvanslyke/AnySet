#include "any-set.h"


TEST_CASE("Local Iterators", "[local-iterators]") {

	using namespace te;
	SECTION("Iterating over the local iterators of every bucket iterates over all elements") {
		std::array<int, 10> v{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
		any_set_t set(v.begin(), v.end());
		REQUIRE(set.size() == v.size());
		REQUIRE(set.bucket_count() >= v.size());
		size_type expect = v.size();
		size_type total = 0;
		for(size_type i = 0; i < set.bucket_count(); ++i)
			total += std::distance(set.begin(i), set.end(i));
		REQUIRE(total == expect);
		set._assert_invariants();
	}

	SECTION("Local iterators iterate over all of the elements in their respective buckets.  No more, no less.") {
		// size_t is specialized to the identity hash (test suite only)
		std::array<std::size_t, 10> v{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
		any_set_t set(v.begin(), v.end());
		REQUIRE(set.size() == v.size());
		REQUIRE(set.bucket_count() >= v.size());
		std::size_t buck_count = set.bucket_count();
		// add an element to bucket 0
		auto [pos, success] = set.emplace<std::size_t>(buck_count);
		REQUIRE(success);
		// shouldn't have triggered a rehash. if it did, then this test is incorrect
		REQUIRE(set.bucket_count() == buck_count);
		REQUIRE(std::distance(set.begin(0), set.end(0)) == 2);
		{
			// first bucket should hold the values {0, buck_count}
			auto* first = try_as<std::size_t>(*set.begin(0));
			REQUIRE(static_cast<bool>(first));
			auto* second = try_as<std::size_t>(*std::next(set.begin(0)));
			REQUIRE(static_cast<bool>(second));
			std::array<std::size_t, 2> first_buck{*first, *second};
			std::array<std::size_t, 2> tmp{0, buck_count};
			REQUIRE(std::is_permutation(begin(first_buck), end(first_buck), begin(tmp), end(tmp)));
		}
		for(size_type i = 1; i < v.back(); ++i)
		{
			REQUIRE(std::distance(set.begin(i), set.end(i)) == 1);
			REQUIRE(as<std::size_t>(*set.begin(i)) == i);
		}
		set._assert_invariants();
	}
}

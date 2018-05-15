#include "any-set.h"

TEST_CASE("Splice", "[splice]") {

	using namespace te;
	using namespace std::literals;

	SECTION("Splicing is equivalent to a push(pop()) when the destination set doesn't contain the popped value") {
		any_set_t set({0, 1, 2, 3, 4, 5, 6, 7, 8, 9});
		any_set_t other;
		for(int i: {4, 2, 6, 3, 9, 5, 0, 7, 1, 8})
		{
			auto a = set;
			auto b = other;
			REQUIRE(set.contains(i));
			REQUIRE(not other.contains(i));
			other.push(set.pop(set.find(i)).first);
			REQUIRE(not set.contains(i));
			REQUIRE(other.contains(i));
			REQUIRE(a.contains(i));
			REQUIRE(not b.contains(i));
			REQUIRE(std::get<2>(b.splice(a, a.find(i))));
			REQUIRE(not a.contains(i));
			REQUIRE(b.contains(i));
			REQUIRE(a == set);
			REQUIRE(b == other);
		}
	}
	
	SECTION("Splicing reports 'false' if the destination set already contains the value") {
		any_set_t set({0, 1, 2, 3, 4, 5, 6, 7, 8, 9});
		any_set_t other({0, 1, 2, 3, 4});
		for(int i: {0, 1, 2, 3, 4})
		{
			auto size = set.size();
			auto other_size = other.size();
			auto pos = set.find(i);
			REQUIRE(other.contains(i));
			REQUIRE(set.contains(i));
			auto [ins, next, moved] = other.splice(set, pos);
			REQUIRE(not moved);
			REQUIRE(size == set.size());
			REQUIRE(other_size == other.size());
			REQUIRE(std::next(pos) == next);
			REQUIRE(*ins == i);
			REQUIRE(*ins == *pos);
			REQUIRE(other.contains(i));
			REQUIRE(set.contains(i));
		}
	}

	SECTION("Splicing reports 'true' if the destination set didn't already contain the value") {
		any_set_t set({0, 1, 2, 3, 4, 5, 6, 7, 8, 9});
		any_set_t other({0, 1, 2, 3, 4});
		for(int i: {5, 6, 7, 8, 9})
		{
			auto size = set.size();
			auto other_size = other.size();
			auto pos = set.find(i);
			REQUIRE(not other.contains(i));
			REQUIRE(set.contains(i));
			auto [ins, next, moved] = other.splice(set, pos);
			REQUIRE(size == set.size() + 1u);
			REQUIRE(other_size == other.size() - 1u);
			REQUIRE(moved);
			REQUIRE(pos == next);
			REQUIRE(*ins == i);
			REQUIRE(other.contains(i));
			REQUIRE(not set.contains(i));
			set._assert_invariants(true);
			other._assert_invariants(true);
		}
	}

	SECTION("Range-splicing is equivalent to one-by-one splicing, but returns the range of values not inserted")
	{
		{
			any_set_t set({0, 1, 2, 3, 4, 5, 6, 7, 8, 9});
			any_set_t other({0, 1, 2, 3, 4});
			{
				auto a = set;
				auto b = other;
				auto [first, last] = set.splice(other, other.begin(), other.end());
				REQUIRE(first == other.begin());
				REQUIRE(last == other.end());
				REQUIRE(a == set);
				REQUIRE(b == other);
			}
			{
				auto a = set;
				auto b = other;
				auto [first, last] = other.splice(set, set.begin(), set.end());
				REQUIRE(std::distance(first, last) == 5);
				REQUIRE(b == set);
				REQUIRE(a == other);
				std::array<int, 5> expect{0, 1, 2, 3, 4};
				REQUIRE(std::is_permutation(first, last, expect.begin(), expect.end()));
			}
		}
	}
}




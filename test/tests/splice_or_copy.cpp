#include "any-set.h"

TEST_CASE("SpliceOrCopy", "[splice-or-copy]") {

	using namespace te;
	using namespace std::literals;

	SECTION("splice_or_copy() is equivalent to splice() when the source set is an rvalue") {
		any_set_t set({0, 1, 2, 3, 4, 5, 6, 7, 8, 9});
		any_set_t other;
		for(int i: {4, 2, 6, 3, 9, 5, 0, 7, 1, 8})
		{
			auto a = set;
			auto b = other;
			REQUIRE(set.contains(i));
			REQUIRE(not other.contains(i));
			other.splice_or_copy(std::move(set), set.find(i));
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
		set = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
		other = {0, 1, 2, 3, 4};
		for(int i: {0, 1, 2, 3, 4})
		{
			auto size = set.size();
			auto other_size = other.size();
			auto pos = set.find(i);
			REQUIRE(other.contains(i));
			REQUIRE(set.contains(i));
			auto [ins, next, moved] = other.splice_or_copy(std::move(set), pos);
			REQUIRE(not moved);
			REQUIRE(size == set.size());
			REQUIRE(other_size == other.size());
			REQUIRE(std::next(pos) == next);
			REQUIRE(*ins == i);
			REQUIRE(*ins == *pos);
			REQUIRE(other.contains(i));
			REQUIRE(set.contains(i));
		}
		set = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
		other = {0, 1, 2, 3, 4};
		for(int i: {5, 6, 7, 8, 9})
		{
			auto size = set.size();
			auto other_size = other.size();
			auto pos = set.find(i);
			REQUIRE(not other.contains(i));
			REQUIRE(set.contains(i));
			auto [ins, next, moved] = other.splice_or_copy(std::move(set), pos);
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
		{
			any_set_t set({0, 1, 2, 3, 4, 5, 6, 7, 8, 9});
			any_set_t other({0, 1, 2, 3, 4});
			{
				auto a = set;
				auto b = other;
				auto [first, last] = set.splice_or_copy(std::move(other), other.begin(), other.end());
				REQUIRE(first == other.begin());
				REQUIRE(last == other.end());
				REQUIRE(a == set);
				REQUIRE(b == other);
			}
			{
				auto a = set;
				auto b = other;
				auto [first, last] = other.splice_or_copy(std::move(set), set.begin(), set.end());
				REQUIRE(std::distance(first, last) == 5);
				REQUIRE(b == set);
				REQUIRE(a == other);
				std::array<int, 5> expect{0, 1, 2, 3, 4};
				REQUIRE(std::is_permutation(first, last, expect.begin(), expect.end()));
			}
		}
	}
	SECTION("splice_or_copy() copies elements when the source set is not an rvalue, or is const")
	{
		{
			any_set_t set({0, 1, 2, 3, 4, 5, 6, 7, 8, 9});
			any_set_t other({0, 1, 2, 3, 4});
			auto a = set;
			auto b = other;
			auto [first, last] = other.splice_or_copy(set, set.begin(), set.end());
			REQUIRE(std::distance(first, last) == 10);
			REQUIRE(a == set);
			REQUIRE(a == other);
			REQUIRE(b != set);
			REQUIRE(b != other);
			std::array<int, 10> expect{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
			REQUIRE(std::is_permutation(first, last, expect.begin(), expect.end()));
		}
		{
			const any_set_t set({0, 1, 2, 3, 4, 5, 6, 7, 8, 9});
			any_set_t other({0, 1, 2, 3, 4});
			auto a = set;
			auto b = other;
			auto [first, last] = other.splice_or_copy(std::move(set), set.begin(), set.end());
			REQUIRE(std::distance(first, last) == 10);
			REQUIRE(a == set);
			REQUIRE(a == other);
			REQUIRE(b != set);
			REQUIRE(b != other);
			std::array<int, 10> expect{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
			REQUIRE(std::is_permutation(first, last, expect.begin(), expect.end()));
		}
		{
			any_set_t set({0, 1, 2, 3, 4, 5, 6, 7, 8, 9});
			any_set_t other({0, 1, 2, 3, 4});
			for(int i: {0, 1, 2, 3, 4})
			{
				auto pos = set.find(i);
				auto [ins, next, inserted] = other.splice_or_copy(set, pos);
				REQUIRE(not inserted);
				REQUIRE(*ins == i);
				REQUIRE(std::next(pos) == next);
				REQUIRE(other.contains(i));
				std::array<int, 10> expect{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
				REQUIRE(std::is_permutation(set.begin(), set.end(), expect.begin(), expect.end()));
			}
			for(int i: {5, 6, 7, 8, 9})
			{
				auto pos = set.find(i);
				auto [ins, next, inserted] = other.splice_or_copy(set, pos);
				REQUIRE(inserted);
				REQUIRE(*ins == i);
				REQUIRE(std::next(pos) == next);
				REQUIRE(other.contains(i));
				std::array<int, 10> expect{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
				REQUIRE(std::is_permutation(set.begin(), set.end(), expect.begin(), expect.end()));
			}
		}
		{
			any_set_t set({0, 1, 2, 3, 4, 5, 6, 7, 8, 9});
			any_set_t other({0, 1, 2, 3, 4});
			set.insert(UniqueInt::make(10));
			// Copying non-copyable types throws a te::CopyConstructionError
			REQUIRE_THROWS_AS(
				other.splice_or_copy(set, set.find(UniqueInt::make(10))),
				const te::NoCopyConstructorError<UniqueInt>&
			);
			REQUIRE_THROWS_AS(
				other.splice_or_copy(set, set.find(UniqueInt::make(10))),
				const te::CopyConstructionError&
			);
		}
	}
}




#include "../any-set.h"
#include "anyset/SetOperations.h"
#include <algorithm>

TEST_CASE("DifferenceOf", "[difference-of]") {

	using namespace te;
	using namespace std::literals;

	auto is_perm = [](const auto& a, std::initializer_list<int> b) {
		return std::is_permutation(begin(a), end(a), begin(b), end(b));
	};

	SECTION("difference_of() computes the difference of one set with the union of the remaining sets") {
		{
			// 2 sets, no overlap
			any_set_t a({0, 1, 2, 3});
			any_set_t b({4, 5, 6});
			any_set_t result = difference_of(a, b);
			REQUIRE(result == a);
			// 'a' and 'b' should be unchanged
			REQUIRE(is_perm(a, {0, 1, 2, 3}));
			REQUIRE(is_perm(b, {4, 5, 6}));
			result._assert_invariants();
			a._assert_invariants();
			b._assert_invariants();

			result.clear();

			// move from 'a' and 'b'
			result = difference_of(std::move(a), std::move(b));
			REQUIRE(result == any_set_t({0, 1, 2, 3}));
			result._assert_invariants();
			a._assert_invariants();
			b._assert_invariants();


			// 'a' and 'b' are equal
			a = {0, 1, 2, 3, 4};
			b = {0, 1, 2, 3, 4};
			result = difference_of(a, b);
			REQUIRE(result.empty());
			// 'a' and 'b' should be unchanged since they weren't moved
			REQUIRE(is_perm(a, {0, 1, 2, 3, 4}));
			REQUIRE(is_perm(b, {0, 1, 2, 3, 4}));
			result._assert_invariants();
			a._assert_invariants();
			b._assert_invariants();
			
			// 'a' and 'b' are equal
			a = {0, 1, 2, 3, 4};
			b = {0, 1, 2, 3, 4};
			// move from 'a' and 'b'
			result = difference_of(std::move(a), std::move(b));
			REQUIRE(result.empty());
			result._assert_invariants();
			a._assert_invariants();
			b._assert_invariants();
		}
		{
			// 3 sets, partial overlap
			any_set_t a({0, 1, 2, 3, 4, 5});
			any_set_t b({3, 4, 5});
			any_set_t c({3, 5, 7, 9, 11});
			any_set_t result = difference_of(a, b, c);
			REQUIRE(is_perm(result, {0, 1, 2}));
			// 'a', 'b', and 'c' should be unchanged
			REQUIRE(is_perm(a, {0, 1, 2, 3, 4, 5}));
			REQUIRE(is_perm(b, {3, 4, 5}));
			REQUIRE(is_perm(c, {3, 5, 7, 9, 11}));
			result._assert_invariants();
			a._assert_invariants();
			b._assert_invariants();
			c._assert_invariants();

			result.clear();

			// move from 'a', 'b', and 'c'
			result = difference_of(std::move(a), std::move(b), std::move(c));
			REQUIRE(is_perm(result, {0, 1, 2}));
			// at least two elements should have been moved out of 'a', 'b', and 'c', collectively
			result._assert_invariants();
			a._assert_invariants();
			b._assert_invariants();
			c._assert_invariants();
		}
	}

	SECTION("operator- is equivalent to difference_of()") {
		{
			// 2 sets, no overlap
			any_set_t a({0, 1, 2, 3});
			any_set_t b({4, 5, 6});
			any_set_t result = a - b;
			REQUIRE(is_perm(result, {0, 1, 2, 3}));
			// 'a' and 'b' should be unchanged
			REQUIRE(is_perm(a, {0, 1, 2, 3}));
			REQUIRE(is_perm(b, {4, 5, 6}));
			result._assert_invariants();
			a._assert_invariants();
			b._assert_invariants();

			result.clear();

			// move from 'a' and 'b'
			result = std::move(a) - std::move(b);
			REQUIRE(is_perm(result, {0, 1, 2, 3}));
			result._assert_invariants();
			a._assert_invariants();
			b._assert_invariants();


			// 'a' and 'b' intersect entirely
			a = {0, 1, 2, 3, 4};
			b = {0, 1, 2, 3, 4};
			result = a - b;
			REQUIRE(result.empty());
			// 'a' and 'b' should be unchanged since they weren't moved
			REQUIRE(is_perm(a, {0, 1, 2, 3, 4}));
			REQUIRE(is_perm(b, {0, 1, 2, 3, 4}));
			result._assert_invariants();
			a._assert_invariants();
			b._assert_invariants();
			
			// 'a' and 'b' intersect entirely
			a = {0, 1, 2, 3, 4};
			b = {0, 1, 2, 3, 4};
			// move from 'a' and 'b'
			result = std::move(a) - std::move(b);
			REQUIRE(result.empty());
			result._assert_invariants();
			a._assert_invariants();
			b._assert_invariants();
		}
		{
			// 3 sets, partial overlap
			any_set_t a(std::make_tuple(0, 1, 2, 3, 4, 5, "abcd"s));
			any_set_t b(std::make_tuple(3, 4, 5, "abc"s));
			any_set_t c(std::make_tuple(3, 5, 7, 9, 11, "abcd"s));
			any_set_t result = (a - (b + c));
			REQUIRE(is_perm(result, {0, 1, 2}));
			// 'a', 'b', and 'c' should be unchanged
			REQUIRE(a == any_set_t(std::make_tuple(0, 1, 2, 3, 4, 5, "abcd"s)));
			REQUIRE(b == any_set_t(std::make_tuple(3, 4, 5, "abc"s)));
			REQUIRE(c == any_set_t(std::make_tuple(3, 5, 7, 9, 11, "abcd"s)));
			result._assert_invariants();
			a._assert_invariants();
			b._assert_invariants();
			c._assert_invariants();

			result.clear();

			// move from 'a', 'b', and 'c'
			result = (std::move(a) - (std::move(b) + std::move(c)));
			REQUIRE(is_perm(result, {0, 1, 2}));
			result._assert_invariants();
			a._assert_invariants();
			b._assert_invariants();
			c._assert_invariants();
		}

	}

	SECTION("difference_of() empty sets is an empty set") {
		any_set_t a;
		any_set_t b;
		REQUIRE(difference_of(a, b).empty());
		REQUIRE(difference_of(a, b, any_set_t{}, any_set_t{}, any_set_t{}).empty());
		b.insert("asdf"s, 1, 2, 100.0);
		REQUIRE(difference_of(a, b).empty());
	}


	SECTION("difference_of() only throws CopyConstructionErrors when copying types without a copy constructor") {
		any_set_t a(std::make_tuple(UniqueInt::make(1), 2, 3, 4));
		any_set_t b;
		REQUIRE_THROWS_AS(difference_of(a, b), const te::NoCopyConstructorError<UniqueInt>&);
		REQUIRE_THROWS_AS(difference_of(a, b), const te::CopyConstructionError&);
		REQUIRE_NOTHROW(difference_of(std::move(a), b));
	}

}




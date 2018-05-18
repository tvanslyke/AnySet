#include "../any-set.h"
#include "anyset/SetOperations.h"
#include <algorithm>

TEST_CASE("UnionOf", "[union-of]") {

	using namespace te;
	using namespace std::literals;

	auto is_perm = [](const auto& a, std::initializer_list<int> b) {
		return std::is_permutation(begin(a), end(a), begin(b), end(b));
	};

	SECTION("union_of() computes the union of two or more sets") {
		{
			// 2 sets, no overlap
			any_set_t a({0, 1, 2, 3});
			any_set_t b({4, 5, 6});
			any_set_t result = union_of(a, b);
			REQUIRE(is_perm(result, {0, 1, 2, 3, 4, 5, 6}));
			// 'a' and 'b' should be unchanged
			REQUIRE(is_perm(a, {0, 1, 2, 3}));
			REQUIRE(is_perm(b, {4, 5, 6}));
			result._assert_invariants();
			a._assert_invariants();
			b._assert_invariants();

			result.clear();

			// move from 'a' and 'b'
			result = union_of(std::move(a), std::move(b));
			REQUIRE(is_perm(result, {0, 1, 2, 3, 4, 5, 6}));
			// 'a' and 'b' should be moved-from
			REQUIRE(a.empty());
			REQUIRE(b.empty());
			result._assert_invariants();
			a._assert_invariants();
			b._assert_invariants();
		}
		{
			// 3 sets, no overlap
			any_set_t a({0, 1, 2, 3});
			any_set_t b({4, 5, 6});
			any_set_t c({7, 8, 9, 10, 11});
			any_set_t result = union_of(a, b, c);
			REQUIRE(is_perm(result, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}));
			// 'a', 'b', and 'c' should be unchanged
			REQUIRE(is_perm(a, {0, 1, 2, 3}));
			REQUIRE(is_perm(b, {4, 5, 6}));
			REQUIRE(is_perm(c, {7, 8, 9, 10, 11}));
			result._assert_invariants();
			a._assert_invariants();
			b._assert_invariants();
			c._assert_invariants();

			result.clear();

			// move from 'a', 'b', and 'c'
			result = union_of(std::move(a), std::move(b), std::move(c));
			REQUIRE(is_perm(result, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}));
			// 'a', 'b', and 'c' should be moved-from
			REQUIRE(a.empty());
			REQUIRE(b.empty());
			REQUIRE(c.empty());
			result._assert_invariants();
			a._assert_invariants();
			b._assert_invariants();
			c._assert_invariants();
		}
	}

	SECTION("operator+ is equivalent to union_of()") {
		{
			// 2 sets, no overlap
			any_set_t a({0, 1, 2, 3});
			any_set_t b({4, 5, 6});
			any_set_t result = a + b;
			REQUIRE(is_perm(result, {0, 1, 2, 3, 4, 5, 6}));
			// 'a' and 'b' should be unchanged
			REQUIRE(is_perm(a, {0, 1, 2, 3}));
			REQUIRE(is_perm(b, {4, 5, 6}));
			result._assert_invariants();
			a._assert_invariants();
			b._assert_invariants();

			result.clear();

			// move from 'a' and 'b'
			result = std::move(a) + std::move(b);
			REQUIRE(is_perm(result, {0, 1, 2, 3, 4, 5, 6}));
			// 'a' and 'b' should be moved-from
			REQUIRE(a.empty());
			REQUIRE(b.empty());
			result._assert_invariants();
			a._assert_invariants();
			b._assert_invariants();
		}
		{
			// 2 sets, no overlap, operator +=
			any_set_t a({0, 1, 2, 3});
			any_set_t b({4, 5, 6});
			any_set_t result;
			result += a;
			result += b;
			REQUIRE(is_perm(result, {0, 1, 2, 3, 4, 5, 6}));
			// 'a' and 'b' should be unchanged
			REQUIRE(is_perm(a, {0, 1, 2, 3}));
			REQUIRE(is_perm(b, {4, 5, 6}));
			result._assert_invariants();
			a._assert_invariants();
			b._assert_invariants();

			result.clear();

			// move from 'a' and 'b'
			result += std::move(a);
			result += std::move(b);
			REQUIRE(is_perm(result, {0, 1, 2, 3, 4, 5, 6}));
			// 'a' and 'b' should be moved-from
			REQUIRE(a.empty());
			REQUIRE(b.empty());
			result._assert_invariants();
			a._assert_invariants();
			b._assert_invariants();
		}
		{
			// 3 sets, no overlap
			any_set_t a({0, 1, 2, 3});
			any_set_t b({4, 5, 6});
			any_set_t c({7, 8, 9, 10, 11});
			any_set_t result = a + b + c;
			REQUIRE(is_perm(result, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}));
			// 'a', 'b', and 'c' should be unchanged
			REQUIRE(is_perm(a, {0, 1, 2, 3}));
			REQUIRE(is_perm(b, {4, 5, 6}));
			REQUIRE(is_perm(c, {7, 8, 9, 10, 11}));
			result._assert_invariants();
			a._assert_invariants();
			b._assert_invariants();
			c._assert_invariants();

			result.clear();

			// move from 'a', 'b', and 'c'
			result = std::move(a) + std::move(b) + std::move(c);
			REQUIRE(is_perm(result, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}));
			// 'a', 'b', and 'c' should be moved-from
			REQUIRE(a.empty());
			REQUIRE(b.empty());
			REQUIRE(c.empty());
			result._assert_invariants();
			a._assert_invariants();
			b._assert_invariants();
			c._assert_invariants();
		}
	}
	
	SECTION("operator| is equivalent to union_of()") {
		{
			// 2 sets, no overlap
			any_set_t a({0, 1, 2, 3});
			any_set_t b({4, 5, 6});
			any_set_t result = a | b;
			REQUIRE(is_perm(result, {0, 1, 2, 3, 4, 5, 6}));
			// 'a' and 'b' should be unchanged
			REQUIRE(is_perm(a, {0, 1, 2, 3}));
			REQUIRE(is_perm(b, {4, 5, 6}));
			result._assert_invariants();
			a._assert_invariants();
			b._assert_invariants();

			result.clear();

			// move from 'a' and 'b'
			result = std::move(a) | std::move(b);
			REQUIRE(is_perm(result, {0, 1, 2, 3, 4, 5, 6}));
			// 'a' and 'b' should be moved-from
			REQUIRE(a.empty());
			REQUIRE(b.empty());
			result._assert_invariants();
			a._assert_invariants();
			b._assert_invariants();
		}
		{
			// 2 sets, no overlap, operator +=
			any_set_t a({0, 1, 2, 3});
			any_set_t b({4, 5, 6});
			any_set_t result;
			result |= a;
			result |= b;
			REQUIRE(is_perm(result, {0, 1, 2, 3, 4, 5, 6}));
			// 'a' and 'b' should be unchanged
			REQUIRE(is_perm(a, {0, 1, 2, 3}));
			REQUIRE(is_perm(b, {4, 5, 6}));
			result._assert_invariants();
			a._assert_invariants();
			b._assert_invariants();

			result.clear();

			// move from 'a' and 'b'
			result |= std::move(a);
			result |= std::move(b);
			REQUIRE(is_perm(result, {0, 1, 2, 3, 4, 5, 6}));
			// 'a' and 'b' should be moved-from
			REQUIRE(a.empty());
			REQUIRE(b.empty());
			result._assert_invariants();
			a._assert_invariants();
			b._assert_invariants();
		}
		{
			// 3 sets, no overlap
			any_set_t a({0, 1, 2, 3});
			any_set_t b({4, 5, 6});
			any_set_t c({7, 8, 9, 10, 11});
			any_set_t result = a | b | c;
			REQUIRE(is_perm(result, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}));
			// 'a', 'b', and 'c' should be unchanged
			REQUIRE(is_perm(a, {0, 1, 2, 3}));
			REQUIRE(is_perm(b, {4, 5, 6}));
			REQUIRE(is_perm(c, {7, 8, 9, 10, 11}));
			result._assert_invariants();
			a._assert_invariants();
			b._assert_invariants();
			c._assert_invariants();

			result.clear();

			// move from 'a', 'b', and 'c'
			result = std::move(a) | std::move(b) | std::move(c);
			REQUIRE(is_perm(result, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}));
			// 'a', 'b', and 'c' should be moved-from
			REQUIRE(a.empty());
			REQUIRE(b.empty());
			REQUIRE(c.empty());
			result._assert_invariants();
			a._assert_invariants();
			b._assert_invariants();
			c._assert_invariants();
		}
	}

	SECTION("union_of() can compute the union of overlapping sets") {
		any_set_t a(std::make_tuple(
			"some string"s, "some other string"s, 0, 1, 2, 3
		));
		any_set_t b(std::make_tuple(
			"yet another string"s, "some string"s, 2, 3, 4, 5
		));

		any_set_t result = union_of(a, b);
		REQUIRE(result.size() == 9u);
		REQUIRE(result.contains("some string"s));
		REQUIRE(result.contains("some other string"s));
		REQUIRE(result.contains("yet another string"s));
		REQUIRE(result.contains(0));
		REQUIRE(result.contains(1));
		REQUIRE(result.contains(2));
		REQUIRE(result.contains(3));
		REQUIRE(result.contains(4));
		REQUIRE(result.contains(5));
		REQUIRE(a.size() == 6);
		REQUIRE(b.size() == 6);
		result._assert_invariants();
		a._assert_invariants();
		b._assert_invariants();

		result.clear();

		// move from only 'b'
		result = union_of(a, std::move(b));
		REQUIRE(result.size() == 9u);
		REQUIRE(result.contains("some string"s));
		REQUIRE(result.contains("some other string"s));
		REQUIRE(result.contains("yet another string"s));
		REQUIRE(result.contains(0));
		REQUIRE(result.contains(1));
		REQUIRE(result.contains(2));
		REQUIRE(result.contains(3));
		REQUIRE(result.contains(4));
		REQUIRE(result.contains(5));
		REQUIRE(a.size() == 6);
		REQUIRE(b.size() == 3);
		result._assert_invariants();
		a._assert_invariants();
		b._assert_invariants();
	}


	SECTION("union_of() empty sets is an empty set") {
		any_set_t a;
		any_set_t b;
		REQUIRE(union_of(a, b).empty());
		REQUIRE(union_of(a, b, any_set_t{}, any_set_t{}, any_set_t{}).empty());
	}
}




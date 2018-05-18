#include "any-set.h"

TEST_CASE("Update", "[update]") {

	using namespace te;
	using namespace std::literals;

	auto is_perm = [](const auto& a, std::initializer_list<int> b) {
		return std::is_permutation(begin(a), end(a), begin(b), end(b));
	};

	SECTION("update() results in the union of the two sets") {
		{
			// 2 sets, no overlap
			any_set_t a({0, 1, 2, 3});
			any_set_t b({4, 5, 6});
			b.update(a);
			REQUIRE(is_perm(b, {0, 1, 2, 3, 4, 5, 6}));
			REQUIRE(is_perm(a, {0, 1, 2, 3}));
			a._assert_invariants();
			b._assert_invariants();

			b = {4, 5, 6};

			// move from 'a' and 'b'
			b.update(std::move(a));
			REQUIRE(is_perm(b, {0, 1, 2, 3, 4, 5, 6}));
			// 'a' and 'b' should be moved-from
			REQUIRE(a.empty());
			a._assert_invariants();
			b._assert_invariants();
		}
	}

	SECTION("update() works with overlapping sets") {
		any_set_t a(std::make_tuple(
			"some string"s, "some other string"s, 0, 1, 2, 3
		));
		any_set_t b(std::make_tuple(
			"yet another string"s, "some string"s, 2, 3, 4, 5
		));

		b.update(a);
		REQUIRE(b.size() == 9u);
		REQUIRE(b.contains("some string"s));
		REQUIRE(b.contains("some other string"s));
		REQUIRE(b.contains("yet another string"s));
		REQUIRE(b.contains(0));
		REQUIRE(b.contains(1));
		REQUIRE(b.contains(2));
		REQUIRE(b.contains(3));
		REQUIRE(b.contains(4));
		REQUIRE(b.contains(5));
		REQUIRE(a.size() == 6);
		a._assert_invariants();
		b._assert_invariants();

		b.clear();
		b.insert(
			"yet another string"s, "some string"s, 2, 3, 4, 5
		);

		// move from 'a'
		b.update(std::move(a));
		REQUIRE(b.size() == 9u);
		REQUIRE(b.contains("some string"s));
		REQUIRE(b.contains("some other string"s));
		REQUIRE(b.contains("yet another string"s));
		REQUIRE(b.contains(0));
		REQUIRE(b.contains(1));
		REQUIRE(b.contains(2));
		REQUIRE(b.contains(3));
		REQUIRE(b.contains(4));
		REQUIRE(b.contains(5));
		REQUIRE(a.contains("some string"s));
		REQUIRE(a.contains(2));
		REQUIRE(a.contains(3));
		REQUIRE(not a.contains("some other string"s));
		REQUIRE(not a.contains(0));
		REQUIRE(not a.contains(1));
		a._assert_invariants();
		b._assert_invariants();
	}
}




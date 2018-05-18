#include "../any-set.h"
#include "anyset/SetOperations.h"
#include <algorithm>

TEST_CASE("SubsetSuperset", "[subset-superset]") {

	using namespace te;
	using namespace std::literals;

	auto is_perm = [](const auto& a, std::initializer_list<int> b) {
		return std::is_permutation(begin(a), end(a), begin(b), end(b));
	};

	SECTION("is_subset_of() indicates whether a set is a subset of another") {
		any_set_t a;
		any_set_t b;
		REQUIRE(is_subset_of(a, b));
		REQUIRE(is_subset_of(b, a));
		a.insert(1);
		REQUIRE(is_subset_of(b, a));
		REQUIRE(not is_subset_of(a, b));
		b.insert(1);
		REQUIRE(is_subset_of(a, b));
		REQUIRE(is_subset_of(b, a));
		b.insert("abcd"s);
		REQUIRE(is_subset_of(a, b));
		REQUIRE(not is_subset_of(b, a));
		a.insert(300.5);
		REQUIRE(not is_subset_of(b, a));
		REQUIRE(not is_subset_of(a, b));
	}

	SECTION("operator<= is equivalent to is_subset_of()") {
		any_set_t a;
		any_set_t b;
		REQUIRE(a <= b);
		REQUIRE(b <= a);
		a.insert(1);
		REQUIRE(b <= a);
		REQUIRE(not (a <= b));
		b.insert(1);
		REQUIRE(a <= b);
		REQUIRE(b <= a);
		b.insert("abcd"s);
		REQUIRE(a <= b);
		REQUIRE(not (b <= a));
		a.insert(300.5);
		REQUIRE(not (b <= a));
		REQUIRE(not (a <= b));
	}

	SECTION("operator< indicates whether a set is a strict subset of another") {
		any_set_t a;
		any_set_t b;
		REQUIRE(not (a < b));
		REQUIRE(not (b < a));
		a.insert(1);
		REQUIRE(b < a);
		REQUIRE(not (a < b));
		b.insert(1);
		REQUIRE(not (a < b));
		REQUIRE(not (b < a));
		b.insert("abcd"s);
		REQUIRE(a < b);
		REQUIRE(not (b < a));
		a.insert(300.5);
		REQUIRE(not (b < a));
		REQUIRE(not (a < b));
	}

	SECTION("is_superset_of() indicates whether a set is a superset of another") {
		any_set_t a;
		any_set_t b;
		REQUIRE(is_superset_of(a, b));
		REQUIRE(is_superset_of(b, a));
		a.insert(1);
		REQUIRE(is_superset_of(a, b));
		REQUIRE(not is_superset_of(b, a));
		b.insert(1);
		REQUIRE(is_superset_of(b, a));
		REQUIRE(is_superset_of(a, b));
		b.insert("abcd"s);
		REQUIRE(is_superset_of(b, a));
		REQUIRE(not is_superset_of(a, b));
		a.insert(300.5);
		REQUIRE(not is_superset_of(b, a));
		REQUIRE(not is_superset_of(a, b));
	}

	SECTION("operator>= is equivalent to is_superset_of()") {
		any_set_t a;
		any_set_t b;
		REQUIRE((a >= b));
		REQUIRE((b >= a));
		a.insert(1);
		REQUIRE((a >= b));
		REQUIRE(not (b >= a));
		b.insert(1);
		REQUIRE((b >= a));
		REQUIRE((a >= b));
		b.insert("abcd"s);
		REQUIRE((b >= a));
		REQUIRE(not (a >= b));
		a.insert(300.5);
		REQUIRE(not (b >= a));
		REQUIRE(not (a >= b));
	}

	SECTION("operator> indicates whether a set is a strict superset of another") {
		any_set_t a;
		any_set_t b;
		REQUIRE(not (a > b));
		REQUIRE(not (b > a));
		a.insert(1);
		REQUIRE(not (b > a));
		REQUIRE((a > b));
		b.insert(1);
		REQUIRE(not (a > b));
		REQUIRE(not (b > a));
		b.insert("abcd"s);
		REQUIRE(not (a > b));
		REQUIRE((b > a));
		a.insert(300.5);
		REQUIRE(not (b > a));
		REQUIRE(not (a > b));
	}

}




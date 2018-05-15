#include "any-set.h"

TEST_CASE("assign", "[assign]") {

	using namespace te;

	SECTION("Copy construction and copy assignment imply equality.") {
		{
			any_set_t set(string_names.begin(), string_names.end());
			any_set_t a(set);
			any_set_t b({1, 2, 3, 4});
			b = set;
			REQUIRE(set == a);
			REQUIRE(not (set != a));
			REQUIRE(set == b);
			REQUIRE(not (set != b));
			REQUIRE(a == b);
			REQUIRE(not (a != b));
			set._assert_invariants();
			a._assert_invariants();
			b._assert_invariants();
		}
		{
			any_set_t set(string_names.begin(), string_names.end());
			set.insert(1);
			any_set_t other({1, 2, 3, 4, 5});
			other = set;
			REQUIRE(set == other);
			REQUIRE(not (set != other));
			set._assert_invariants();
			other._assert_invariants();
		}
	}

	SECTION("Move assignment leaves the other set empty.") {
		any_set_t set(string_names.begin(), string_names.end());
		any_set_t cpy(set);
		any_set_t moved({1, 2, 3});
		moved = std::move(set);
		REQUIRE(set.empty());
		REQUIRE(cpy == moved);
		set._assert_invariants();
		cpy._assert_invariants();
		moved._assert_invariants();
	}
	
	SECTION("Move assignment does not throw a te::CopyConstructionError.") {
		any_set_t set(string_names.begin(), string_names.end());
		any_set_t moved({1, 2, 3});
		set.insert(UniqueInt::make(1));
		moved = std::move(set);
	}

	SECTION("Copy assignment throws a te::CopyConstructionError if copying a non-copyable type.") {
		any_set_t set(string_names.begin(), string_names.end());
		any_set_t moved({1, 2, 3});
		set.insert(UniqueInt::make(1));
		REQUIRE_THROWS_AS(moved = set, const te::NoCopyConstructorError<UniqueInt>&);
		REQUIRE_THROWS_AS(moved = set, const te::CopyConstructionError&);
	}


}

#include "any-set.h"

TEST_CASE("Load Factor", "[load-factor]") {

	using namespace te;
	using namespace std::literals;

	auto compute_load_factor = [](const any_set_t& set) -> float {
		return static_cast<double>(set.size()) / set.bucket_count();
	};

	auto load_factor_satisfied = [=](const any_set_t& set) -> bool {
		return compute_load_factor(set) <= set.max_load_factor();
	};

	SECTION("Load factor is always satisfied after construction") {
		{
			any_set_t set;
			REQUIRE(load_factor_satisfied(set));
		}
		{
			any_set_t set(15);
			REQUIRE(load_factor_satisfied(set));
		}
		{
			any_set_t set(16);
			REQUIRE(load_factor_satisfied(set));
		}
		{
			any_set_t set(17);
			REQUIRE(load_factor_satisfied(set));
		}
		{
			any_set_t set({1, 2, 3, 4, 5, 6, 7, 8}, 4);
			REQUIRE(load_factor_satisfied(set));
		}
		{
			any_set_t set({1, 2, 3, 4, 5, 6, 7, 8}, 16);
			REQUIRE(load_factor_satisfied(set));
		}
		{
			any_set_t set({1, 2, 3, 4, 5, 6, 7, 8}, 8);
			REQUIRE(load_factor_satisfied(set));
		}
		{
			std::array<int, 16> a;
			std::iota(begin(a), end(a), 0);
			any_set_t set(begin(a), end(a), 3);
			REQUIRE(load_factor_satisfied(set));
		}
		{
			any_set_t set(std::make_tuple(1, 2, 3, 4, 5, 6, 7, 8), 6);
			REQUIRE(load_factor_satisfied(set));
		}
		{
			any_set_t set(std::make_tuple(1, 2, 3, 4, 5, 6, 7, 8), 8);
			REQUIRE(load_factor_satisfied(set));
		}
		{
			any_set_t set(std::make_tuple(1, 2, 3, 4, 5, 6, 7, 8), 12);
			REQUIRE(load_factor_satisfied(set));
		}
	}

	SECTION("Load factor can only be above the maximum after a call to max_load_factor(float)") {
		any_set_t set{1, 2, 3, 4, 5, 6, 7};
		set.rehash(8);
		REQUIRE(set.bucket_count() == 8);
		REQUIRE(load_factor_satisfied(set));
		set.max_load_factor(0.5);
		REQUIRE(not load_factor_satisfied(set));
		set.rehash(0);
		REQUIRE(load_factor_satisfied(set));
	}

	SECTION("Inserting new elements does not break the load factor") {
		any_set_t set;
		REQUIRE(load_factor_satisfied(set));
		set.rehash(4);
		REQUIRE(load_factor_satisfied(set));
		for(std::size_t i = 0; i < 64; ++i)
		{
			set.insert(i);
			REQUIRE(load_factor_satisfied(set));
		}
	}
}

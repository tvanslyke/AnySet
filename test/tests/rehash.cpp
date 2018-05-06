#include "any-set.h"

TEST_CASE("Rehash", "[rehash]") {

	using namespace te;
	using namespace std::literals;

	auto compute_load_factor = [](const any_set_t& set) -> float {
		return static_cast<double>(set.size()) / set.bucket_count();
	};

	auto load_factor_satisfied = [=](const any_set_t& set) -> bool {
		return compute_load_factor(set) <= set.max_load_factor();
	};

	SECTION("Load factor is always satisfied after a rehash") {
		any_set_t set{1, 2, 3, 4, 5, 6, 7};
		set.rehash(8);
		REQUIRE(set.bucket_count() >= 8u);
		REQUIRE(load_factor_satisfied(set));
		set.max_load_factor(0.5);
		REQUIRE(not load_factor_satisfied(set));
		std::cout << "BEFORE: " << set.size() << ", " << set.bucket_count() << std::endl;
		set.rehash(0);
		REQUIRE(set.bucket_count() >= 0u);
		std::cout << "AFTER: " << set.size() << ", " << set.bucket_count() << std::endl;
		REQUIRE(load_factor_satisfied(set));
		set.rehash(16);
		REQUIRE(set.bucket_count() >= 16u);
		REQUIRE(load_factor_satisfied(set));
		set.max_load_factor(0.1);
		REQUIRE(not load_factor_satisfied(set));
		set.rehash(0);
		REQUIRE(set.bucket_count() >= 0u);
		REQUIRE(load_factor_satisfied(set));
	}
}

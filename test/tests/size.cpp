#include "any-set.h"
#include <numeric>

TEST_CASE("Size", "[size]") {

	using namespace te;
	using namespace std::literals;
	SECTION("Default-constructed set has size 0") {
		any_set_t set;
		REQUIRE(set.size() == 0u);
	}
	
	SECTION("Set contructed with N unique elements has size N") {
		{
			std::array<int, 3> init{0, 1, 2};
			any_set_t set(begin(init), end(init));
			REQUIRE(set.size() == init.size());
		}
		{
			std::array<int, 4> init{0, 1, 2, 3};
			any_set_t set(begin(init), end(init));
			REQUIRE(set.size() == init.size());
		}
		{
			std::array<int, 5> init{0, 1, 2, 3, 4};
			any_set_t set(begin(init), end(init));
			REQUIRE(set.size() == init.size());
		}
		{
			any_set_t set({1, 2, 3, 4, 1, 2, 3, 4});
			REQUIRE(set.size() == 4);
		}
		{
			any_set_t set(std::make_tuple(1, "string"s, 3.1415));
			REQUIRE(set.size() == 3);
		}
	}
	
	SECTION("Inserting a new elements increases the size only when the elements did not already exist in the set") {
		any_set_t set;
		for(std::size_t i = 0; i < 30; ++i)
		{
			auto old_size = set.size();
			auto [_, success] = set.insert(i);
			REQUIRE(success);
			REQUIRE(old_size + 1 == set.size());
		}
		
		for(std::size_t i = 0; i < 30; ++i)
		{
			auto old_size = set.size();
			auto [_, success] = set.insert(i);
			REQUIRE(not success);
			REQUIRE(old_size == set.size());
		}
		
		for(std::size_t i = 30; i < 70; ++i)
		{
			auto old_size = set.size();
			auto [_, success] = set.insert(i);
			REQUIRE(success);
			REQUIRE(old_size + 1 == set.size());
		}
		auto old_size = set.size();
		set.insert({"asdf"s, "qwer"s, "zxcv"s});
		REQUIRE(set.size() == old_size + 3);
	}
}

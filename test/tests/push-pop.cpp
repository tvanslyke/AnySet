#include "any-set.h"

TEST_CASE("PushPop", "[push-pop]") {

	using namespace te;
	using namespace std::literals;

	SECTION("Popping a node reduces the set's size() by 1") {
		any_set_t set({0, 1, 2, 3, 4, 5, 6, 7, 8, 9});
		while(not set.empty())
		{
			auto old_size = set.size();
			auto handle = set.pop(set.begin()).first;
			REQUIRE(static_cast<bool>(handle));
			REQUIRE(set.size() + 1ull == old_size);
		}
	}

	SECTION("Popping a node removes the contained value from the set") {
		any_set_t set({0, 1, 2, 3, 4, 5, 6, 7, 8, 9});
		for(int i: {4, 2, 6, 3, 9, 5, 0, 7, 1, 8})
		{
			REQUIRE(set.contains(i));
			set.pop(set.find(i));
			REQUIRE(not set.contains(i));
		}
	}

	SECTION("Pushing a node adds the contained value to the set") {
		any_set_t set({0, 1, 2, 3, 4, 5, 6, 7, 8, 9});
		any_set_t other;
		for(int i: {4, 2, 6, 3, 9, 5, 0, 7, 1, 8})
		{
			REQUIRE(set.contains(i));
			REQUIRE(not other.contains(i));
			other.push(set.pop(set.find(i)).first);
			REQUIRE(not set.contains(i));
			REQUIRE(other.contains(i));
		}
		REQUIRE(set.empty());
		REQUIRE(other.size() == 10u);
	}

	SECTION("Push and pop are inverse operations") {
		any_set_t set({0, 1, 2, 3, 4, 5, 6, 7, 8, 9});
		for(auto pos = set.begin(); pos != set.end(); ++pos)
		{
			auto cpy = set;
			REQUIRE(not static_cast<bool>(set.push(set.pop(pos).first).second));
			REQUIRE(cpy == set);
		}
	}

}

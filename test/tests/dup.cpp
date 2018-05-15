#include "any-set.h"

TEST_CASE("Dup", "[dup]") {

	using namespace te;
	using namespace std::literals;

	SECTION("Duplicated nodes are simply copies") {
		any_set_t set({1, 2, 3, 4, 5});
		for(auto pos = set.begin(); pos != set.end(); ++pos)
		{
			auto handle = set.dup(pos);
			REQUIRE(*handle == *pos);
			REQUIRE(static_cast<bool>(set.push(std::move(handle)).second));
		}
		REQUIRE(*set.dup(set.insert("abcd"s).first) == "abcd"s);
	}
	SECTION("Duplicated nodes with non-copyable types throws a te::CopyConstructionError") {
		any_set_t set({1, 2, 3, 4, 5});
		auto [pos, inserted] = set.insert(UniqueInt::make(6));
		REQUIRE(inserted);
		REQUIRE(set.contains(UniqueInt::make(6)));
		REQUIRE_THROWS_AS(
			set.dup(set.find(UniqueInt::make(6))),
			const te::CopyConstructionError&
		);
	}
}

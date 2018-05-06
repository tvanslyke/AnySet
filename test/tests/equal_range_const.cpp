#include "any-set.h"


TEST_CASE("Equal Range Const", "[equal_range_const]") {

	using namespace te;
	using namespace std::literals;
	auto len = [](auto iter_pair) {
		return static_cast<std::size_t>(
			std::distance(iter_pair.first, iter_pair.second)
		);
	};
	SECTION("An empty set always returns an empty equal_range()") {
		any_set_t set;
		REQUIRE(len(std::as_const(set).equal_range("asdf"s)) == 0u);
		REQUIRE(len(std::as_const(set).equal_range(1)) == 0u);
		REQUIRE(len(std::as_const(set).equal_range(1ull)) == 0u);
	}

	SECTION("An equal range of an inserted element has length 1") {
		any_set_t set;
		set.insert("asdf"s);
		REQUIRE(len(std::as_const(set).equal_range("asdf"s)) == 1u);
		REQUIRE(*std::as_const(set).equal_range("asdf"s).first == "asdf"s);
		set.insert(1);
		REQUIRE(len(std::as_const(set).equal_range(1)) == 1u);
		REQUIRE(*std::as_const(set).equal_range(1).first == 1);
		set.insert(1ull);
		REQUIRE(len(std::as_const(set).equal_range(1ull)) == 1u);
		REQUIRE(*std::as_const(set).equal_range(1ull).first == 1ull);
	}

	SECTION("An equal range of an erased element has length 0") {
		any_set_t set;
		set.insert("asdf"s);
		set.insert(1);
		set.erase("asdf"s);
		set.insert(1ull);
		REQUIRE(len(std::as_const(set).equal_range("asdf"s)) == 0u);
		set.erase(1);
		REQUIRE(len(std::as_const(set).equal_range(1)) == 0u);
		set.erase(1ull);
		REQUIRE(len(std::as_const(set).equal_range(1ull)) == 0u);
	}

	SECTION("An equal range of values not inserted has length 0") {
		any_set_t set;
		set.rehash(8);
		for(std::size_t i = 0; i < 8u; ++i)
			set.insert(i);
		for(std::size_t i = 0; i < 8u; ++i)
		{
			REQUIRE(len(std::as_const(set).equal_range(i)) == 1u);
			REQUIRE(*std::as_const(set).equal_range(i).first == i);
		}
		for(std::size_t i = 8u; i < 16u; ++i)
		{
			REQUIRE(len(std::as_const(set).equal_range(i)) == 0u);
		}
	}
}

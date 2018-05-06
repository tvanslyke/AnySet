#include <iostream>
#include <utility>
#include "extra-hash.h"
#include "any-set.h"

using pair_t = std::pair<int, double>;

TEST_CASE("Insert", "[insert]") {

	using namespace te;
	using namespace std::literals;
	SECTION("Lvalues can be inserted") {
		any_set_t set;

		const int a = 1;
		const double b = 1;
		const std::string c = "some string"s;

		auto [pos, success] = set.insert(a);
		REQUIRE(success);
		REQUIRE(set.size() == 1u);
		REQUIRE(*pos == a);
		REQUIRE(set.count(a) == 1u);
		std::tie(pos, success) = set.insert(b);
		REQUIRE(success);
		REQUIRE(set.size() == 2u);
		REQUIRE(*pos == b);
		REQUIRE(set.count(b) == 1u);
		std::tie(pos, success) = set.insert(c);
		REQUIRE(success);
		REQUIRE(set.size() == 3u);
		REQUIRE(*pos == c);
		REQUIRE(set.count(c) == 1u);

		std::tie(pos, success) = set.insert(a);
		REQUIRE(not success);
		REQUIRE(set.size() == 3u);
		std::tie(pos, success) = set.insert(b);
		REQUIRE(not success);
		REQUIRE(set.size() == 3u);
		std::tie(pos, success) = set.insert(c);
		REQUIRE(not success);
		REQUIRE(set.size() == 3u);
	}
	
	SECTION("Lvalues can be inserted with an iterator hint") {
		any_set_t set;

		const int a = 1;
		const double b = 1;
		const std::string c = "some string"s;

		auto [pos, success] = set.insert(set.begin(), a);
		REQUIRE(success);
		REQUIRE(set.size() == 1u);
		REQUIRE(*pos == a);
		REQUIRE(set.count(a) == 1u);
		std::tie(pos, success) = set.insert(pos, b);
		REQUIRE(success);
		REQUIRE(set.size() == 2u);
		REQUIRE(*pos == b);
		REQUIRE(set.count(b) == 1u);
		std::tie(pos, success) = set.insert(pos, c);
		REQUIRE(success);
		REQUIRE(set.size() == 3u);
		REQUIRE(*pos == c);
		REQUIRE(set.count(c) == 1u);

		std::tie(pos, success) = set.insert(a);
		REQUIRE(not success);
		REQUIRE(set.size() == 3u);
		std::tie(pos, success) = set.insert(b);
		REQUIRE(not success);
		REQUIRE(set.size() == 3u);
		std::tie(pos, success) = set.insert(c);
		REQUIRE(not success);
		REQUIRE(set.size() == 3u);
	}
	
	SECTION("Rvalues can be inserted") {
		any_set_t set;

		auto [pos, success] = set.insert(int(1));
		REQUIRE(success);
		REQUIRE(set.size() == 1u);
		REQUIRE(*pos == int(1));
		REQUIRE(set.count(int(1)) == 1u);
		std::tie(pos, success) = set.insert(double(1));
		REQUIRE(success);
		REQUIRE(set.size() == 2u);
		REQUIRE(*pos == double(1));
		REQUIRE(set.count(double(1)) == 1u);
		std::tie(pos, success) = set.insert("some string"s);
		REQUIRE(success);
		REQUIRE(set.size() == 3u);
		REQUIRE(*pos == "some string"s);
		REQUIRE(set.count("some string"s) == 1u);

		std::tie(pos, success) = set.insert(int(1));
		REQUIRE(not success);
		REQUIRE(set.size() == 3u);
		std::tie(pos, success) = set.insert(double(1));
		REQUIRE(not success);
		REQUIRE(set.size() == 3u);
		std::tie(pos, success) = set.insert("some string"s);
		REQUIRE(not success);
		REQUIRE(set.size() == 3u);
	}
	
	SECTION("Rvalues can be inserted with an iterator hint") {
		any_set_t set;

		auto [pos, success] = set.insert(set.begin(), int(1));
		REQUIRE(success);
		REQUIRE(set.size() == 1u);
		REQUIRE(*pos == int(1));
		REQUIRE(set.count(int(1)) == 1u);
		std::tie(pos, success) = set.insert(pos, double(1));
		REQUIRE(success);
		REQUIRE(set.size() == 2u);
		REQUIRE(*pos == double(1));
		REQUIRE(set.count(double(1)) == 1u);
		std::tie(pos, success) = set.insert(pos, "some string"s);
		REQUIRE(success);
		REQUIRE(set.size() == 3u);
		REQUIRE(*pos == "some string"s);
		REQUIRE(set.count("some string"s) == 1u);

		std::tie(pos, success) = set.insert(int(1));
		REQUIRE(not success);
		REQUIRE(set.size() == 3u);
		std::tie(pos, success) = set.insert(double(1));
		REQUIRE(not success);
		REQUIRE(set.size() == 3u);
		std::tie(pos, success) = set.insert("some string"s);
		REQUIRE(not success);
		REQUIRE(set.size() == 3u);
	}
	
	SECTION("Range, initializer list, and vararg insertion are all equivalent to one-by-one insertion") {
		any_set_t set1(string_names.begin(), string_names.end());
		any_set_t set2;
		any_set_t set3;
		set2.insert(string_names.begin(), string_names.end());
		for(const auto& name: string_names)
			set3.insert(name);
		REQUIRE(set1 == set2);
		REQUIRE(set1 == set3);
		REQUIRE(set2 == set3);
		REQUIRE(set1.size() == string_names.size());
		REQUIRE(set2.size() == string_names.size());
		REQUIRE(set3.size() == string_names.size());
		set1.insert({1, 2, 3, 4});
		REQUIRE(set1.size() == (string_names.size() + 4u));
		set2.insert(1, 2, 3, 4);
		REQUIRE(set2.size() == (string_names.size() + 4u));
		std::array<int, 4> tmp{1, 2, 3, 4};
		set3.insert(tmp.begin(), tmp.end());
		
		REQUIRE(set1 == set2);
		REQUIRE(set1 == set3);
		REQUIRE(set2 == set3);
		REQUIRE(set1.size() == string_names.size() + 4);
		REQUIRE(set2.size() == string_names.size() + 4);
		REQUIRE(set3.size() == string_names.size() + 4);
		for(const auto& name: string_names)
		{
			REQUIRE(set1.count(name) == 1u);
			REQUIRE(set2.count(name) == 1u);
			REQUIRE(set3.count(name) == 1u);
		}
		
		for(int n: {1, 2, 3, 4})
		{
			REQUIRE(set1.count(n) == 1u);
			REQUIRE(set2.count(n) == 1u);
			REQUIRE(set3.count(n) == 1u);
		}
		
		for(int n: {5, 6, 7})
		{
			REQUIRE(set1.count(n) == 0u);
			REQUIRE(set2.count(n) == 0u);
			REQUIRE(set3.count(n) == 0u);
		}
		
	}
}

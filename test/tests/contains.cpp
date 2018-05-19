#include "any-set.h"

struct SizeEquals {
	template <class T, class U>
	bool operator()(const T& l, const U& r) const
	{
		return l.size() == r.size();
	}
};

struct SizeHash
{
	template <class T>
	std::size_t operator()(const T& v) const
	{ return te::hash_value(v.size()); }
};

TEST_CASE("Contains", "[contains]") {

	using namespace te;
	using namespace std::literals;

	SECTION("An empty set always returns contains() == false") {
		any_set_t set;
		any_set_t other(std::make_tuple("asdf"s, 1, 1ull));
		REQUIRE(not set.contains("asdf"s));
		REQUIRE(not set.contains(1));
		REQUIRE(not set.contains(1ull));
		
		REQUIRE(not set.contains_eq("asdf"s));
		REQUIRE(not set.contains_eq(1));
		REQUIRE(not set.contains_eq(1ull));
		
		REQUIRE(not set.contains_value(*other.find("asdf"s)));
		REQUIRE(not set.contains_value(*other.find(1)));
		REQUIRE(not set.contains_value(*other.find(1ull)));
		
		REQUIRE(not set.contains_value_eq(*other.find("asdf"s)));
		REQUIRE(not set.contains_value_eq(*other.find(1)));
		REQUIRE(not set.contains_value_eq(*other.find(1ull)));
	}

	SECTION("Sets contain inserted elements") {
		any_set_t set;
		set.insert("asdf"s);
		REQUIRE(set.contains("asdf"s));
		REQUIRE(set.contains_eq("asdf"s));
		set.insert(1);
		REQUIRE(set.contains(1));
		REQUIRE(set.contains_eq(1));
		set.insert(1ull);
		REQUIRE(set.contains(1ull));
		REQUIRE(set.contains_eq("asdf"s));
	}

	SECTION("Sets do not contain erased elements") {
		any_set_t set;
		set.insert("asdf"s);
		set.insert(1);
		set.erase("asdf"s);
		set.insert(1ull);
		REQUIRE(not set.contains("asdf"s));
		REQUIRE(not set.contains_eq("asdf"s));
		set.erase(1);
		REQUIRE(not set.contains(1));
		REQUIRE(not set.contains_eq(1));
		set.erase(1ull);
		REQUIRE(not set.contains(1ull));
		REQUIRE(not set.contains_eq(1ull));
	}

	SECTION("Sets do not contain values not inserted") {
		any_set_t set;
		set.rehash(8);
		for(std::size_t i = 0; i < 8u; ++i)
			set.insert(i);
		for(std::size_t i = 0; i < 8u; ++i)
		{
			REQUIRE(set.contains(i));
			REQUIRE(set.contains_eq(i));
		}
		for(std::size_t i = 8u; i < 16u; ++i)
		{
			REQUIRE(not set.contains(i));
			REQUIRE(not set.contains_eq(i));
		}
	}

	SECTION("*_eq functions always use operator==") {
		te::AnySet<SizeHash, SizeEquals> set({
			"a"s,
			"ab"s,
			"abc"s,
			"abcd"s,
			"abcde"s
		});
		te::AnySet<SizeHash, SizeEquals> other({
			"1"s,
			"12"s,
			"123"s,
			"1234"s,
			"12345"s
		});
		std::array<std::string, 5> a{
			"a"s,
			"ab"s,
			"abc"s,
			"abcd"s,
			"abcde"s
		};
		std::array<std::string, 5> b{
			"1"s,
			"12"s,
			"123"s,
			"1234"s,
			"12345"s
		};

		for(const auto& str: a)
		{
			REQUIRE(set.contains(str));
			REQUIRE(set.contains_eq(str));
			REQUIRE(other.contains(str));
			REQUIRE(not other.contains_eq(str));
		}
		for(const auto& str: b)
		{
			REQUIRE(set.contains(str));
			REQUIRE(not set.contains_eq(str));
			REQUIRE(other.contains(str));
			REQUIRE(other.contains_eq(str));
		}
		for(const auto& v: set)
		{
			REQUIRE(set.contains_value(v));
			REQUIRE(set.contains_value_eq(v));
			REQUIRE(other.contains_value(v));
			REQUIRE(not other.contains_value_eq(v));
		}
		for(const auto& v: other)
		{
			REQUIRE(set.contains_value(v));
			REQUIRE(not set.contains_value_eq(v));
			REQUIRE(other.contains_value(v));
			REQUIRE(other.contains_value_eq(v));
		}
	}
}

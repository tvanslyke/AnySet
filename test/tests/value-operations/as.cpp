#include "./../any-set.h"
#include <functional>


namespace {
struct MyString:
	public std::string
{
	using std::string::string;
};

bool operator==(const MyString& l, const MyString& r)
{
	return static_cast<const std::string&>(l) == static_cast<const std::string&>(r);
}

struct Frankenstein:
	public MyString,
	private std::less<>,
	public std::bitset<8>
{
	using MyString::MyString;

};

// overload operator<<() to make catch2 happy
std::ostream& operator<<(std::ostream& os, const Frankenstein& f)
{
	return os << static_cast<const MyString&>(f);
}

bool operator==(const Frankenstein& l, const Frankenstein& r)
{
	return static_cast<const MyString&>(l) == static_cast<const MyString&>(r);
}


struct FinalFrankenstein final:
	public Frankenstein
{
	using Frankenstein::Frankenstein;
};


} /* namespace */

template <>
struct te::Hash<MyString>: public te::Hash<std::string> { };

template <>
struct te::Hash<Frankenstein>: public te::Hash<MyString> { };

template <>
struct te::Hash<FinalFrankenstein>: public te::Hash<Frankenstein> { };


TEST_CASE("As", "[as]") {

	using namespace te;


	SECTION("as() and try_as() are equivalent to trying an exact_cast() followed by a polymorphic_cast") {
		any_set_t set;
		set.emplace<MyString>();
		set.emplace<Frankenstein>();
		set.emplace<FinalFrankenstein>();
		
		{
			auto pos = set.find(MyString());
			REQUIRE(pos != set.end());

			REQUIRE_NOTHROW(as<MyString>(*pos));
			REQUIRE(static_cast<bool>(try_as<MyString>(*pos)));
			REQUIRE(as<MyString>(*pos) == MyString());
			REQUIRE(&as<MyString>(*pos) == try_as<MyString>(*pos));

			REQUIRE_NOTHROW(as<std::string>(*pos));
			REQUIRE(static_cast<bool>(try_as<std::string>(*pos)));
			REQUIRE(as<std::string>(*pos) == std::string());
			REQUIRE(&as<std::string>(*pos) == try_as<std::string>(*pos));

			REQUIRE_THROWS_AS(as<std::vector<bool>>(*pos), const std::bad_cast&);
			REQUIRE(not try_as<std::vector<bool>>(*pos));
		}

		{
			auto pos = set.find(Frankenstein());
			REQUIRE(pos != set.end());

			REQUIRE_NOTHROW(as<MyString>(*pos));
			REQUIRE(static_cast<bool>(try_as<MyString>(*pos)));
			REQUIRE(as<MyString>(*pos) == MyString());
			REQUIRE(&as<MyString>(*pos) == try_as<MyString>(*pos));

			REQUIRE_NOTHROW(as<std::string>(*pos));
			REQUIRE(static_cast<bool>(try_as<std::string>(*pos)));
			REQUIRE(as<std::string>(*pos) == std::string());
			REQUIRE(&as<std::string>(*pos) == try_as<std::string>(*pos));

			REQUIRE_NOTHROW(as<Frankenstein>(*pos));
			REQUIRE(static_cast<bool>(try_as<Frankenstein>(*pos)));
			REQUIRE(as<Frankenstein>(*pos) == Frankenstein());
			REQUIRE(&as<Frankenstein>(*pos) == try_as<Frankenstein>(*pos));

			REQUIRE_NOTHROW(as<std::bitset<8>>(*pos));
			REQUIRE(static_cast<bool>(try_as<std::bitset<8>>(*pos)));
			REQUIRE(as<std::bitset<8>>(*pos) == std::bitset<8>());
			REQUIRE(&as<std::bitset<8>>(*pos) == try_as<std::bitset<8>>(*pos));

			REQUIRE_THROWS_AS(as<std::less<>>(*pos), const std::bad_cast&);
			REQUIRE(not try_as<std::less<>>(*pos));

			REQUIRE_THROWS_AS(as<std::vector<bool>>(*pos), const std::bad_cast&);
			REQUIRE(not try_as<std::vector<bool>>(*pos));

		}

		
		{
			auto pos = set.find(FinalFrankenstein());
			REQUIRE(pos != set.end());

			REQUIRE_THROWS_AS(as<Frankenstein>(*pos), const std::bad_cast&);
			REQUIRE(not try_as<Frankenstein>(*pos));

			REQUIRE_THROWS_AS(as<MyString>(*pos), const std::bad_cast&);
			REQUIRE(not try_as<MyString>(*pos));

			REQUIRE_THROWS_AS(as<std::string>(*pos), const std::bad_cast&);
			REQUIRE(not try_as<std::string>(*pos));

			REQUIRE_THROWS_AS(as<std::bitset<8>>(*pos), const std::bad_cast&);
			REQUIRE(not try_as<std::bitset<8>>(*pos));

			REQUIRE_THROWS_AS(as<std::less<>>(*pos), const std::bad_cast&);
			REQUIRE(not try_as<std::less<>>(*pos));

			REQUIRE_THROWS_AS(as<std::vector<bool>>(*pos), const std::bad_cast&);
			REQUIRE(not try_as<std::vector<bool>>(*pos));

		}


	}


}

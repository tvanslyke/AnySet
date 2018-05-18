#include "./../any-set.h"
#include <functional>

struct MyString:
	public std::string
{
	using std::string::string;
};

bool operator==(const MyString& l, const MyString& r)
{
	return static_cast<const std::string&>(l) == static_cast<const std::string&>(r);
}

template <>
struct te::Hash<MyString>: public te::Hash<std::string> { };

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


template <>
struct te::Hash<Frankenstein>: public te::Hash<MyString> { };

struct FinalFrankenstein final:
	public Frankenstein
{
	using Frankenstein::Frankenstein;
};


template <>
struct te::Hash<FinalFrankenstein>: public te::Hash<Frankenstein> { };


TEST_CASE("PolymorphicCast", "[polymorphic-cast]") {

	using namespace te;


	SECTION("polymorphic_cast() can only cast instances non-final class types") {
		any_set_t set;
		set.emplace<MyString>();
		set.emplace<Frankenstein>();
		set.emplace<FinalFrankenstein>();
		
		{
			auto pos = set.find(MyString());
			REQUIRE(pos != set.end());

			REQUIRE_NOTHROW(polymorphic_cast<const MyString&>(*pos));
			REQUIRE(static_cast<bool>(polymorphic_cast<const MyString*>(&(*pos))));
			REQUIRE(polymorphic_cast<const MyString&>(*pos) == MyString());
			REQUIRE(&polymorphic_cast<const MyString&>(*pos) == polymorphic_cast<const MyString*>(&(*pos)));

			REQUIRE_NOTHROW(polymorphic_cast<const std::string&>(*pos));
			REQUIRE(static_cast<bool>(polymorphic_cast<const std::string*>(&(*pos))));
			REQUIRE(polymorphic_cast<const std::string&>(*pos) == std::string());
			REQUIRE(&polymorphic_cast<const std::string&>(*pos) == polymorphic_cast<const std::string*>(&(*pos)));

			REQUIRE_THROWS_AS(polymorphic_cast<const std::vector<bool>&>(*pos), const std::bad_cast&);
			REQUIRE(not polymorphic_cast<const std::vector<bool>*>(&(*pos)));
		}

		{
			auto pos = set.find(Frankenstein());
			REQUIRE(pos != set.end());

			REQUIRE_NOTHROW(polymorphic_cast<const MyString&>(*pos));
			REQUIRE(static_cast<bool>(polymorphic_cast<const MyString*>(&(*pos))));
			REQUIRE(polymorphic_cast<const MyString&>(*pos) == MyString());
			REQUIRE(&polymorphic_cast<const MyString&>(*pos) == polymorphic_cast<const MyString*>(&(*pos)));

			REQUIRE_NOTHROW(polymorphic_cast<const std::string&>(*pos));
			REQUIRE(static_cast<bool>(polymorphic_cast<const std::string*>(&(*pos))));
			REQUIRE(polymorphic_cast<const std::string&>(*pos) == std::string());
			REQUIRE(&polymorphic_cast<const std::string&>(*pos) == polymorphic_cast<const std::string*>(&(*pos)));

			REQUIRE_NOTHROW(polymorphic_cast<const Frankenstein&>(*pos));
			REQUIRE(static_cast<bool>(polymorphic_cast<const Frankenstein*>(&(*pos))));
			REQUIRE(polymorphic_cast<const Frankenstein&>(*pos) == Frankenstein());
			REQUIRE(&polymorphic_cast<const Frankenstein&>(*pos) == polymorphic_cast<const Frankenstein*>(&(*pos)));

			REQUIRE_NOTHROW(polymorphic_cast<const std::bitset<8>&>(*pos));
			REQUIRE(static_cast<bool>(polymorphic_cast<const std::bitset<8>*>(&(*pos))));
			REQUIRE(polymorphic_cast<const std::bitset<8>&>(*pos) == std::bitset<8>());
			REQUIRE(&polymorphic_cast<const std::bitset<8>&>(*pos) == polymorphic_cast<const std::bitset<8>*>(&(*pos)));

			REQUIRE_THROWS_AS(polymorphic_cast<const std::less<>&>(*pos), const std::bad_cast&);
			REQUIRE(not polymorphic_cast<const std::less<>*>(&(*pos)));

			REQUIRE_THROWS_AS(polymorphic_cast<const std::vector<bool>&>(*pos), const std::bad_cast&);
			REQUIRE(not polymorphic_cast<const std::vector<bool>*>(&(*pos)));

		}

		
		{
			auto pos = set.find(FinalFrankenstein());
			REQUIRE(pos != set.end());

			REQUIRE_THROWS_AS(polymorphic_cast<const Frankenstein&>(*pos), const std::bad_cast&);
			REQUIRE(not polymorphic_cast<const Frankenstein*>(&(*pos)));

			REQUIRE_THROWS_AS(polymorphic_cast<const MyString&>(*pos), const std::bad_cast&);
			REQUIRE(not polymorphic_cast<const MyString*>(&(*pos)));

			REQUIRE_THROWS_AS(polymorphic_cast<const std::string&>(*pos), const std::bad_cast&);
			REQUIRE(not polymorphic_cast<const std::string*>(&(*pos)));

			REQUIRE_THROWS_AS(polymorphic_cast<const std::bitset<8>&>(*pos), const std::bad_cast&);
			REQUIRE(not polymorphic_cast<const std::bitset<8>*>(&(*pos)));

			REQUIRE_THROWS_AS(polymorphic_cast<const std::less<>&>(*pos), const std::bad_cast&);
			REQUIRE(not polymorphic_cast<const std::less<>*>(&(*pos)));

			REQUIRE_THROWS_AS(polymorphic_cast<const std::vector<bool>&>(*pos), const std::bad_cast&);
			REQUIRE(not polymorphic_cast<const std::vector<bool>*>(&(*pos)));

		}


	}


}

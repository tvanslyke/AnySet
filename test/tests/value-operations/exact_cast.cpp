#include "./../any-set.h"


struct MyString:
	public std::string
{
	using std::string::string;
};

template <>
struct te::Hash<MyString>: public te::Hash<std::string> { };


TEST_CASE("ExactCast", "[exact-cast]") {

	using namespace te;
	using namespace std::literals;

	SECTION("exact_cast() can only cast to the exact type of the contained object") {
		any_set_t set;
		set.insert(1);
		set.insert(2ll);
		set.insert("string"s);
		set.insert(MyString("MyString"));
		{
			using type = int;
			type value = 1;
			REQUIRE_NOTHROW(exact_cast<const type&>(*set.find(value)));
			REQUIRE(exact_cast<const type*>(&(*set.find(value))));
			REQUIRE(&exact_cast<const type&>(*set.find(value)) == exact_cast<const type*>(&(*set.find(value))));
			REQUIRE_THROWS_AS(exact_cast<const char&>(*set.find(value)), const std::bad_cast&);
			REQUIRE(not exact_cast<const char*>(&(*set.find(value))));
			REQUIRE_THROWS_AS(exact_cast<const long&>(*set.find(value)), const std::bad_cast&);
			REQUIRE(not exact_cast<const long*>(&(*set.find(value))));
		}

		{
			using type = long long;
			type value = 2ll;
			REQUIRE_NOTHROW(exact_cast<const type&>(*set.find(value)));
			REQUIRE(exact_cast<const type*>(&(*set.find(value))));
			REQUIRE(&exact_cast<const type&>(*set.find(value)) == exact_cast<const type*>(&(*set.find(value))));
			REQUIRE_THROWS_AS(exact_cast<const char&>(*set.find(value)), const std::bad_cast&);
			REQUIRE(not exact_cast<const char*>(&(*set.find(value))));
			REQUIRE_THROWS_AS(exact_cast<const int&>(*set.find(value)), const std::bad_cast&);
			REQUIRE(not exact_cast<const int*>(&(*set.find(value))));
		}

		{
			using type = std::string;
			type value = "string"s;
			REQUIRE_NOTHROW(exact_cast<const type&>(*set.find(value)));
			REQUIRE(exact_cast<const type*>(&(*set.find(value))));
			REQUIRE(&exact_cast<const type&>(*set.find(value)) == exact_cast<const type*>(&(*set.find(value))));
			REQUIRE_THROWS_AS(exact_cast<const char&>(*set.find(value)), const std::bad_cast&);
			REQUIRE(not exact_cast<const char*>(&(*set.find(value))));
			REQUIRE_THROWS_AS(exact_cast<const MyString&>(*set.find(value)), const std::bad_cast&);
			REQUIRE(not exact_cast<const MyString*>(&(*set.find(value))));
		}
		
		{
			using type = MyString;
			type value("MyString");
			REQUIRE_NOTHROW(exact_cast<const type&>(*set.find(value)));
			REQUIRE(exact_cast<const type*>(&(*set.find(value))));
			REQUIRE(&exact_cast<const type&>(*set.find(value)) == exact_cast<const type*>(&(*set.find(value))));
			REQUIRE_THROWS_AS(exact_cast<const char&>(*set.find(value)), const std::bad_cast&);
			REQUIRE(not exact_cast<const char*>(&(*set.find(value))));
			REQUIRE_THROWS_AS(exact_cast<const std::string&>(*set.find(value)), const std::bad_cast&);
			REQUIRE(not exact_cast<const std::string*>(&(*set.find(value))));
		}
	}
}

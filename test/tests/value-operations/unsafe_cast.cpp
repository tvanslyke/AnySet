#include "./../any-set.h"


TEST_CASE("UnsafeCast", "[unsafe-cast]") {

	using namespace te;
	using namespace std::literals;

	SECTION("exact_cast() can only cast to the exact type of the contained object") {
		any_set_t set;
		set.insert(1);
		set.insert(2ll);
		set.insert("string"s);
		{
			using type = int;
			type value = 1;
			REQUIRE_NOTHROW(unsafe_cast<const type&>(*set.find(value)));
			REQUIRE(unsafe_cast<const type&>(*set.find(value)) == value);
		}

		{
			using type = long long;
			type value = 2ll;
			REQUIRE_NOTHROW(unsafe_cast<const type&>(*set.find(value)));
			REQUIRE(unsafe_cast<const type&>(*set.find(value)) == value);
		}

		{
			using type = std::string;
			type value = "string"s;
			REQUIRE_NOTHROW(unsafe_cast<const type&>(*set.find(value)));
			REQUIRE(unsafe_cast<const type&>(*set.find(value)) == value);
		}
		
	}
}

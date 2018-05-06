#include "any-set.h"


TEST_CASE("Iterators", "[iterators]") {

	using namespace te;
	SECTION("Iterating over all elements with iterators iterates over each bucket sequentially") {
		any_set_t set({0, 1, 2, 3, 4, 5, 6, 7, 8, 9});
		REQUIRE(set.size() == 10);
		REQUIRE(set.bucket_count() >= 10);
		auto pos = set.begin();
		auto last = set.end();
		REQUIRE(static_cast<std::size_t>(std::distance(pos, last)) == set.size());
		for(std::size_t i = 0; i < set.bucket_count(); ++i)
		{
			for(auto f = set.begin(i), l = set.end(i); f != l; (void)++f, ++pos)
			{
				REQUIRE(pos != last);
				REQUIRE(*f == *pos);
			}
		}
		REQUIRE(pos == last);
	}

	SECTION("Iterators only compare equal when they point to the same element") {
		any_set_t set{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
		{
			auto pos1 = set.cbegin();
			auto pos2 = set.cbegin();
			auto stop = set.cend();
			REQUIRE(pos1 == pos2);
			REQUIRE(pos1 != stop);
			while(pos1 != stop)
			{
				REQUIRE(*pos1 == *pos2);
				++pos2;
				auto tmp = std::next(pos1);
				REQUIRE(pos1 != tmp);
				REQUIRE(pos2 == tmp);
				++pos1;
				REQUIRE(pos1 == tmp);
				REQUIRE(pos1 == pos2);
			}
			REQUIRE(pos1 == stop);
			REQUIRE(pos2 == stop);
		}
		{
			auto pos1 = set.begin();
			auto pos2 = set.begin();
			auto stop = set.end();
			REQUIRE(pos1 == pos2);
			REQUIRE(pos1 != stop);
			while(pos1 != stop)
			{
				REQUIRE(*pos1 == *pos2);
				++pos2;
				auto tmp = std::next(pos1);

				REQUIRE(pos1 != tmp);
				REQUIRE(not (pos1 == tmp));
				
				REQUIRE(pos2 == tmp);
				REQUIRE(not (pos2 != tmp));
				++pos1;

				REQUIRE(pos1 == tmp);
				REQUIRE(not (pos1 != tmp));

				REQUIRE(pos1 == pos2);
				REQUIRE(not (pos1 != pos2));
			}
			REQUIRE(pos1 == stop);
			REQUIRE(not (pos1 != stop));

			REQUIRE(pos2 == stop);
			REQUIRE(not (pos2 != stop));
		}
	}

	
	SECTION("Iterators from different sets never compare equal") {
		any_set_t set1{0, 1, 2, 3, 4};
		any_set_t set2{0, 1, 2, 3, 4};
		REQUIRE(not (set1.begin() == set2.begin()));
		REQUIRE(not (set1.end() == set2.end()));
		REQUIRE(not (set1.cbegin() == set2.cbegin()));
		REQUIRE(not (set1.cend() == set2.cend()));
		REQUIRE(set1.begin() != set2.begin());
		REQUIRE(set1.end() != set2.end());
		REQUIRE(set1.cbegin() != set2.cbegin());
		REQUIRE(set1.cend() != set2.cend());
		for(auto pos1 = set1.begin(); pos1 != set1.end(); ++pos1)
		{
			for(auto pos2 = set2.begin(); pos2 != set2.end(); ++pos2)
			{
				REQUIRE(pos1 != pos2);
				REQUIRE(not (pos1 == pos2));
			}
		}
	}

}

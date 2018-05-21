#include "any-set.h"
#include <numeric>


TEST_CASE("Constructor", "[constructor]") {

	using namespace te;
	SECTION("Constructors") {
		{
			any_set_t set;
			REQUIRE(set.bucket_count() >= 1u);
			REQUIRE(set.size() == 0u);
		}
		{
			any_set_t set(8, std::allocator<value_type>{});
			REQUIRE(set.bucket_count() >= 8u);
			REQUIRE(set.size() == 0u);
		}
		{
			any_set_t set(33, AnyHash{}, std::equal_to<>{});
			REQUIRE(set.bucket_count() >= 33u);
			REQUIRE(set.size() == 0u);
		}
		{
			any_set_t set(0, AnyHash{}, std::equal_to<>{}, std::allocator<value_type>{});
			REQUIRE(set.bucket_count() >= 1u);
			REQUIRE(set.size() == 0u);
		}
		{
			any_set_t set(string_names.begin(), string_names.end());
			REQUIRE(std::is_permutation(string_names.begin(), string_names.end(), set.begin(), set.end(), std::equal_to<>{}));
			REQUIRE(set.bucket_count() >= string_names.size());
		}
		{
			any_set_t set(string_names.begin(), string_names.end(), 10, std::allocator<value_type>{});
			REQUIRE(std::is_permutation(string_names.begin(), string_names.end(), set.begin(), set.end(), std::equal_to<>{}));
			REQUIRE(set.bucket_count() >= string_names.size());
			REQUIRE(set.bucket_count() >= 10u);
		}
		{
			any_set_t set(string_names.begin(), string_names.end(), 10, AnyHash{}, std::allocator<value_type>{});
			REQUIRE(std::is_permutation(string_names.begin(), string_names.end(), set.begin(), set.end(), std::equal_to<>{}));
			REQUIRE(set.bucket_count() >= string_names.size());
			REQUIRE(set.bucket_count() >= 10u);
		}
		{
			any_set_t set(string_names.begin(), string_names.end(), 10, AnyHash{}, std::equal_to<>{}, std::allocator<value_type>{});
			REQUIRE(std::is_permutation(string_names.begin(), string_names.end(), set.begin(), set.end(), std::equal_to<>{}));
			REQUIRE(set.bucket_count() >= string_names.size());
			REQUIRE(set.bucket_count() >= 10u);
		}

		{
			std::array<int, 7> numbers{0, 1, 2, 3, 4, 5, 6};
			any_set_t set(std::forward_as_tuple(0, 1, 2, 3, 4, 5, 6));
			REQUIRE(std::is_permutation(numbers.begin(), numbers.end(), set.begin(), set.end(), std::equal_to<>{}));
			REQUIRE(set.bucket_count() >= numbers.size());
		}
		{
			std::array<int, 7> numbers{0, 1, 2, 3, 4, 5, 6};
			any_set_t set(std::forward_as_tuple(0, 1, 2, 3, 4, 5, 6), 10, std::allocator<value_type>{});
			REQUIRE(std::is_permutation(numbers.begin(), numbers.end(), set.begin(), set.end(), std::equal_to<>{}));
			REQUIRE(set.bucket_count() >= 10u);
			REQUIRE(set.bucket_count() >= numbers.size());
		}
		{
			std::array<int, 7> numbers{0, 1, 2, 3, 4, 5, 6};
			any_set_t set(std::forward_as_tuple(0, 1, 2, 3, 4, 5, 6), 20, AnyHash{}, std::allocator<value_type>{});
			REQUIRE(std::is_permutation(numbers.begin(), numbers.end(), set.begin(), set.end(), std::equal_to<>{}));
			REQUIRE(set.bucket_count() >= 20u);
			REQUIRE(set.bucket_count() >= numbers.size());
		}
		{
			std::array<int, 7> numbers{0, 1, 2, 3, 4, 5, 6};
			any_set_t set(std::forward_as_tuple(0, 1, 2, 3, 4, 5, 6), 40, AnyHash{}, std::equal_to<>{}, std::allocator<value_type>{});
			REQUIRE(std::is_permutation(numbers.begin(), numbers.end(), set.begin(), set.end(), std::equal_to<>{}));
			REQUIRE(set.bucket_count() >= 40u);
			REQUIRE(set.bucket_count() >= numbers.size());
		}


		{
			std::array<int, 7> numbers{0, 1, 2, 3, 4, 5, 6};
			any_set_t set(std::make_tuple(0, 1, 2, 3, 4, 5, 6));
			REQUIRE(std::is_permutation(numbers.begin(), numbers.end(), set.begin(), set.end(), std::equal_to<>{}));
			REQUIRE(set.bucket_count() >= numbers.size());
		}
		{
			std::array<int, 7> numbers{0, 1, 2, 3, 4, 5, 6};
			any_set_t set(std::make_tuple(0, 1, 2, 3, 4, 5, 6), 10, std::allocator<value_type>{});
			REQUIRE(std::is_permutation(numbers.begin(), numbers.end(), set.begin(), set.end(), std::equal_to<>{}));
			REQUIRE(set.bucket_count() >= 10u);
			REQUIRE(set.bucket_count() >= numbers.size());
		}
		{
			std::array<int, 7> numbers{0, 1, 2, 3, 4, 5, 6};
			any_set_t set(std::make_tuple(0, 1, 2, 3, 4, 5, 6), 20, AnyHash{}, std::allocator<value_type>{});
			REQUIRE(std::is_permutation(numbers.begin(), numbers.end(), set.begin(), set.end(), std::equal_to<>{}));
			REQUIRE(set.bucket_count() >= 20u);
			REQUIRE(set.bucket_count() >= numbers.size());
		}
		{
			std::array<int, 7> numbers{0, 1, 2, 3, 4, 5, 6};
			any_set_t set(std::make_tuple(0, 1, 2, 3, 4, 5, 6), 40, AnyHash{}, std::equal_to<>{}, std::allocator<value_type>{});
			REQUIRE(std::is_permutation(numbers.begin(), numbers.end(), set.begin(), set.end(), std::equal_to<>{}));
			REQUIRE(set.bucket_count() >= 40u);
			REQUIRE(set.bucket_count() >= numbers.size());
		}



		{
			std::array<int, 7> numbers{0, 1, 2, 3, 4, 5, 6};
			any_set_t set({0, 1, 2, 3, 4, 5, 6});
			REQUIRE(std::is_permutation(numbers.begin(), numbers.end(), set.begin(), set.end(), std::equal_to<>{}));
			REQUIRE(set.bucket_count() >= numbers.size());
		}
		{
			std::array<int, 7> numbers{0, 1, 2, 3, 4, 5, 6};
			any_set_t set({0, 1, 2, 3, 4, 5, 6}, 10u, std::allocator<value_type>{});
			REQUIRE(std::is_permutation(numbers.begin(), numbers.end(), set.begin(), set.end(), std::equal_to<>{}));
			REQUIRE(set.bucket_count() >= 10u);
			REQUIRE(set.bucket_count() >= numbers.size());
		}
		{
			std::array<int, 7> numbers{0, 1, 2, 3, 4, 5, 6};
			any_set_t set({0, 1, 2, 3, 4, 5, 6}, 16u, AnyHash{}, std::allocator<value_type>{});
			REQUIRE(std::is_permutation(numbers.begin(), numbers.end(), set.begin(), set.end(), std::equal_to<>{}));
			REQUIRE(set.bucket_count() >= 16u);
			REQUIRE(set.bucket_count() >= numbers.size());
		}
		{
			std::array<int, 7> numbers{0, 1, 2, 3, 4, 5, 6};
			any_set_t set({0, 1, 2, 3, 4, 5, 6}, 40u, AnyHash{}, std::equal_to<>{}, std::allocator<value_type>{});
			REQUIRE(std::is_permutation(numbers.begin(), numbers.end(), set.begin(), set.end(), std::equal_to<>{}));
			REQUIRE(set.bucket_count() >= 40u);
			REQUIRE(set.bucket_count() >= numbers.size());
		}
	}
}

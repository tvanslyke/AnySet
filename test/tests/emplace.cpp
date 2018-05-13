#include <iostream>
#include <utility>
#include "extra-hash.h"
#include "any-set.h"

template <class L, class R>
std::ostream& std::operator<<(std::ostream& os, const std::pair<L, R>& p)
{ return os << "std::pair(" << p.first << ", " << p.second << ')'; }

using pair_t = std::pair<int, double>;

struct Emplaceable:
	public std::pair<int, double>
{
	using std::pair<int, double>::pair;
	Emplaceable() = default;
	Emplaceable(const Emplaceable& other) = delete;
	Emplaceable& operator=(const Emplaceable& other) = delete;
	// Emplaceable(const std::pair<int, double>& other) = delete;
	// Emplaceable& operator=(const std::pair<int, double>& other) = delete;
};
template <>
struct te::Hash<Emplaceable>:
	public te::Hash<std::pair<int, double>>
{};

TEST_CASE("Emplace", "[emplace]") {

	using namespace te;
	using namespace std::literals;
	SECTION("Emplacement can insert move-only types") {
		any_set_t set;
		set._assert_invariants();
		{
			auto [pos, success] = set.emplace<Emplaceable>();
			set._assert_invariants();
			REQUIRE(success);
			REQUIRE(set.size() == 1u);
			REQUIRE(as<Emplaceable>(*pos) == Emplaceable());
		}
		{
			auto [pos, success] = set.emplace<Emplaceable>(5, 6.0);
			set._assert_invariants();
			REQUIRE(success);
			REQUIRE(set.size() == 2u);
			REQUIRE(as<Emplaceable>(*pos) == Emplaceable(5, 6.0));
		}
		{
			auto [pos, success] = set.emplace<Emplaceable>(5, 6.0);
			set._assert_invariants();
			REQUIRE(not success);
			REQUIRE(set.size() == 2u);
			REQUIRE(as<Emplaceable>(*pos) == Emplaceable(5, 6.0));
		}
		any_set_t other_set;
		other_set._assert_invariants();
		{
			auto [pos, success] = other_set.emplace_hint<Emplaceable>(other_set.cbegin());
			other_set._assert_invariants();
			REQUIRE(success);
			REQUIRE(other_set.size() == 1u);
			REQUIRE(as<Emplaceable>(*pos) == Emplaceable());
		}
		{
			auto [pos, success] = other_set.emplace_hint<Emplaceable>(other_set.cbegin(), 5, 6.0);
			other_set._assert_invariants();
			REQUIRE(success);
			REQUIRE(other_set.size() == 2u);
			REQUIRE(as<Emplaceable>(*pos) == Emplaceable(5, 6.0));
		}
		{
			auto [pos, success] = other_set.emplace_hint<Emplaceable>(other_set.cbegin(), 5, 6.0);
			other_set._assert_invariants();
			REQUIRE(not success);
			REQUIRE(other_set.size() == 2u);
			REQUIRE(as<Emplaceable>(*pos) == Emplaceable(5, 6.0));
		}
		REQUIRE(set == other_set);
	}
	SECTION("Emplacement and insertion of copyable types are semantically equivalent") {
		any_set_t set;
		{
			auto [pos, success] = set.emplace<pair_t>();
			REQUIRE(success);
			REQUIRE(set.size() == 1u);
			REQUIRE(as<pair_t>(*pos) == pair_t());
		}
		{
			auto [pos, success] = set.emplace<pair_t>(5, 6.0);
			REQUIRE(success);
			REQUIRE(set.size() == 2u);
			REQUIRE(as<pair_t>(*pos) == pair_t(5, 6.0));
		}
		{
			auto [pos, success] = set.emplace<pair_t>(5, 6.0);
			REQUIRE(not success);
			REQUIRE(set.size() == 2u);
			REQUIRE(as<pair_t>(*pos) == pair_t(5, 6.0));
		}
		any_set_t other_set;
		
		other_set.insert(pair_t());
		other_set.insert(pair_t(5, 6.0));
		auto [_, success] = other_set.emplace<pair_t>(5, 6.0);
		REQUIRE(not success);
		REQUIRE(set == other_set);
	}
}

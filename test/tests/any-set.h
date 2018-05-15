#ifndef ANY_SET_TEST_INCLUDE_H
#define ANY_SET_TEST_INCLUDE_H
#include "AnySet.h"
#include "SetOperations.h"
#include <cstddef>
#include <string>
#include <vector>
#include <array>
#include <algorithm>
#include <iterator>
#include <catch/catch.hpp>

using any_set_t = te::AnySet<>;
using value_type = typename te::AnySet<>::value_type;
using size_type = typename te::AnySet<>::size_type;
using difference_type = typename te::AnySet<>::difference_type;
using iterator = typename te::AnySet<>::iterator;
using const_iterator = typename te::AnySet<>::const_iterator;
using local_iterator = typename te::AnySet<>::local_iterator;
using const_local_iterator = typename te::AnySet<>::const_local_iterator;

using te::as;
using te::try_as;

struct UniqueInt:
	std::unique_ptr<int>
{

	using std::unique_ptr<int>::unique_ptr;
	
	UniqueInt(std::unique_ptr<int>&& p):
		std::unique_ptr<int>(std::move(p))
	{
		
	}

	friend bool operator==(const UniqueInt& left, const UniqueInt& right)
	{
		if(left.get() == right.get())
			return true;
		else if(left.get() and right.get())
			return *left == *right;
		else
			return false;
	}

	static UniqueInt make(int value)
	{
		return UniqueInt(
			std::make_unique<int>(value)
		);
	}
};

template <>
struct te::Hash<UniqueInt> {

	std::size_t operator()(const UniqueInt& p) const
	{ return p ? te::compute_hash(*p) : 0u; }
};

static const std::array<int, 10> test_ints{
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9
};

static const std::array<std::string, 10> string_names{
	"Adam",
	"Billy",
	"Cathy",
	"David",
	"Elizabeth",
	"Franklin",
	"George",
	"Helen",
	"Irma",
	"Joseph"
};

template <>
struct te::Hash<std::size_t>
{
	std::size_t operator()(std::size_t n) const
	{ return n; }
};

template <class T>
std::vector<T> to_vector(const any_set_t& set)
{
	std::vector<T> vec;
	for(const auto& v: set)
		vec.push_back(as<T>(v));
	return vec;
}

template <class T>
std::vector<T> instances_of(const any_set_t& set)
{
	std::vector<T> vec;
	for(const auto& v: set)
		if(const T* p = try_as<T>(v); static_cast<bool>(p))
			vec.push_back(*p);
	return vec;
}



#endif /* ANY_SET_TEST_INCLUDE_H */

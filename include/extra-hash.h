#ifndef EXTRA_HASH_H
#define EXTRA_HASH_H

#include <functional>
#include <cstddef>
#include <complex>
#include <utility>
#include <tuple>
#include "AnyHash.h"
#include "AnySet.h"

namespace te {
inline std::size_t hash_combine(std::size_t first, std::size_t second)
{ return first ^ (second + 0x9e3779b9 + (first << 6) + (first >> 2)); }

template <
	class T, 
	class U, 
	class ... Args, 
	class = std::enable_if<
		(sizeof...(Args) > 0u) 
		and std::is_same_v<T, std::size_t> 
		and std::is_same_v<U, std::size_t>
		and std::conjunction_v<std::is_same_v<Args, std::size_t>...>
	>
>
inline std::size_t hash_combine(T first, U second, Args ... args)
{
	return hash_combine(hash_combine(first, second), args...);
}

namespace detail {

struct ArgHash {
	template <class ... T>
	std::size_t operator()(T&& ... args) const
	{
		if constexpr(sizeof...(args) == 1u)
			return compute_hash(std::forward<T>(args) ...);
		else
			return hash_combine(compute_hash(std::forward<T>(args)) ...);
	}
};
} /* namespace detail */



// overload compute_hash for std::tuple
template <class ... T>
struct Hash<std::tuple<T...>>
{
	std::size_t operator()(const std::tuple<T ...>& tup) const
	{ return std::apply(detail::ArgHash{}, tup); }
};

// overload compute_hash for std::pair
template <class T, class U>
struct Hash<std::pair<T, U>>
{
	std::size_t operator()(const std::pair<T, U>& p) const
	{ return std::apply(detail::ArgHash{}, p); }
};

// overload compute_hash for std::pair
template <class T>
struct Hash<std::complex<T>>
{
	std::size_t operator()(const std::complex<T>& c) const
	{ return std::invoke(detail::ArgHash{}, real(c), imag(c)); }
};

template <class T, std::size_t N>
struct Hash<std::array<T, N>>
{
	std::size_t operator()(const std::array<T, N>& a) const
	{ return std::apply(detail::ArgHash{}, a); }
};

template <class H, class E, class A>
std::size_t compute_hash(const AnySet<H, E, A>& set)
{
	std::size_t hash_v = 0;
	for(const auto& v: set)
		hash_v = hash_combine(hash_v, compute_hash(v));
	return hash_v;
}

} /* namespace te */



#endif /* EXTRA_HASH_H */

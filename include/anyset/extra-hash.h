#ifndef EXTRA_HASH_H
#define EXTRA_HASH_H

#include <functional>
#include <cstddef>
#include <cstdint>
#include <complex>
#include <utility>
#include <tuple>
#include <bitset>
#include "AnyHash.h"
#include "AnySet.h"

namespace te {


/**
 * @brief Combine two hash values using a formula that is compatible with 
 *        boost::hash_combine().
 * 
 * @note This function is not order agnostic with respect to its arguments.
 *       That is, the returned hash value can change if the argument order
 *       changes.
 * 
 * @param first  - First hash value.
 * @param second - Second hash value.
 * 
 * @return A hash value obtained from combining the two given hash values.
 */ 
inline std::size_t hash_combine(std::size_t first, std::size_t second)
{
	return first ^ (second + std::size_t(0x9e3779b9ull) + (first << 6) + (first >> 2));
}


/**
 * @brief Combine an arbitrary number of hash values using a formula that is 
 *        compatible with boost::hash_combine().
 * 
 * @note This function is not order agnostic with respect to its arguments.
 *       That is, the returned hash value can change if the argument order
 *       changes.
 * 
 * @param first  - First hash value.
 * @param second - Second hash value.
 * @param args   - Other hash values.
 * 
 * @return A hash value obtained from combining the given hash values.
 */ 
template <
	class T, class U, class ... Args, 
	class = std::enable_if<
		(sizeof...(Args) > 0u) 
		and std::is_same_v<T, std::size_t> 
		and std::is_same_v<U, std::size_t>
		and std::conjunction_v<std::is_same<Args, std::size_t>...>
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
			return hash_value(std::forward<T>(args) ...);
		else
			return hash_combine(hash_value(std::forward<T>(args)) ...);
	}
};
} /* namespace detail */



/// Specialize te::Hash for std::tuple.
template <class ... T>
struct Hash<std::tuple<T...>>
{
	std::size_t operator()(const std::tuple<T ...>& tup) const
	{ return std::apply(detail::ArgHash{}, tup); }
};

/// Specialize te::Hash for std::pair.
template <class T, class U>
struct Hash<std::pair<T, U>>
{
	std::size_t operator()(const std::pair<T, U>& p) const
	{ return std::apply(detail::ArgHash{}, p); }
};

/// Specialize te::Hash for std::complex.
template <class T>
struct Hash<std::complex<T>>
{
	std::size_t operator()(const std::complex<T>& c) const
	{ return std::invoke(detail::ArgHash{}, real(c), imag(c)); }
};

/// Specialize te::Hash for std::array.
template <class T, std::size_t N>
struct Hash<std::array<T, N>>
{
	std::size_t operator()(const std::array<T, N>& a) const
	{ return std::apply(detail::ArgHash{}, a); }
};

} /* namespace te */



#endif /* EXTRA_HASH_H */

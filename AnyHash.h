#ifndef ANY_HASH_H
#define ANY_HASH_H

#include <functional>
#include <cstddef>
#include <type_traits>

namespace te {

template <class T = void>
struct Hash: std::hash<T> {};

namespace detail {

template <class T>
struct has_hash_specialization: 
	std::conditional_t<
		std::is_default_constructible_v<Hash<T>>,
		std::true_type,
		std::false_type
	> 
{

};

template <class T>
inline constexpr const bool has_hash_specialization_v = has_hash_specialization<T>::value;

} /* namespace detail */



template <>
struct Hash<void>
{
	template <class T>
	std::size_t operator()(const T& value) const
	{
		return std::invoke(Hash<T>{}, value); 
	}
};

template <class T, class = std::enable_if_t<detail::has_hash_specialization_v<T>>>
std::size_t compute_hash(const T& value)
{ return std::invoke(Hash<T>{}, value); }

namespace detail {
template <class T>
struct can_compute_hash {
private:
	template <class Hashable>
	static auto f(int) -> decltype((std::declval<std::size_t&>() = compute_hash(std::declval<Hashable>())), std::true_type{});
	
	template <class Hashable>
	static auto f(...) -> std::false_type;
public:
	static constexpr const bool value = decltype(f<T>(0))::value;
};

template <class T>
inline constexpr const bool can_compute_hash_v = can_compute_hash<T>::value;

} /* namespace detail */

struct AnyHash {
	template <class T>
	std::size_t operator()(const T& value) const
	{
		using te::compute_hash;
		return compute_hash(value);
	}
};



} /* namespace te */

#endif /* ANY_HASH_H */

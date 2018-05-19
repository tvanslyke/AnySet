#ifndef ANY_HASH_H
#define ANY_HASH_H

#include <functional>
#include <cstddef>
#include <type_traits>

namespace te {

/**
 * @brief
 * Function object that implements a hash function for instances of 
 * type T.  Inherits from std::hash<T> unless otherwise specialized by
 * the user to not do so.
 * 
 * This class is used by te::AnyHash if it has a specialization for the 
 * necessary type.  Users can specialize this template and provide an operator() 
 * for their own types, to be used by te::AnyHash.
 */
template <class T = void>
struct Hash: std::hash<T>
{

};

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



/**
 * @brief
 * Function object that implements a hash function for instances of type T.
 */
template <>
struct Hash<void>
{
	/**
	 * @brief Generic hash function.
	 * @param value - object to compute the hash code for.
	 * @return std::invoke(te::Hash<T>{}, value)
	 */
	template <class T>
	std::size_t operator()(const T& value) const
	{ return std::invoke(Hash<T>{}, value); }
};

/**
 * @brief Generic hash function.
 *
 * Users can customize the behavior of AnyHash by providing an overload of hash_value()
 * in the same namespace that their own classes are declared in.  AnyHash makes an unqualified
 * call to hash_value() like so:
 * @code
 * using te::hash_value;
 * return hash_value(value);
 * @endcode
 * 
 * This allows ADL to find overloads of hash_value() in the namespace that @p value's type is 
 * defined in.  This is the same method that boost::container_hash uses, so existing overloads
 * of hash_value() intended for boost::container_hash will also work here.
 *
 * te::hash_value() otherwise uses te::Hash to compute @p value's hash code.  If te::Hash has
 * no specialization for @p value's type, then te::hash_value() is removed from overload 
 * resolution via SFINAE.
 * 
 * @param value - object to compute the hash code for.
 * @return std::invoke(te::Hash<T>{}, value)
 *
 * @see Hash    - A template class whose specializations determine the behavior of AnyHash.
 * @see AnyHash - The default hash function type for AnySet.
 *
 * @relates AnyHash
 */
template <class T, class = std::enable_if_t<detail::has_hash_specialization_v<T>>>
std::size_t hash_value(const T& value)
{ return std::invoke(Hash<T>{}, value); }

/**
 * @brief Generic hash function object.
 * 
 * This is the default hash type for te::AnySet.  Users should not specialize
 * the operator() member function of this class.
 *
 * @see Hash         - A template class whose specializations determine the behavior of AnyHash.
 * @see hash_value() - Provides and ADL-based approach to customizing the behavior of AnyHash.
 */
struct AnyHash {

	/**
	 * @brief Implements a generic hash function.
	 * 
	 * Users should not specialize this member function.  To modify the behavior
	 * of AnyHash for different types, users can do one of two things:
	 *   1. Specialize `te::Hash<T>` for the desired type `T`, providing 
	 *      a default constructor and member function `std::size_t te::Hash<T>::operator()(const T&)`. 
	 *   2. Provide an overload of `std::size_t hash_value(const T&)` in the 
	 *      same namespace as type `T`, which will be found by ADL in AnyHash::operator()`.
	 * 
	 * @param value - object to compute the hash of.
	 * @return the computed hash of @p value.
	 *
	 * @see Hash         - A template class whose specializations determine the behavior of AnyHash.
	 * @see hash_value() - Provides and ADL-based approach to customizing the behavior of AnyHash.
	 */
	template <class T>
	std::size_t operator()(const T& value) const
	{
		using te::hash_value;
		return hash_value(value);
	}
};

} /* namespace te */

#endif /* ANY_HASH_H */

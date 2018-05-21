#ifndef SET_OPERATIONS_H
#define SET_OPERATIONS_H

#ifdef _MSC_VER
# include <iso646.h>
#endif 

#include "AnySet.h"
#include <type_traits>

/// @file SetOperations.h
/// Free functions and operator overloads to compute fundamental set operations
/// on AnySet instances:
/// * union_of()
/// * intersection_of()
/// * difference_of()
/// * symmetric_difference_of()
/// * is_subset_of()
/// * is_superset_of()

namespace te {

namespace detail {

template <class T>
struct is_any_set: public std::false_type {};

template <class H, class E, class A>
struct is_any_set<AnySet<H, E, A>>: public std::true_type {};

template <class T>
inline constexpr const bool is_any_set_v = is_any_set<T>::value;

template <class Op, class Pred, class T, class ... U>
decltype(auto) select_and_invoke(Op&& op, [[maybe_unused]] Pred pred, T&& first)
{
	return std::invoke(std::forward<Op>(op), std::forward<T>(first));
}


template <class Op, class Pred, class T, class U, class ... Args>
decltype(auto) select_and_invoke(Op&& op, Pred pred, T&& first, U&& second, Args&& ... args)
{
	using std::forward;
	using std::invoke;
	if(pred(static_cast<const T&>(first), static_cast<const U&>(second)))
	{
		return select_and_invoke(
			// bind 'first' to 'op's second argument position
			[&op, &first](auto&& f, auto&& ... a) {
				return invoke(
					forward<Op>(op),
					forward<decltype(f)>(f), forward<T>(first), forward<decltype(a)>(a)...
				);
			},
			pred, forward<U>(second), forward<Args>(args) ...
		);
	}
	else
	{
		return select_and_invoke(
			// bind 'second' to 'op's second argument position
			[&op, &second](auto&& f, auto&& ... a) {
				return invoke(
					forward<Op>(op),
					forward<decltype(f)>(f), forward<U>(second), forward<decltype(a)>(a)...
				);
			},
			pred, forward<T>(first), forward<Args>(args) ...
		);
	}
}

} /* namespace detail */

/// @name Set Operation Free-Functions
/// @{

/**
 * @brief Get the union of a group of AnySet instances.
 * 
 * All arguments (@p first and @p args...) must be AnySet instances 
 * with the same @p HashFn and @p KeyEqual values.
 *
 * @note Set membership is determined using the shared @p KeyEqual function, not 
 *       necessarily operator==.
 * 
 * @note RValue AnySet instances passed to this function will be left in a valid
 *       but unspecified state. 
 *
 * @param first - the first set in the group.
 * @param args  - the other sets in the group.
 *
 * @return the set union of @p first and @p args....
 */
template <
	class T,
	class ... U,
	class = std::enable_if_t<
		detail::is_any_set_v<std::decay_t<T>>
		and (std::is_same_v<std::decay_t<T>, std::decay_t<U>> and ...)
	>
>
std::decay_t<T> union_of(T&& first, U&& ... args)
{
	// Avoid an ICE in gcc-7.x
#if !(defined(__GNUC__) && (__GNUC__ == 7))
	// this 'const' causes an internal compiler error in gcc-7.x.  I like const enough 
	// to do this #if and even make a comment about it.
	const 
#endif
	std::size_t max_total_size = (first.size() + ... + args.size());
	auto is_better = [](auto&& left, auto&& right) {
		// return true if 'right' is the better candidate
		constexpr bool left_is_rvalue = std::is_rvalue_reference_v<decltype(left)>;
		constexpr bool right_is_rvalue = std::is_rvalue_reference_v<decltype(right)>;
		if constexpr(left_is_rvalue != right_is_rvalue)
			return right_is_rvalue;
		else
			return left.bucket_count() < right.bucket_count();
	};
	auto union_of_impl = [=](auto&& f, auto&& ... sets) -> std::decay_t<T> {
		std::decay_t<T> result(std::forward<decltype(f)>(f));
		result.max_load_factor(1.0);
		result.reserve(max_total_size);
		(result.update(std::forward<decltype(sets)>(sets)) , ...);
		return result;
	};
	return detail::select_and_invoke(
		union_of_impl, is_better, std::forward<T>(first), std::forward<U>(args)...
	);
}

/**
 * @brief Get the intersection of a group of AnySet instances.
 * 
 * All arguments (@p first and @p args...) must be AnySet instances 
 * with the same @p HashFn and @p KeyEqual values.
 *
 * @note Set membership is determined using the shared @p KeyEqual function, not 
 *       necessarily operator==.
 * 
 * @note RValue AnySet instances passed to this function will be left in a valid
 *       but unspecified state. 
 *
 * @param first - the first set in the group.
 * @param args  - the other sets in the group.
 *
 * @return the set intersection of @p first and @p args....
 */
template <
	class T,
	class ... U,
	class = std::enable_if_t<
		detail::is_any_set_v<std::decay_t<T>>
		and (std::is_same_v<std::decay_t<T>, std::decay_t<U>> and ...)
	>
>
std::decay_t<T> intersection_of(T&& first, U&& ... args)
{
	auto is_better = [](auto&& left, auto&& right) {
		// return true if 'right' is the better candidate
		constexpr bool left_is_rvalue = std::is_rvalue_reference_v<decltype(left)>;
		constexpr bool right_is_rvalue = std::is_rvalue_reference_v<decltype(right)>;
		if constexpr(left_is_rvalue != right_is_rvalue)
			return right_is_rvalue;
		else
			return left.size() > right.size();
	};
	auto intersection_of_impl = [](auto&& f, const auto& ... set) -> std::decay_t<T> {
		std::decay_t<T> result(std::forward<decltype(f)>(f));
		result.max_load_factor(1.0);
		auto pos = result.begin();
		while(pos != result.end())
		{
			const auto& any_v = *pos;
			auto has_value = [&](const auto& s) { return s.contains_value(any_v); };
			if(not static_cast<bool>((has_value(set) and ...)))
			{
				pos = result.erase(pos);
			}
			else
			{
				++pos;
			}
		}
		return result;
	};
	return detail::select_and_invoke(
		intersection_of_impl, is_better, std::forward<T>(first), std::forward<U>(args)...
	);
}


/**
 * @brief Get the symmetric difference of a group of AnySet instances.
 * 
 * All arguments (@p first and @p args...) must be AnySet instances 
 * with the same @p HashFn and @p KeyEqual values.
 *
 * @note Set membership is determined using the shared @p KeyEqual function, not 
 *       necessarily operator==.
 * 
 * @note RValue AnySet instances passed to this function will be left in a valid
 *       but unspecified state. 
 *
 * @param first - the first set in the group.
 * @param args  - the other sets in the group.
 *
 * @return the set symmetric difference of @p first and @p args....
 */
template <
	class T,
	class ... U,
	class = std::enable_if_t<
		detail::is_any_set_v<std::decay_t<T>>
		and (std::is_same_v<std::decay_t<T>, std::decay_t<U>> and ...)
	>
>
std::decay_t<T> symmetric_difference_of(T&& first, U&& ... args)
{
	auto is_better = [](auto&& left, auto&& right) {
		// return true if 'right' is the better candidate
		constexpr bool left_is_rvalue = std::is_rvalue_reference_v<decltype(left)>;
		constexpr bool right_is_rvalue = std::is_rvalue_reference_v<decltype(right)>;
		if constexpr(left_is_rvalue != right_is_rvalue)
			return right_is_rvalue;
		else
			return left.bucket_count() < right.bucket_count();
	};
	auto symmetric_difference_of_impl = [](auto&& f, auto&& ... set) -> std::decay_t<T> {
		std::decay_t<std::decay_t<T>> result(std::forward<decltype(f)>(f));
		result.max_load_factor(1.0);
		auto inplace_symdiff = [&](auto&& s) {
			result ^= std::forward<decltype(s)>(s);
		};
		(... , inplace_symdiff(std::forward<decltype(set)>(set)));
		return result;
	};
	return detail::select_and_invoke(
		symmetric_difference_of_impl, is_better, std::forward<T>(first), std::forward<U>(args)...
	);
}

/**
 * @brief Get the (asymmetric) difference of a group of AnySet instances.
 * 
 * All arguments (@p first and @p args...) must be AnySet instances 
 * with the same @p HashFn and @p KeyEqual values.
 *
 * @note Set membership is determined using the shared @p KeyEqual function, not 
 *       necessarily operator==.
 * 
 * @note RValue AnySet instances passed to this function will be left in a valid
 *       but unspecified state. 
 *
 * @param first - the first set in the group.
 * @param args  - the other sets in the group.
 *
 * @return the set (asymmetric) difference of @p first and @p args....
 */
template <
	class T,
	class ... U,
	class = std::enable_if_t<
		detail::is_any_set_v<std::decay_t<T>>
		and (std::is_same_v<std::decay_t<T>, std::decay_t<U>> and ...)
	>
>
std::decay_t<T> difference_of(T&& left, const U& ... right)
{
	if constexpr(std::is_rvalue_reference_v<decltype(left)>)
	{
		std::decay_t<T> result(std::move(left));
		result.max_load_factor(1.0);
		
		for(auto pos = result.begin(); pos != result.end();)
		{
			if(((right.contains_value(*pos)) or ...))
			{
				pos = result.erase(pos);
			}
			else
			{
				++pos;
			}
		}
		return result;
	}
	else
	{
		std::decay_t<T> result(left.bucket_count());
		
		for(auto pos = left.begin(); pos != left.end(); ++pos)
		{
			if(not ((right.contains_value(*pos)) or ...))
				result.push(left.dup(pos));
		}
		return result;
	}
}

/**
 * @brief Determine whether @p sub is a subset of @p super.
 * 
 * @param sub   - The set to be considered the subset of @p super.
 * @param super - The set to be considered the superset of @p sub.
 * 
 * @note Set membership is determined using the shared @p KeyEqual function, not 
 *       necessarily operator==.
 * 
 * @return true if @p sub is a subset of @p super.
 */
template <class H, class E, class A>
bool is_subset_of(const AnySet<H, E, A>& sub, const AnySet<H, E, A>& super)
{
	if(sub.size() > super.size())
		return false;
	for(const auto& v: sub)
	{
		if(not super.contains_value(v))
			return false;
	}
	return true;
}

/**
 * @brief Determine whether @p super is a superset of @p sub.
 * 
 * @param super - The set to be considered the superset of @p sub.
 * @param sub   - The set to be considered the subset of @p super.
 * 
 * @note Set membership is determined using the shared @p KeyEqual function, not 
 *       necessarily operator==.
 * 
 * @return true if @p super is a superset of @p sub.
 */
template <class H, class E, class A>
bool is_superset_of(const AnySet<H, E, A>& super, const AnySet<H, E, A>& sub)
{ return is_subset_of(sub, super); }


/// @} Set Operation Free-Functions

/// @name Set Operation Operator Overloads
/// @{

/// @name Set Union Operators
/// @{

/// Compute the union of @p left and @p right as if by `union_of(left, right);`
template <
	class T, class U,
	class = std::enable_if_t<
		detail::is_any_set_v<std::decay_t<T>>
		and std::is_same_v<std::decay_t<T>, std::decay_t<U>>
	>
>
std::decay_t<T> operator+(T&& left, U&& right)
{ return union_of(std::forward<T>(left), std::forward<U>(right)); }

/// Compute the union of @p left and @p right as if by `left = (left + right);`
template <class H, class E, class A>
AnySet<H, E, A>& operator+=(AnySet<H, E, A>& left, AnySet<H, E, A>&& right)
{ return left.update(std::move(right)); }

/// Compute the union of @p left and @p right as if by `left = (left + right);`
template <class H, class E, class A>
AnySet<H, E, A>& operator+=(AnySet<H, E, A>& left, const AnySet<H, E, A>& right)
{ return left.update(right); }


/// Compute the union of @p left and @p right as if by `union_of(left, right);`
template <
	class T, class U,
	class = std::enable_if_t<
		detail::is_any_set_v<std::decay_t<T>>
		and std::is_same_v<std::decay_t<T>, std::decay_t<U>>
	>
>
std::decay_t<T> operator|(T&& left, U&& right)
{ return std::forward<T>(left) + std::forward<U>(right); }

/// Compute the union of @p left and @p right as if by `left = (left + right);`
template <class H, class E, class A>
AnySet<H, E, A>& operator|=(AnySet<H, E, A>& left, AnySet<H, E, A>&& right)
{ return left += std::move(right); }

/// Compute the union of @p left and @p right as if by `left = (left + right);`
template <class H, class E, class A>
AnySet<H, E, A>& operator|=(AnySet<H, E, A>& left, const AnySet<H, E, A>& right)
{ return left += right; }

/// @} Set Union Operators

/// @name Set Intersection Operators
/// @{


/// Compute the intersection of @p left and @p right as if by `intersection_of(left, right);`
template <
	class T, class U,
	class = std::enable_if_t<
		detail::is_any_set_v<std::decay_t<T>>
		and std::is_same_v<std::decay_t<T>, std::decay_t<U>>
	>
>
std::decay_t<T> operator&(T&& left, U&& right)
{ return intersection_of(std::forward<T>(left), std::forward<U>(right)); }

/// Compute the intersection of @p left and @p right as if by `left = (left & right);`
template <class H, class E, class A>
AnySet<H, E, A>& operator&=(AnySet<H, E, A>& left, AnySet<H, E, A>&& right)
{ return left = intersection_of(std::move(left), std::move(right)); }

/// Compute the intersection of @p left and @p right as if by `left = (left & right);`
template <class H, class E, class A>
AnySet<H, E, A>& operator&=(AnySet<H, E, A>& left, const AnySet<H, E, A>& right)
{ return left = intersection_of(std::move(left), right); }

/// @} Set Intersection Operators

/// @name Set Difference Operators
/// @{


/// Compute the (asymmetric) difference of @p left and @p right as if by `difference_of(left, right);`
template <
	class T, class U,
	class = std::enable_if_t<
		detail::is_any_set_v<std::decay_t<T>>
		and std::is_same_v<std::decay_t<T>, std::decay_t<U>>
	>
>
std::decay_t<T> operator-(T&& left, U&& right)
{ return difference_of(std::forward<T>(left), std::forward<U>(right)); }

/// Compute the (asymmetric) difference of @p left and @p right as if by `left = (left - right);`
template <class H, class E, class A>
AnySet<H, E, A>& operator-=(AnySet<H, E, A>& left, AnySet<H, E, A>&& right)
{ return left = difference_of(std::move(left), std::move(right)); }

/// Compute the (asymmetric) difference of @p left and @p right as if by `left = (left - right);`
template <class H, class E, class A>
AnySet<H, E, A>& operator-=(AnySet<H, E, A>& left, const AnySet<H, E, A>& right)
{ return left = difference_of(std::move(left), right); }


/// @} Set Difference Operators

/// @name Set Symmetric Difference Operators
/// @{

/// Compute the symmetric difference of @p left and @p right as if by `symmetric_difference_of(left, right);`
template <
	class T, class U,
	class = std::enable_if_t<
		detail::is_any_set_v<std::decay_t<T>>
		and std::is_same_v<std::decay_t<T>, std::decay_t<U>>
	>
>
std::decay_t<T> operator^(T&& left, U&& right)
{ return symmetric_difference_of(std::forward<T>(left), std::forward<U>(right)); }

/// Compute the symmetric difference of @p left and @p right as if by `left = (left ^ right);`
template <
	class H, class E, class A, class U,
	class = std::enable_if_t<std::is_same_v<AnySet<H, E, A>, std::decay_t<U>>>
>
AnySet<H, E, A>& operator^=(AnySet<H, E, A>& left, U&& right)
{
	for(auto pos = right.begin(); pos != right.end();)
	{
		auto [ins_pos, next, success] = left.splice_or_copy(
			std::forward<U>(right), pos
		);
		pos = next;
		if(not success)
			left.pop(ins_pos);
	}
	return left;
}

/// @} Set Symmetric Difference Operators

/// @name Subset/Superset Operators
/// @{

/// Semantically equivalent to `is_subset_of(left, right);`.
template <class H, class E, class A>
bool operator<=(const AnySet<H, E, A>& left, const AnySet<H, E, A>& right)
{ return is_subset_of(left, right); }

/// Semantically equivalent to `is_subset_of(left, right) && !(left.size() == right.size());`.
template <class H, class E, class A>
bool operator<(const AnySet<H, E, A>& left, const AnySet<H, E, A>& right)
{ return is_subset_of(left, right) and (left.size() < right.size()); }

/// Semantically equivalent to `is_superset_of(left, right);`.
template <class H, class E, class A>
bool operator>=(const AnySet<H, E, A>& left, const AnySet<H, E, A>& right)
{ return is_superset_of(left, right); }

/// Semantically equivalent to `is_subset_of(left, right) && !(left.size() == right.size());`.
template <class H, class E, class A>
bool operator>(const AnySet<H, E, A>& left, const AnySet<H, E, A>& right)
{ return is_superset_of(left, right) and (left.size() > right.size()); }

/// @} Subset/Superset Operators

/// @name Stream Insertion Operators
/// @{

/// Write an AnySet instance to the std::ostream @p os.
template <class H, class E, class C>
std::ostream& operator<<(std::ostream& os, const AnySet<H, E, C>& set)
{
	os << '{';
	if(set.size() > 0)
	{
		auto pos = set.begin();
		os << *pos++;
		for(auto stop = set.end(); pos != stop; ++pos)
			os << ", " << *pos;
	}
	os << '}';
	return os;
}

/// @} Stream Insertion Operators


} /* namespace te */




#endif /* SET_OPERATIONS_H */

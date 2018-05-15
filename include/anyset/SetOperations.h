#ifndef SET_OPERATIONS_H
#define SET_OPERATIONS_H

#include "AnySet.h"
#include <type_traits>

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
	const std::size_t max_total_size(first.size() + (args.size() + ...));
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
		result._assert_invariants();
		result.max_load_factor(1.0);
		auto pos = result.begin();
		auto stop = result.end();
		while(pos != stop)
		{
			const auto& any_v = *pos;
			auto has_value = [&](const auto& s) { return s.contains_value(any_v); };
			if(not static_cast<bool>((has_value(set) and ...)))
				pos = result.erase(pos);
			else
				++pos;
		}
		result._assert_invariants();
		return result;
	};
	return detail::select_and_invoke(
		intersection_of_impl, is_better, std::forward<T>(first), std::forward<U>(args)...
	);
}

template <
	class T,
	class ... U,
	class = std::enable_if_t<
		detail::is_any_set_v<std::decay_t<T>>
		and (std::is_same_v<std::decay_t<T>, std::decay_t<U>> and ...)
	>
>
std::pair<std::decay_t<T>, std::decay_t<T>> exclusive_elements_of(T&& first, U&& ... args)
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
	auto exclusive_elements_of_impl = [](auto&& f, const auto& ... set) -> std::decay_t<T> {
		std::decay_t<std::decay_t<T>> result(std::forward<decltype(f)>(f));
		std::decay_t<std::decay_t<T>> duplicates;
		result._assert_invariants();
		result.max_load_factor(1.0);
		auto symdiff = [&](auto&& other) {
			using iter_t = typename std::decay_t<std::decay_t<T>>::iterator;
			for(iter_t pos = other.begin(), stop = other.end(); pos != stop;)
			{
				if(duplicates.contains_value(*pos))
				{
					++pos;
					continue;
				}
				auto [ins_pos, next, success] = result.splice_or_copy(
					std::forward<decltype(other)>(other), pos
				);
				pos = next;
				if(success)
					continue;
				duplicates.splice(std::move(result), ins_pos);
			}
		};
		(... , symdiff(std::forward<decltype(set)>(set)));
		result._assert_invariants();
		return std::make_pair(std::move(result), std::move(duplicates));
	};
	return detail::select_and_invoke(
		exclusive_elements_of_impl, is_better, std::forward<T>(first), std::forward<U>(args)...
	);
}

template <
	class H, class E, class A, class U,
	class = std::enable_if_t<std::is_same_v<AnySet<H, E, A>, std::decay_t<U>>>
>
AnySet<H, E, A>& operator^=(AnySet<H, E, A>& left, U&& right)
{
	for(auto pos = right.begin(), stop = right.end(); pos != stop;)
	{
		auto [ins_pos, next, success] = left.splice_or_copy(
			std::forward<decltype(right)>(right), pos
		);
		pos = next;
		if(not success)
			left.pop(ins_pos);
	}
	return left;
}

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
	auto symmetric_difference_of_impl = [](auto&& f, const auto& ... set) -> std::decay_t<T> {
		std::decay_t<std::decay_t<T>> result(std::forward<decltype(f)>(f));
		std::decay_t<std::decay_t<T>> duplicates;
		result._assert_invariants();
		result.max_load_factor(1.0);
		auto inplace_symdiff = [&](auto&& s) {
			result ^= s;
		};
		(... , inplace_symdiff(std::forward<decltype(set)>(set)));
		result._assert_invariants();
		return result;
	};
	return detail::select_and_invoke(
		symmetric_difference_of_impl, is_better, std::forward<T>(first), std::forward<U>(args)...
	);
}

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
		
		for(auto pos = result.begin(), stop = result.end(); pos != stop;)
		{
			if(((right.contains_value(*pos)) or ...))
			{
				result.erase(pos);
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

template <class H, class E, class A>
bool is_superset_of(const AnySet<H, E, A>& super, const AnySet<H, E, A>& sub)
{ return is_subset_of(sub, super); }


template <
	class T, class U,
	class = std::enable_if_t<
		detail::is_any_set_v<std::decay_t<T>>
		and std::is_same_v<std::decay_t<T>, std::decay_t<U>>
	>
>
std::decay_t<T> operator+(T&& left, U&& right)
{ return union_of(std::forward<T>(left), std::forward<U>(right)); }

template <class H, class E, class A>
AnySet<H, E, A>& operator+=(AnySet<H, E, A>& left, AnySet<H, E, A>&& right)
{ return left.update(std::move(right)); }

template <class H, class E, class A>
AnySet<H, E, A>& operator+=(AnySet<H, E, A>& left, const AnySet<H, E, A>& right)
{ return left.update(right); }


template <
	class T, class U,
	class = std::enable_if_t<
		detail::is_any_set_v<std::decay_t<T>>
		and std::is_same_v<std::decay_t<T>, std::decay_t<U>>
	>
>
std::decay_t<T> operator|(T&& left, U&& right)
{ return std::forward<T>(left) + std::forward<U>(right); }

template <class H, class E, class A>
AnySet<H, E, A>& operator|=(AnySet<H, E, A>& left, AnySet<H, E, A>&& right)
{ return left += std::move(right); }

template <class H, class E, class A>
AnySet<H, E, A>& operator|=(AnySet<H, E, A>& left, const AnySet<H, E, A>& right)
{ return left += right; }


template <
	class T, class U,
	class = std::enable_if_t<
		detail::is_any_set_v<std::decay_t<T>>
		and std::is_same_v<std::decay_t<T>, std::decay_t<U>>
	>
>
std::decay_t<T> operator&(T&& left, U&& right)
{ return intersection_of(std::forward<T>(left), std::forward<U>(right)); }

template <class H, class E, class A>
AnySet<H, E, A>& operator&=(AnySet<H, E, A>& left, AnySet<H, E, A>&& right)
{ return left = intersection_of(std::move(left), std::move(right)); }

template <class H, class E, class A>
AnySet<H, E, A>& operator&=(AnySet<H, E, A>& left, const AnySet<H, E, A>& right)
{ return left = intersection_of(std::move(left), right); }


template <
	class T, class U,
	class = std::enable_if_t<
		detail::is_any_set_v<std::decay_t<T>>
		and std::is_same_v<std::decay_t<T>, std::decay_t<U>>
	>
>
std::decay_t<T> operator-(T&& left, U&& right)
{ return difference_of(std::forward<T>(left), std::forward<U>(right)); }

template <class H, class E, class A>
AnySet<H, E, A>& operator-=(AnySet<H, E, A>& left, AnySet<H, E, A>&& right)
{ return left = difference_of(std::move(left), std::move(right)); }

template <class H, class E, class A>
AnySet<H, E, A>& operator-=(AnySet<H, E, A>& left, const AnySet<H, E, A>& right)
{ return left = difference_of(std::move(left), right); }


template <
	class T, class U,
	class = std::enable_if_t<
		detail::is_any_set_v<std::decay_t<T>>
		and std::is_same_v<std::decay_t<T>, std::decay_t<U>>
	>
>
std::decay_t<T> operator^(T&& left, U&& right)
{ return symmetric_difference_of(std::forward<T>(left), std::forward<U>(right)); }


template <class H, class E, class A>
bool operator<=(const AnySet<H, E, A>& left, const AnySet<H, E, A>& right)
{ return is_subset_of(left, right); }

template <class H, class E, class A>
bool operator<(const AnySet<H, E, A>& left, const AnySet<H, E, A>& right)
{ return is_subset_of(left, right) and (left.size() < right.size()); }

template <class H, class E, class A>
bool operator>=(const AnySet<H, E, A>& left, const AnySet<H, E, A>& right)
{ return is_superset_of(left, right); }

template <class H, class E, class A>
bool operator>(const AnySet<H, E, A>& left, const AnySet<H, E, A>& right)
{ return is_superset_of(left, right) and (left.size() > right.size()); }





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

} /* namespace te */

#endif /* SET_OPERATIONS_H */

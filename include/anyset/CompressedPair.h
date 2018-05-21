#ifndef COMPRESSED_PAIR_H
#define COMPRESSED_PAIR_H

#ifdef _MSC_VER
# include <iso646.h>
#endif 

#include "ValueHolder.h"

namespace te {

namespace detail {


template <class T>
struct CompressedPairBase:
	private ValueHolder<T>
{
	template <class ... Args>
	CompressedPairBase(Args&& ... args):
		ValueHolder<T>(std::forward<Args>(args)...)
	{
		
	}

	const T& first() const &
	{ return get_value(static_cast<const ValueHolder<T>&>(*this)); }

	const T&& first() const &&
	{ return std::move(get_value(std::move(static_cast<const ValueHolder<T>&&>(*this)))); }

	T& first() &
	{ return get_value(static_cast<ValueHolder<T>&>(*this)); }

	T&& first() &&
	{ return std::move(get_value(static_cast<ValueHolder<T>&&>(*this).value())); }

};

} /* namespace detail */


/**
 *  Compressed pair class like boost::compressed_pair.  Hand-rolled to avoid the small dependency.
 */
template <class T, class U>
struct CompressedPair:
	public detail::CompressedPairBase<T>,
	private ValueHolder<U>
{
private:
	using left_base = detail::CompressedPairBase<T>;
	using right_base = ValueHolder<U>;
	using self_type = CompressedPair<T, U>;
public:
	constexpr CompressedPair() = default;

	template <
		class Left,
		class Right,
		class = std::enable_if_t<std::is_same_v<std::decay_t<Left>, T>>,
		class = std::enable_if_t<std::is_same_v<std::decay_t<Right>, U>>
	>
	CompressedPair(Left&& left, Right&& right):
		left_base(std::forward<Left>(left)), right_base(std::forward<Right>(right))
	{
		
	}
	
	using left_base::first;

	const U& second() const &
	{ return get_value(static_cast<const ValueHolder<U>&>(*this)); }

	const U&& second() const &&
	{ return std::move(get_value(static_cast<const ValueHolder<U>&&>(*this))); }

	U& second() &
	{ return get_value(static_cast<ValueHolder<U>&>(*this)); }

	U&& second() &&
	{ return std::move(get_value(static_cast<ValueHolder<U>&&>(*this))); }

	void swap(CompressedPair& other) 
		noexcept(std::is_nothrow_swappable_v<T> and std::is_nothrow_swappable_v<U>)
	{
		using std::swap;
		swap(first(), other.first());
		swap(second(), other.second());
	}
};

template <class T, class U>
CompressedPair<std::decay_t<T>, std::decay_t<U>> make_compressed_pair(T&& left, U&& right)
{
	return CompressedPair<std::decay_t<T>, std::decay_t<U>>(
		std::forward<T>(left), std::forward<U>(right)
	);
}

template <class T, class U>
void swap(CompressedPair<T, U>& left, CompressedPair<T, U>& right) noexcept(noexcept(left.swap(right)))
{ left.swap(right); }

} /* namespace te */

#endif /* COMPRESSED_PAIR_H */

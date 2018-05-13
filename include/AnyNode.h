#ifndef ANY_NODE_H
#define ANY_NODE_H

#include <typeindex>
#include <typeinfo>
#include <string>
#include <cassert>
#include <type_traits>
#include <optional>
#include <variant>
#include <ostream>
#include <memory>
#include <any>
#include "ValueHolder.h"

namespace te {

struct AnyListSentinalType;

template <class H, class E, class A>
struct AnySet;

template <class H, class E, class A>
std::ostream& operator<<(std::ostream& os, const AnySet<H, E, A>& set);

namespace detail {

template <class S, class T>
class is_streamable_helper
{
	template<class Stream, class Type>
	static auto test(int) -> decltype( 
		(std::declval<Stream>() << std::declval<Type>()), std::true_type() 
	);

	template<class, class>
	static auto test(...) -> std::false_type;

public:
	static constexpr const bool value = decltype(test<S,T>(0))::value;
};

template <class T>
using is_streamable = is_streamable_helper<std::ostream&, T>;

template <class T>
inline constexpr const bool is_streamable_v = is_streamable<T>::value;

template <class T, class = void>
struct is_equality_comparable: std::false_type {};
template <class T>
struct is_equality_comparable<
	T, 
	std::enable_if_t<
		true, 
		decltype((void)(std::declval<T>() == std::declval<T>()))
		>
	>: std::true_type {};
template <class T>
inline constexpr const bool is_equality_comparable_v = is_equality_comparable<T>::value;

template <class T, class = void>
struct is_inequality_comparable: std::false_type {};
template <class T>
struct is_inequality_comparable<
	T, 
	std::enable_if_t<
		true, 
		decltype((void)(std::declval<T>() != std::declval<T>()), (void)0)
		>
	>: std::true_type {};
template <class T>
inline constexpr const bool is_inequality_comparable_v = is_inequality_comparable<T>::value;


} /* namespace detail */


template <class Hash, class Compare>
struct AnyValue;

template <class Value, class Hash, class Compare>
struct TypedValue;

template <class Value, class Hash, class Compare>
struct AnyValueLink;

template <class Hash, class Compare>
class AnyList;

/**
 * @brief Check if @p any_v contains an object of type T.
 *
 * @param any_v - The AnyValue instance whose contained type is to be queried.
 * 
 * @return true if the AnyValue instance contains an object of type T, false otherwise.
 */
template <class T, class H, class C>
bool is(const AnyValue<H, C>& any_v);

/**
 * @brief Get a pointer to the contained object of type T.  If the contained object is not
 *        an instance of type T, returns nullptr.
 *
 * @param any_v - The AnyValue instance holding the value needed.
 * 
 * @return A pointer to the contained T the AnyValue instance contains an object of type T, false otherwise.
 */
template <class T, class H, class C>
const T* try_as(const AnyValue<H, C>& self) noexcept;

template <class T, class H, class C>
const T& as(const AnyValue<H, C>& self);

struct CopyConstructionError:
	std::logic_error
{
	CopyConstructionError(const std::type_info& ti):
		std::logic_error("Attempt to copy construct type that is not copy constructible."), 
		typeinfo(ti)
	{
		
	}

	const std::type_info& typeinfo;
};

template <class T, class H, class C, class ... Args>
std::unique_ptr<TypedValue<T, H, C>> make_typed_value(std::size_t hash_v, Args&& ... args);

template <class T, class H, class C, class ... Args>
std::unique_ptr<TypedValue<T, H, C>> make_typed_value(H hash_fn, Args&& ... args);

template <class T, class H, class C, class ... Args>
std::unique_ptr<AnyValue<H, C>> make_any_value(Args&& ... args);


template <class Hash, class Compare>
struct AnyValue
{
	using self_type = AnyValue<Hash, Compare>;
	virtual ~AnyValue() = default;

	AnyValue(const self_type&) = delete;
	AnyValue(self_type&&) = delete;


	template <class T>
	friend bool operator==(const self_type& left, const T& right)
	{
		if(const T* p = try_as<T>(left); static_cast<bool>(p))
			return *p == right; 
		return false;
	}
	
	template <class T>
	friend bool operator==(const T& left, const self_type& right)
	{
		if(const T* p = try_as<T>(right); static_cast<bool>(p))
			return left == *p; 
		return false;
	}

	template <class T>
	friend bool operator!=(const T& left, const self_type& right)
	{
		if(const T* p = try_as<T>(right); static_cast<bool>(p))
			return left != *p; 
		return true;
	}

	template <class T>
	friend bool operator!=(const self_type& left, const T& right)
	{
		if(const T* p = try_as<T>(left); static_cast<bool>(p))
			return *p != right; 
		return true;
	}

	friend bool operator==(const self_type& left, const self_type& right)
	{ return left.equals(right); }

	friend bool operator!=(const self_type& left, const self_type& right)
	{ return left.not_equals(right); }

	friend bool compare(const self_type& left, const self_type& right, Compare comp)
	{ return left.compare_to(right, comp); }

	template <class T, class Comp = Compare, class = std::enable_if_t<not std::is_same_v<T, self_type>>>
	friend bool compare(const T& left, const self_type& right, Comp comp)
	{
		if(const T* p = try_as<T>(right); static_cast<bool>(p))
			return comp(left, *p);
		return false;
	}

	template <class T, class Comp = Compare, class = std::enable_if_t<not std::is_same_v<T, self_type>>>
	friend bool compare(const self_type& left, const T& right, Comp comp)
	{
		if(const T* p = try_as<T>(left); static_cast<bool>(p))
			return comp(*p, right);
		return false;
	}

	friend std::ostream& operator<<(std::ostream& os, const self_type& any_v)
	{
		any_v.write(os);
		return os;
	}

	virtual const std::type_info& typeinfo() const = 0;

protected:
	AnyValue(std::size_t hash_v):
		hash(hash_v)
	{
		
	}

	virtual bool compare_to(const self_type& other, Compare comp) const = 0;
	virtual bool equals(const self_type& other) const = 0;
	virtual bool not_equals(const self_type& other) const = 0;
	virtual void write(std::ostream& os) const = 0;
	virtual std::unique_ptr<self_type> clone() const = 0;

public:
	const std::size_t hash;
private:
	self_type* next{nullptr};
	template <class, class, class>	
	friend class AnySet;
	friend class AnyList<Hash, Compare>;
};

template <class Value, class Hash, class Compare>
struct AnyValueLink;

template <class Value, class Hash, class Compare>
struct TypedValue:
	public AnyValue<Hash, Compare>
{
	using base_type = AnyValue<Hash, Compare>;
	using value_type = Value;
	using holder_type = ConstValueHolder<value_type>;
	using self_type = TypedValue<Value, Hash, Compare>;
	using link_type = AnyValueLink<Value, Hash, Compare>;

	TypedValue(const self_type&) = delete;
	TypedValue(self_type&&) = delete;
	self_type& operator=(const self_type&) = delete;
	self_type& operator=(self_type&&) = delete;
protected:
	TypedValue(std::size_t hash_v):
		base_type(hash_v)
	{
		
	}
public:
	
	virtual ~TypedValue() override = default;
	
	bool equals(const base_type& other) const final override
	{
		if constexpr(detail::is_equality_comparable_v<Value>)
			if(const Value* v = try_as<Value>(other); static_cast<bool>(v))
				return (*v) == this->value();
			else
				return false;
		else
			return false;
	}
	
	bool not_equals(const base_type& other) const final override
	{
		if constexpr(detail::is_inequality_comparable_v<Value>)
			if(const Value* v = try_as<Value>(other); static_cast<bool>(v))
				return (*v) != this->value();
			else
				return false;
		else
			return true;
	}

	bool compare_to(const base_type& other, Compare comp) const final override
	{
		auto* p = try_as<Value>(other);
		if constexpr(not std::is_same_v<Value, AnyListSentinalType>)
			return static_cast<bool>(p) and comp(*p, this->value()); 
		else
			return static_cast<bool>(p);
	}

	void write(std::ostream& os) const final override
	{
		if constexpr(detail::is_streamable_v<Value>)
			os << this->value();
		else
			os << "AnyValue(typeid.name = '" << typeinfo().name() << "', hash = " << this->hash << ')';
	}

	std::unique_ptr<base_type> clone() const final override
	{
		if constexpr(std::is_copy_constructible_v<Value>)
			return make_any_value<Value, Hash, Compare>(this->hash, this->value());
		else
			throw CopyConstructionError(typeinfo());
	}

	const std::type_info& typeinfo() const final override
	{ return typeid(Value); }

	const value_type& value() const&
	{ return get_value(as_link()); } 

	const value_type&& value() const&&
	{ return std::move(get_value(as_link())); } 

private:
	const link_type& as_link() const& noexcept
	{ return static_cast<const link_type&>(*this); }
		
	const link_type&& as_link() const&& noexcept
	{ return static_cast<const link_type&&>(*this); }

	friend const Value* try_as<Value>(const AnyValue<Hash, Compare>& self) noexcept;
};

template <class Value, class Hash, class Compare>
struct AnyValueLink final:
	public ConstValueHolder<Value>,
	public TypedValue<Value, Hash, Compare>
{
	friend class TypedValue<Value, Hash, Compare>;
	using self_type = AnyValueLink<Value, Hash, Compare>;
	using holder_type = ConstValueHolder<Value>;
	using base_type = TypedValue<Value, Hash, Compare>;

	template <class ... Args>
	AnyValueLink(std::size_t hash_v, Args&& ... args):
		holder_type(std::forward<Args>(args)...),
		base_type(hash_v)
	{
		
	}
public:	
	template <class ... Args>
	AnyValueLink(Hash hasher, Args&& ... args):
		holder_type(std::forward<Args>(args)...),
		base_type(hasher(get_value(*this)))
	{
		
	}

	friend const Value& get_value(const self_type& self)
	{ return get_value(static_cast<const holder_type&>(self)); }

	friend const Value&& get_value(const self_type&& self)
	{ return get_value(static_cast<const holder_type&&>(self)); }

	template <class T, class H, class C, class ... Args>
	friend std::unique_ptr<TypedValue<T, H, C>> make_typed_value(std::size_t, Args&& ...);
	
	template <class T, class H, class C, class ... Args>
	friend std::unique_ptr<TypedValue<T, H, C>> make_typed_value(H, Args&& ...);

	template <class To, class H, class C>
	friend To polymorphic_cast(const AnyValue<H, C>& any_v);

	template <class To, class H, class C>
	friend To polymorphic_cast(const AnyValue<H, C>* any_v);

};

template <class T, class H, class C, class ... Args>
std::unique_ptr<TypedValue<T, H, C>> make_typed_value(std::size_t hash_v, Args&& ... args)
{
	auto tmp = std::make_unique<AnyValueLink<T, H, C>>(hash_v, std::forward<Args>(args) ...);
	return std::unique_ptr<TypedValue<T, H, C>>(
		static_cast<TypedValue<T, H, C>*>(tmp.release())
	);
}

template <class T, class H, class C, class ... Args>
std::unique_ptr<TypedValue<T, H, C>> make_typed_value(H hash_fn, Args&& ... args)
{
	auto tmp = std::make_unique<AnyValueLink<T, H, C>>(hash_fn, std::forward<Args>(args) ...);
	return std::unique_ptr<TypedValue<T, H, C>>(
		static_cast<TypedValue<T, H, C>*>(tmp.release())
	);
}

template <class T, class H, class C, class ... Args>
std::unique_ptr<AnyValue<H, C>> make_any_value(Args&& ... args)
{
	auto tmp = make_typed_value<T, H, C>(std::forward<Args>(args)...);
	return std::unique_ptr<AnyValue<H, C>>(
		static_cast<AnyValue<H, C>*>(tmp.release())
	);
}

template <
	class To,
	class H,
	class C
>
To polymorphic_cast(const AnyValue<H, C>& any_v)
{
	using type = std::decay_t<To>;
	static_assert(std::is_class_v<type> and not std::is_final_v<type>, 
		"Cannot 'polymorphic_cast()' to final or non-class type."
	);
	return dynamic_cast<To>(any_v);
}

template <
	class To,
	class H,
	class C
>
To polymorphic_cast(const AnyValue<H, C>* any_v)
{
	using type = std::decay_t<std::remove_pointer_t<std::decay_t<To>>>;
	static_assert(std::is_class_v<type> and not std::is_final_v<type>, 
		"Cannot 'polymorphic_cast()' to final or non-class type."
	);
	return dynamic_cast<To>(any_v);
}

template <class To, class H, class C>
To exact_cast(const AnyValue<H, C>& any_v)
{
	using to_type = std::decay_t<To>;
	if(is<to_type>(any_v))
		return static_cast<const TypedValue<to_type, H, C>&>(any_v).value();
	else
		throw std::bad_cast();
}

template <class To, class H, class C>
To exact_cast(const AnyValue<H, C>* any_v) noexcept
{
	using to_type = std::remove_pointer_t<std::decay_t<To>>;
	static_assert(
		std::is_pointer_v<std::decay_t<To>>, 
		"exact_cast<T>(const AnyValue<H, C>*) requires that T be a pointer type."
	);
	if(is<to_type>(*any_v))
		return std::addressof(
			static_cast<const TypedValue<to_type, H, C>&>(*any_v).value()
		);
	else
		return nullptr;
}

template <class To, class H, class C>
To unsafe_cast(const AnyValue<H, C>& any_v) 
{
	return static_cast<const TypedValue<std::decay_t<To>, H, C>&>(any_v).value();
}

template <class H, class C>
std::size_t compute_hash(const AnyValue<H, C>& any_v)
{ return any_v.hash; }

template <class T, class H, class C>
bool is(const AnyValue<H, C>& any_v)
{ return any_v.typeinfo() == typeid(T); }

template <class T, class H, class C>
const T* try_as(const AnyValue<H, C>& any_v) noexcept
{
	using type = std::decay_t<std::remove_pointer_t<std::decay_t<T>>>;
	if(const T* p = exact_cast<const T*>(&any_v); static_cast<bool>(p))
		return p;
	else if constexpr(std::is_class_v<type> and not std::is_final_v<type>)
	{
		if(p = polymorphic_cast<const T*>(&any_v); static_cast<bool>(p))
			return p;
	}
	return nullptr;
}

template <class T, class H, class C>
const T& as(const AnyValue<H, C>& self)
{
	if(const auto* p = try_as<T>(self); static_cast<bool>(p))
		return *p;
	throw std::bad_cast();
}

} /* namespace te */	

#endif /* ANY_NODE_H */

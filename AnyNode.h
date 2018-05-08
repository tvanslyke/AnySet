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
		decltype(std::declval<T>() != std::declval<T>(), (void)0)
		>
	>: std::true_type {};
template <class T>
inline constexpr const bool is_inequality_comparable_v = is_inequality_comparable<T>::value;


} /* namespace detail */

template <class Hash, class Compare>
struct AnyValue;

template <class T, class H, class C>
bool is(const AnyValue<H, C>&);

template <class Value, class Hash, class Compare>
struct AnyNode;

template <class Hash, class Compare>
struct AnyNodeBase;
	
template <class Hash, class Compare>
class AnyList;

template <class Hash, class Compare>
struct AnyValue;

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

template <class Hash, class Compare>
struct AnyValue
{
	using self_type = AnyValue<Hash, Compare>;
	virtual ~AnyValue() = 0; 

	AnyValue(const self_type&) = delete;
	AnyValue(self_type&&) = delete;

	const std::type_info& typeinfo() const
	{ return get_typeinfo(); }


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

	friend std::ostream& operator<<(std::ostream& os, const self_type& any_v)
	{
		any_v.write(os);
		return os;
	}


	// std::size_t get_hash() const noexcept
	// { return static_cast<const AnyNodeBase<Hash, Compare>*>(this)->hash; }

	const std::size_t hash;
private:
	AnyNodeBase<Hash, Compare>* const& get_next() const noexcept
	{ return static_cast<const AnyNodeBase<Hash, Compare>*>(this)->next; }

	AnyNodeBase<Hash, Compare>*& get_next() noexcept
	{ return static_cast<AnyNodeBase<Hash, Compare>*>(this)->next; }

protected:
	AnyValue(std::size_t hash_v):
		hash(hash_v)
	{
		
	}
	virtual const std::type_info& get_typeinfo() const = 0;
	virtual bool compare_to(const self_type& other, Compare comp) const = 0;
	virtual bool equals(const self_type& other) const = 0;
	virtual bool not_equals(const self_type& other) const = 0;
	virtual void write(std::ostream& os) const = 0;
	virtual std::unique_ptr<self_type> clone() const = 0;

	template <class, class, class>	
	friend class AnySet;
	friend class AnyList<Hash, Compare>;
};

template <class H, class C>
std::size_t compute_hash(const AnyValue<H, C>& any_v)
{ return any_v.hash; }
// { return any_v.get_hash(); }

template <class H, class C>
AnyValue<H, C>::~AnyValue() = default;

template <class T, class H, class C>
bool is(const AnyValue<H, C>& any_v)
{ return any_v.typeinfo() == typeid(T); }

template <class Hash, class Compare>
struct AnyNodeBase:
	protected AnyValue<Hash, Compare>
{
	using any_type = AnyValue<Hash, Compare>;
	using self_type = AnyNodeBase<Hash, Compare>;

	AnyNodeBase(std::size_t hash_v): 
		any_type(hash_v), next(nullptr) 
	{
		
	}

	virtual ~AnyNodeBase() = default;

private:
	friend class AnyList<Hash, Compare>;
	friend class AnyValue<Hash, Compare>;
	template <class, class, class>
	friend class AnySet;
	self_type* next;
};


namespace detail {
template <class Value, bool = std::is_empty_v<Value>>
struct ValueHolder;
	
template <class Value>
struct ValueHolder<Value, true>: Value
{
	using Value::Value;
	template <class ... T>
	ValueHolder(T&& ... args):
		Value(std::forward<T>(args)...)
	{
		
	}
	const Value& value() const &
	{ return *static_cast<const Value*>(this); }

	const Value&& value() const &&
	{ return std::move(*static_cast<const Value*>(this)); }

	Value& value() &
	{ return *static_cast<Value*>(this); }

	Value&& value() &&
	{ return std::move(*static_cast<Value*>(this)); }
};

template <class Value>
struct ValueHolder<Value, false>
{
	template <class ... T>
	ValueHolder(T&& ... args):
		value_(std::forward<T>(args)...)
	{
		
	}

	const Value& value() const &
	{ return value_; }

	const Value& value() const &&
	{ return value_; }

	Value& value() &
	{ return value_; }

	Value&& value() &&
	{ return value_; }

private:
	Value value_;
};
	
} /* namespace detail */ 

template <class Value, class Hash, class Compare>
struct AnyNode final:
	public detail::ValueHolder<const Value>,
	public AnyNodeBase<Hash, Compare>
{
	using base_type = AnyNodeBase<Hash, Compare>;
	using value_type = Value;
	using holder_type = detail::ValueHolder<const value_type>;
	using any_type = typename base_type::any_type;
	using self_type = AnyNode<Value, Hash, Compare>;

	template <class ... Args>
	AnyNode(std::size_t hash_v, Args&& ... args):
		holder_type(std::forward<Args>(args)...), base_type(hash_v)
	{
		
	}
	
	template <class ... Args>
	AnyNode(Hash hasher, Args&& ... args):
		holder_type(std::forward<Args>(args)...),
		base_type(hasher(this->value()))
	{
		assert(hasher(this->value()) == this->hash);
	}
	
	virtual ~AnyNode() final override = default;
	
	bool equals(const any_type& other) const final override
	{
		if constexpr(detail::is_equality_comparable_v<Value>)
		{
			const Value* v = try_as<Value>(other); 
			return static_cast<bool>(v) and ((*v) == this->value());
		}
		else
		{
			return true;
		}
	}
	
	bool not_equals(const any_type& other) const final override
	{
		if constexpr(detail::is_equality_comparable_v<Value>)
		{
			const Value* v = try_as<Value>(other);
			return (not static_cast<bool>(v)) or ((*v) != this->value());
		}
		else
		{
			return true;
		}
	}

	bool compare_to(const any_type& other, Compare comp) const final override
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
			os << "AnyValue(typeid.name = '" << typeid(Value).name() << "', hash = " << this->hash << ')';
	}

	std::unique_ptr<any_type> clone() const final override
	{
		if constexpr(std::is_copy_constructible_v<Value>)
		{
			return std::unique_ptr<any_type>(
				static_cast<any_type*>(std::make_unique<self_type>(this->hash, this->value()).release())
			);
		}
		else
		{
			throw CopyConstructionError(get_typeinfo());
		}
	}

	const std::type_info& get_typeinfo() const final override
	{ return typeid(Value); }

	friend const Value* try_as<Value>(const AnyValue<Hash, Compare>& self) noexcept;
};

template <class T, class H, class C>
const T* try_as(const AnyValue<H, C>& self) noexcept
{
	if(is<T>(self))
		return std::addressof(static_cast<const AnyNode<T, H, C>*>(&self)->value());
	else
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

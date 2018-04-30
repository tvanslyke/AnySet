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

namespace detail {
template <class S, class T>
class is_streamable_helper
{
	template<class Stream, class Type>
	static auto test(int) -> decltype( 
		std::declval<Stream&>() << std::declval<Type>(), std::true_type() 
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
		decltype(std::declval<T>() == std::declval<T>(), (void)0)
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

template <class Value>
struct AnyNode;

struct AnyNodeBase;

struct AnyValue
{
	virtual ~AnyValue() = 0; 

	AnyValue(const AnyValue&) = delete;
	AnyValue(AnyValue&&) = delete;

	const std::type_info& typeinfo() const
	{ return get_typeinfo(); }

	template <class T>
	const T* try_as() const noexcept;

	template <class T>
	const T& as() const;

	template <class T>
	explicit operator T() const;

	friend bool operator==(const AnyValue& left, const AnyValue& right)
	{ return left.equals(right); }

	friend bool operator!=(const AnyValue& left, const AnyValue& right)
	{ return left.not_equals(right); }

	friend std::ostream& operator<<(std::ostream& os, const AnyValue& any_v)
	{
		any_v.write(os);
		return os;
	}

protected:
	
	AnyValue() = default;
	std::size_t get_hash() const noexcept;
	AnyNodeBase* const& get_next() const noexcept;
	AnyNodeBase*& get_next() noexcept;
	virtual const std::type_info& get_typeinfo() const = 0;
	virtual bool equals(const AnyValue& other) const = 0;
	virtual bool not_equals(const AnyValue& other) const = 0;
	virtual void write(std::ostream& os) const = 0;

	template <class, class, class>	
	friend class AnySet;
	friend class AnyList;
};

AnyValue::~AnyValue() = default;

template <class T>
bool is(const AnyValue& any_v)
{ return any_v.typeinfo() == typeid(T); }


struct AnyNodeBase:
	protected AnyValue
{

	AnyNodeBase(std::size_t hash_v): 
		AnyValue(), hash(hash_v), next(nullptr) 
	{
		
	}

	virtual ~AnyNodeBase() = default;

	template <class Key, class Comp>
	bool matches(const Key& key, std::size_t hash_value, Comp comp) const;

	const std::size_t hash;
private:
	friend class AnyList;
	friend class AnyValue;
	AnyNodeBase* next;
};



template <class Value>
struct AnyNode final:
	public AnyNodeBase
{
	
	template <class ... Args>
	AnyNode(std::size_t hash_v, Args&& ... args):
		AnyNodeBase(hash_v), value(std::forward<Args>(args)...)
	{
		
	}
	
	bool equals(const AnyValue& other) const final override
	{
		if constexpr(detail::is_equality_comparable_v<Value>)
		{
			const Value* v = other.try_as<Value>(); 
			return static_cast<bool>(v) and ((*v) == value);
		}
		else
		{
			return true;
		}
	}
	
	bool not_equals(const AnyValue& other) const final override
	{
		if constexpr(detail::is_equality_comparable_v<Value>)
		{
			const Value* v = other.try_as<Value>(); 
			return (not static_cast<bool>(v)) or ((*v) != value);
		}
		else
		{
			return true;
		}
	}

	void write(std::ostream& os) const final override
	{
		if constexpr(detail::is_streamable_v<Value>)
		{
			os << value;
		}
		else
		{
			os << "<instance of non-streamable type: typeid.name = '" 
				<< typeid(Value).name() << "', hash = " << hash << '>';
		}
	}

	virtual ~AnyNode() final override = default;

	const std::type_info& get_typeinfo() const final override
	{ return typeid(Value); }

	// TODO: EBO for Value?
	const Value value;
};

std::size_t AnyValue::get_hash() const noexcept
{ return static_cast<const AnyNodeBase*>(this)->hash; }

AnyNodeBase* const& AnyValue::get_next() const noexcept
{ return static_cast<const AnyNodeBase*>(this)->next; }

AnyNodeBase*& AnyValue::get_next() noexcept
{ return static_cast<AnyNodeBase*>(this)->next; }

template <class T>
const T* AnyValue::try_as() const noexcept
{
	if(is<T>(*this))
		return std::addressof(static_cast<const AnyNode<T>*>(this)->value);
	else
		return nullptr;
}

template <class T>
const T& AnyValue::as() const
{ return dynamic_cast<const AnyNode<T>&>(*this).value; }

template <class T>
AnyValue::operator T() const
{ return as<T>(); }


template <class Value, class Comp>
bool AnyNodeBase::matches(const Value& value, std::size_t hash_value, Comp comp) const
{
	if(hash_value != hash)
		return false;
	const auto* stored_value = try_as<Value>();
	if(not stored_value)
		return false;
	return comp(stored_value, value);
}

namespace detail {

	
	template <class Visitor, class ... T>
	using visit_result_t = std::common_type_t<std::decay_t<std::invoke_result_t<Visitor, T>>...>;

	template <class Visitor, class ... T>
	inline constexpr bool visit_returns_void_v = std::conjunction_v<
		std::is_same<std::invoke_result_t<Visitor, T>, void> ...
	>;

	template <class Visitor, class ... T>
	inline constexpr bool visit_inconsistently_returns_void_v = (not visit_returns_void_v<Visitor, T...>)
		and std::disjunction_v<std::is_same<std::invoke_result_t<Visitor, T>, void> ...>;	

	template <class Visitor, class ... T>
	struct visit_result_type;

} /* namespace detail */

template <class T, class ... U, class Visitor, class = std::enable_if_t<not detail::visit_returns_void_v<Visitor, T, U...>>>
std::optional<detail::visit_result_t<Visitor, T, U...>> visit(Visitor visitor, const AnyValue& any_v) 
{
	if(const auto* v = any_v.try_as<T>(); v)
		return visitor(*v);
	
	if constexpr(sizeof...(U) > 0u)
		return visit<U...>(visitor, any_v);
	else
		return std::nullopt;
}

template <class T, class ... U, class Visitor, class = std::enable_if_t<detail::visit_returns_void_v<Visitor, T, U...>>>
bool visit(Visitor visitor, const AnyValue& any_v) 
{
	if(const auto* v = any_v.try_as<T>(); v)
	{
		visitor(*v);
		return true;
	}
	
	if constexpr(sizeof...(U) > 0u)
	{
		return visit<U...>(visitor, any_v);
	}
	else
	{
		return false;
	}
}

template <class T, class ... U, class Visitor, class = std::enable_if_t<detail::visit_inconsistently_returns_void_v<Visitor, T, U...>>>
void visit(Visitor visitor, const AnyValue& any_v) 
{
	static_assert(
		not detail::visit_inconsistently_returns_void_v<Visitor, T, U...>, 
		"Visitor incosistently returns void."
	);
}

template <class T>
const T& get(const AnyValue& any_v)
{ return any_v.as<T>(); }

template <class T>
const T* try_get(const AnyValue& any_v)
{ return any_v.try_as<T>(); }


template <class ... T>
std::variant<std::monostate, std::reference_wrapper<const T>...> get_one_of(const AnyValue& any_v)
{
	std::variant<std::monostate, std::reference_wrapper<const T>...> result;
	visit([&](const auto& v) -> void { result = v; }, any_v);
	return result;
}

template <class ... T>
std::variant<std::monostate, T...> get_one_of_v(const AnyValue& any_v)
{
	static_assert(
		(std::is_same_v<T, std::decay_t<T>> and ...), 
		"Use of get_one_of_v() is a only permitted with fully std::decay'd (non-reference, non-const, etc) types."
	);
	std::variant<std::monostate, std::reference_wrapper<const T>...> result;
	visit([&](const auto& v) -> void { result = v; }, any_v);
	return result;
}



#endif /* ANY_NODE_H */

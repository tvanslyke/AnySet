#ifndef ANY_NODE_H
#define ANY_NODE_H

#include <typeinfo>
#include <cassert>
#include <type_traits>
#include <ostream>
#include <memory>
#include "ValueHolder.h"

namespace te {

/* Forward declarations. */

template <class H, class E, class A>
struct AnySet;

template <class H, class E, class A>
std::ostream& operator<<(std::ostream& os, const AnySet<H, E, A>& set);


/// @internal
namespace detail {

/* Some type traits. */

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

template <class Value, class HashFn, class Compare>
struct TypedValue;

template <class Value, class HashFn, class Compare>
struct AnyValueLink;

template <class HashFn, class Compare>
struct AnyList;

} /* namespace detail */
/// @endinternal

/* More forward declarations. */

template <class HashFn, class Compare>
struct AnyValue;

/**
 * @brief Check if @p any_v contains an object of type T.
 *
 * @param any_v - The AnyValue instance whose contained type is to be queried.
 * 
 * @return true if the AnyValue instance contains an object of type T, false otherwise.
 * 
 * @see AnyValue
 * @see exact_cast()
 * @see unsafe_cast()
 * 
 * @relates AnyValue
 */
template <class T, class H, class C>
bool is(const AnyValue<H, C>& any_v);

template <class T, class H, class C>
const T* try_as(const AnyValue<H, C>& self);

template <class T, class H, class C>
const T& as(const AnyValue<H, C>& self);


/**
 * @brief Base type of NoCopyConstructorError.  Thrown when attempting to make copies of an AnyValue 
 *        instance that contains a non-copy-constructible type.
 */
struct CopyConstructionError:
	std::logic_error
{
	/**
	 * @brief Construct a CopyConstructionError exception object.
	 * @param ti - Reference to a std::type_info object of the type that 
	 *             failed to be constructed.
	 */ 
	CopyConstructionError(const std::type_info& ti):
		std::logic_error("Attempt to copy construct type that is not copy constructible."), 
		typeinfo(ti)
	{
		
	}

	virtual const char* what() const noexcept override 
	{
		// make what() visible to doxygen
		return this->std::logic_error::what();
	}

	/** 
	 * Reference to a std::type_info object indicating the 
	 * contained type of the AnyValue instance.
	 */
	const std::type_info& typeinfo;
};

/**
 * @brief Exception thrown when attempting to make copies of an AnyValue instance that
 *        contains an instance of non-copyable type T.
 * 
 * This class is provided to allow users to catch CopyConstructionErrors for a specific,
 * known type.
 */
template <class T>
struct NoCopyConstructorError final:
	public CopyConstructionError
{
	static_assert(not std::is_copy_constructible_v<T>);

	NoCopyConstructorError():
		CopyConstructionError(typeid(T))
	{
		
	}
};

/// @internal
namespace detail {

template <class T, class H, class C, class ... Args>
std::unique_ptr<TypedValue<T, H, C>> make_typed_value(std::size_t hash_v, Args&& ... args);

template <class T, class H, class C, class ... Args>
std::unique_ptr<TypedValue<T, H, C>> make_typed_value(H hash_fn, Args&& ... args);

} /* namespace detail */
/// @endinternal

/**
 * @brief Constructs a std::unique_ptr<AnyValue<H, C>> suitable for splicing into
 *        an AnySet instance.
 * 
 * @tparam T - The type of the contained value.
 * @tparam H - The type of hash function object used by AnySet instances that 
 * @tparam C - The type of the contained value.
 * 
 * @param hash_value - The resultant hash code obtained from hashing the to-be-constructed 
 *                     @p T instance.
 * @param args       - arguments to forward to @p T's constructor.
 * @note @p hash_value is a "trust me" argument.  Only call this overload if you are
 *       absolutely sure of what the hash code of the constructed object will be.
 *       Use of this overload is appropriate, for example, when move-constructing a 
 *       long std::string whose hash has already been computed.
 *       
 */
template <class T, class H, class C, class ... Args>
std::unique_ptr<AnyValue<H, C>> make_any_value(std::size_t hash_value, Args&& ... args);

/**
 * @brief Constructs a std::unique_ptr<AnyValue<H, C>> suitable for splicing into
 *        an @p AnySet<@p H, @p C> instance.
 * 
 * @tparam T - The type of the contained value.
 * @tparam H - The type of hash function object used by AnySet instances that 
 * @tparam C - The type of the contained value.
 * 
 * @param hasher - The instance of @p H to use to compute the hash of contained @p T
 *                 object after it is constructed in the AnyValue.
 * @param args   - Arguments to forward to @p T's constructor.
 *
 * @relates AnyValue.
 */
template <class T, class H, class C, class ... Args>
std::unique_ptr<AnyValue<H, C>> make_any_value(H hasher, Args&& ... args);

/**
 * @class AnyValue
 * @brief A @p std::any -like type that serves as the @p value_type for AnySet instances.
 * 
 * AnyValue instances are always dynamically-allocated and are typically owned by an AnySet 
 * instance.  AnyValue has no user-visible constructors and is neither copy nor move-assignable.
 * 
 * AnyValue instances can be spliced into and extracted from AnySet instances whose @p HashFn
 * and @p KeyEqual types are the same as @p %HashFn and @p Compare respectively.
 *
 * Following the conventions of @p std::any, the value contained in a particular AnyValue 
 * instance can be obtained using special 'cast' functions.  The cast functions provided for
 * AnyValue are exact_cast(), polymorphic_cast(), and unsafe_cast().  Additionally, non-cast-style
 * accessor and query functions is(), as(), and try_as() are provided as well.
 *
 *
 * @tparam HashFn    - The type of the hash function object used in the corresponding 
 *                   @p AnySet<@p HashFn, @p Compare> instances.
 * @tparam Compare - The type of the equality function object (@p KeyEqual) used in the 
 *                   corresponding @p AnySet<@p HashFn, @p Compare> instances.
 *
 * @see AnySet
 * @see AnyHash
 *
 * @ingroup AnySet-Module
 */
template <class HashFn, class Compare>
struct AnyValue
{
	using self_type = AnyValue<HashFn, Compare>;
	using hasher = HashFn;
	using key_equal = Compare;
	virtual ~AnyValue() = default;

	AnyValue(const AnyValue&) = delete;
	AnyValue(AnyValue&&) = delete;

	/// @name Comparison Operations
	/// @{

	/// Query whether the contained object in @p left is of type @p T and equal to @p right.
	template <class T>
	friend bool operator==(const AnyValue& left, const T& right)
	{
		if(const T* p = try_as<T>(left); static_cast<bool>(p))
			return *p == right; 
		return false;
	}
	
	/// Query whether the contained object in @p right is of type @p T and equal to @p left.
	template <class T>
	friend bool operator==(const T& left, const AnyValue& right)
	{
		if(const T* p = try_as<T>(right); static_cast<bool>(p))
			return left == *p; 
		return false;
	}

	/// Query whether the contained object in @p right is not of type @p T or not equal to @p left.
	template <class T>
	friend bool operator!=(const T& left, const AnyValue& right)
	{
		if(const T* p = try_as<T>(right); static_cast<bool>(p))
			return left != *p; 
		return true;
	}

	/// Query whether the contained object in @p left is not of type @p T or not equal to @p right.
	template <class T>
	friend bool operator!=(const AnyValue& left, const T& right)
	{
		if(const T* p = try_as<T>(left); static_cast<bool>(p))
			return *p != right; 
		return true;
	}

	/**
	 * @brief Query whether the contained objects in @p left and @p right are of
	 *        the same equality-comparable type, and subsequently whether the contained,
	 *        same-type objects compare equal using operator==.
	 * 	@note If either of the contained objects are of non-equality-comparable 
	 * 	      types, this function returns @p false.  This is the case even if the contained 
	 * 	      objects are of the same, empty type.
	 *
	 * @return true if the contained types are identical, the contained 
	 *         type is equality-comparable, and if the contained same-type
	 *         objects compare equal via operator==.
	 */ 
	friend bool operator==(const AnyValue& left, const AnyValue& right)
	{ return left.equals(right); }

	/**
	 * @brief Query whether the contained objects in @p left and @p right are of
	 *        different types, or whether the contained same-type objects compare 
	 *        not-equal using operator!=.
	 * 	@note If either of the contained objects are of non-inequality-comparable 
	 * 	      types, this function returns @p true.  This is the case even if the 
	 * 	      contained objects are of the same, empty type.
	 *
	 * @return true if the contained types are not identical, if the contained 
	 *         types are not inequality-comparable, or if the contained same-type
	 *         objects compare not-equal via operator!=.
	 */ 
	friend bool operator!=(const AnyValue& left, const AnyValue& right)
	{ return left.not_equals(right); }

	/**
	 * @brief Query whether the contained objects in @p left and @p right are of
	 *        the same type, and subsequently whether the contained same-type objects 
	 *        compare equal using the provided instance of the @p Compare comparison
	 *        function object.
	 *
	 * @param left  - An instance of @p te::AnyValue<@p HashFn, @p Compare>.
	 * @param right - An instance of @p te::AnyValue<@p HashFn, @p Compare>.
	 * @param comp  - The instance of @p Compare to use for comparing the 
	 *                contained objects.
	 *
	 * @return true if the contained types are identical and compare equal when @p comp
	 *         is invoked with the contained objects. 
	 */ 
	friend bool compare(const AnyValue& left, const AnyValue& right, Compare comp)
	{ return left.compare_to(right, comp); }

	/**
	 * @brief Query whether the contained object in @p right is of type @p T and
	 *        subsequently whether the contained object compares equal to @p left 
	 *        using the provided instance of the @p Comp comparison function object.
	 *
	 * @param left  - An instance of type @p T.
	 * @param right - An instance of @p te::AnyValue<@p HashFn, @p Compare>.
	 * @param comp  - An instance of an arbitrary comparison function type @p Comp 
	 *                to use for comparing the contained objects.
	 *
	 * @return true if the contained types are identical and compare equal when @p comp
	 *         is invoked with the contained objects. 
	 */ 
	template <class T, class Comp = Compare, class = std::enable_if_t<not std::is_same_v<T, self_type>>>
	friend bool compare(const T& left, const AnyValue& right, Comp comp)
	{
		if(const T* p = try_as<T>(right); static_cast<bool>(p))
			return comp(left, *p);
		return false;
	}

	/**
	 * @brief Query whether the contained object in @p left is of type @p T and
	 *        subsequently whether the contained object compares equal to @p right 
	 *        using the provided instance of the @p Comp comparison function object.
	 *
	 * @param left  - An instance of @p te::AnyValue<@p HashFn, @p Compare>.
	 * @param right - An instance of type @p T.
	 * @param comp  - An instance of an arbitrary comparison function type @p Comp 
	 *                to use for comparing the contained objects.
	 *
	 * @return true if the contained types are identical and compare equal when @p comp
	 *         is invoked with the contained objects. 
	 */ 
	template <class T, class Comp = Compare, class = std::enable_if_t<not std::is_same_v<T, self_type>>>
	friend bool compare(const AnyValue& left, const T& right, Comp comp)
	{
		if(const T* p = try_as<T>(left); static_cast<bool>(p))
			return comp(*p, right);
		return false;
	}

	/// @} Comparison Operations


	/**
	 * @brief Write the contained object to the given @p std::ostream using the
	 *        stream insertion << operator.
	 * @note If the contained object is of a type that is not streamable (has no
	 *       @p operator<<(@p std::ostream, @p const @p T&) overload), writes a
	 *       string of the form "AnyValue(typeid.name='<typeid(T).name()>', hash=<hash code>)".
	 */
	friend std::ostream& operator<<(std::ostream& os, const AnyValue& any_v)
	{
		any_v.write(os);
		return os;
	}

	/**
	 * @brief Get a reference to a std::type_info object that indicates the 
	 *        type of the contained object.
	 * 
	 * @return A std::type_info object that indicates the type of the contained object.
	 */ 
	virtual const std::type_info& typeinfo() const = 0;

protected:
	AnyValue(std::size_t hash_v):
		hash(hash_v)
	{
		
	}

	virtual bool compare_to(const AnyValue& other, Compare comp) const = 0;
	virtual bool equals(const AnyValue& other) const = 0;
	virtual bool not_equals(const AnyValue& other) const = 0;
	virtual void write(std::ostream& os) const = 0;
	virtual std::unique_ptr<self_type> clone() const = 0;

public:
	/**
	 *  The hash code obtained by invoking an instance of @p HashFn on the contained object.
	 */
	const std::size_t hash;
private:
	self_type* next{nullptr};
	template <class, class, class>	
	friend struct AnySet;
	friend struct detail::AnyList<HashFn, Compare>;

};

/// @internal
namespace detail {

template <class Value, class HashFn, class Compare>
struct AnyValueLink;

/**
 * @brief This class is an implementation detail and is not part of the public interface
 * of AnyValue.
 *
 * TypedValue introduces the static type of the contained object into the inheritence 
 * heirarchy but does not, itself contain the object.  AnyValueLink implements accessors
 * to the contained object through overloads of a non-member function get_value().  TypedValue
 * knows this statically, and knows that all TypedValue instances are really part of a larger
 * AnyValueLink instance.  Therefore, we implement AnyValue's virtual member functions at 
 * this point in the heirarchy so that we do not override any member functions of the contained
 * objects, which might be inherited from in AnyValueLink.
 *
 * @tparam Value - The type of the contained value.
 */
template <class Value, class HashFn, class Compare>
struct TypedValue:
	public AnyValue<HashFn, Compare>
{
	using base_type = AnyValue<HashFn, Compare>;
	using value_type = Value;
	using holder_type = ConstValueHolder<value_type>;
	using self_type = TypedValue<Value, HashFn, Compare>;
	using link_type = AnyValueLink<Value, HashFn, Compare>;

	virtual ~TypedValue() = default;

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
		return static_cast<bool>(p) and comp(*p, this->value()); 
	}

	void write(std::ostream& os) const final override
	{
		if constexpr(detail::is_streamable_v<Value>)
			os << this->value();
		else
			os << "AnyValue(typeid.name='" << typeinfo().name() << "', hash=" << this->hash << ')';
	}

	std::unique_ptr<base_type> clone() const final override
	{
		if constexpr(std::is_copy_constructible_v<Value>)
			return make_any_value<Value, HashFn, Compare>(this->hash, this->value());
		else
			throw NoCopyConstructorError<Value>();
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

	friend const Value* try_as<Value>(const AnyValue<HashFn, Compare>& self);
};

/**
 * @brief This class is an implementation detail and is not part of the public interface
 * of AnyValue.
 *
 * AnyValueLink 'links' the AnyValue instance with its contained object by inheriting from
 * ConstValueHolder<Value>.  ConstValueHolder conditionally inherits from 'Value', depending
 * on whether 'Value' is a non-final class type.  The inheritence heirarchy looks like this:
@verbatim
       AnyValue            Value
          ^                  ^
           \                /     <<< This part of the inheritence tree is conditional
        TypedValue  ConstValueHolder
             ^            ^
              \          /
              AnyValueLink (final)
@endverbatim  
 * This allows us to implement polymorphic_cast() with a simple dynamic_cast.  Since AnyValue
 * introduces RTTI into the heirarchy, polymorphic_cast() can cast through @p Value's public 
 * inheritance heirarchy without knowing what the actual type of Value is, in spite of the fact 
 * that Value may not even be a polymorphic type itself.  
 */
template <class Value, class HashFn, class Compare>
struct AnyValueLink final:
	public ConstValueHolder<Value>,
	public TypedValue<Value, HashFn, Compare>
{
	friend struct TypedValue<Value, HashFn, Compare>;
	using self_type = AnyValueLink<Value, HashFn, Compare>;
	using holder_type = ConstValueHolder<Value>;
	using base_type = TypedValue<Value, HashFn, Compare>;
public:
	virtual ~AnyValueLink() final = default;

	template <class ... Args>
	AnyValueLink(std::size_t hash_v, Args&& ... args):
		holder_type(std::forward<Args>(args)...),
		base_type(hash_v)
	{
		
	}

	template <class ... Args>
	AnyValueLink(HashFn hasher, Args&& ... args):
		holder_type(std::forward<Args>(args)...),
		base_type(hasher(get_value(*this)))
	{
		
	}

	friend const Value& get_value(const self_type& self)
	{ return get_value(static_cast<const holder_type&>(self)); }

	friend const Value&& get_value(const self_type&& self)
	{ return get_value(static_cast<const holder_type&&>(self)); }

};

} /* namespace detail */
/// @endinternal


template <class T, class H, class C, class ... Args>
std::unique_ptr<detail::TypedValue<T, H, C>> detail::make_typed_value(std::size_t hash_v, Args&& ... args)
{
	auto tmp = std::make_unique<AnyValueLink<T, H, C>>(hash_v, std::forward<Args>(args) ...);
	return std::unique_ptr<TypedValue<T, H, C>>(
		static_cast<TypedValue<T, H, C>*>(tmp.release())
	);
}

template <class T, class H, class C, class ... Args>
std::unique_ptr<detail::TypedValue<T, H, C>> detail::make_typed_value(H hash_fn, Args&& ... args)
{
	auto tmp = std::make_unique<AnyValueLink<T, H, C>>(hash_fn, std::forward<Args>(args) ...);
	return std::unique_ptr<TypedValue<T, H, C>>(
		static_cast<TypedValue<T, H, C>*>(tmp.release())
	);
}

template <class T, class H, class C, class ... Args>
std::unique_ptr<AnyValue<H, C>> make_any_value(std::size_t hash_value, Args&& ... args)
{
	auto tmp = detail::make_typed_value<T, H, C>(hash_value, std::forward<Args>(args)...);
	return std::unique_ptr<AnyValue<H, C>>(
		static_cast<AnyValue<H, C>*>(tmp.release())
	);
}

template <class T, class H, class C, class ... Args>
std::unique_ptr<AnyValue<H, C>> make_any_value(H hasher, Args&& ... args)
{
	auto tmp = detail::make_typed_value<T, H, C>(hasher, std::forward<Args>(args)...);
	return std::unique_ptr<AnyValue<H, C>>(
		static_cast<AnyValue<H, C>*>(tmp.release())
	);
}


/// @name Casts and Accessors
/// @{

/**
 * @brief Access the contained object through a reference to dynamic type @p To.
 * 
 * Unlike @p exact_cast() or @p unsafe_cast(), @p polymorphic_cast() can 
 * access the @p AnyValue instance's contained object through a reference to
 * a base type of the contained object.  For example, an AnyValue instance 
 * which contains an object of type `std::runtime_error` could be
 * polymorphic_cast()'d to `const std::exception&`.
 *
 * @note Neither the type of the contained object nor target type of the cast 
 *       need be of polymorphic type for this cast to work.  polymorphic_cast()
 *       can cast through non-polymorphic inheritence heirarchies.
 *
 * If the contained object is of non-class type, throws a @p std::bad_cast.
 *
 * If the contained object is an instance of a final class type, throws a 
 * @p std::bad_cast, even if the contained object is an instance of @p To.
 * 
 * If @p To is a reference to non-class or final class type, the cast fails to
 * compile.
 * 
 * @tparam To - The type to cast to.  Should take the form `const T&`.
 * 
 * @param any_v - The AnyValue instance whose contained object is being accessed.
 * 
 * @return A reference to the contained object of the requested type.
 *
 * @throws std::bad_cast
 *
 * @see exact_cast()
 * @see unsafe_cast()
 * @see try_as()
 * @see as()
 * @see AnyValue
 *
 * @relates AnyValue
 */
template <class To, class H, class C>
To polymorphic_cast(const AnyValue<H, C>& any_v)
{
	static_assert(
		std::is_same_v<const std::decay_t<To>&, To>, 
		"Can only polymorphic_cast an AnyValue reference to a const reference type."
	);
	static_assert(
		std::is_class_v<std::decay_t<To>> and not std::is_final_v<std::decay_t<To>>,
		"Cannot 'polymorphic_cast()' to final or non-class type."
	);
	return dynamic_cast<To>(any_v);
}

/**
 * @brief Access the contained object through a pointer to dynamic type @p To.
 * 
 * Unlike @p exact_cast() or @p unsafe_cast(), @p polymorphic_cast() can access 
 * the @p AnyValue instance's contained object through a (possibly polymorphic) 
 * pointer to a base type of the contained object.  For example, an AnyValue 
 * instance which contains an object of type `std::runtime_error` could be
 * polymorphic_cast()'d to `const std::exception*`.
 *
 * @note Neither the type of the contained object nor target type of the cast 
 *       need be of polymorphic type for this cast to work.  polymorphic_cast()
 *       can cast through non-polymorphic inheritence heirarchies.
 *
 * If the contained object is of non-class type, returns @p nullptr.
 *
 * If the contained object is an instance of a final class type, returns 
 * @p nullptr, even if the contained object is an instance of @p To.
 * 
 * If @p To is a pointer to non-class or final class type, the cast fails to
 * compile.
 * 
 * @tparam To - The type to cast to.  Should take the form `const T*`.
 * 
 * @param any_v - The AnyValue instance whose contained object is being accessed.
 * 
 * @return A pointer to the contained object of the requested type.
 *
 * @see exact_cast()
 * @see unsafe_cast()
 * @see try_as()
 * @see as()
 * @see AnyValue
 *
 * @relates AnyValue
 */
template <class To, class H, class C>
To polymorphic_cast(const AnyValue<H, C>* any_v)
{
	using type = std::decay_t<std::remove_pointer_t<std::decay_t<To>>>;
	static_assert(
		std::is_same_v<const type*, To>, 
		"Can only polymorphic_cast an AnyValue pointer to a pointer to const type."
	);
	static_assert(std::is_class_v<type> and not std::is_final_v<type>, 
		"Cannot 'polymorphic_cast()' to final or non-class type."
	);
	return dynamic_cast<To>(any_v);
}

/**
 * @brief Access the contained object.
 * 
 * If the type of contained object is not the same as the requested type (after
 * stripping off references and cv qualifiers, i.e. std::decay), throws a @p std::bad_cast.
 * 
 * @tparam To - The type to access the contained object as.  May be a 
 *              reference or value type.
 * 
 * @param any_v - The AnyValue instance whose contained object is to be accessed.
 * 
 * @return A reference to (or copy of, if @p To is a non-reference type) the
 *         contained object.
 *
 * @throws std::bad_cast
 *
 * @see polymorphic_cast()
 * @see unsafe_cast()
 * @see try_as()
 * @see as()
 * @see AnyValue
 *
 * @relates AnyValue
 */
template <class To, class H, class C>
To exact_cast(const AnyValue<H, C>& any_v)
{
	using to_type = std::decay_t<To>;
	if(is<to_type>(any_v))
		return static_cast<const detail::TypedValue<to_type, H, C>&>(any_v).value();
	else
		throw std::bad_cast();
}

/**
 * @brief Access the contained object.
 * 
 * If the type of contained object is not the same as the requested type (after
 * stripping off top-level pointer and then cv qualifiers), returns null.
 * 
 * @tparam To - The type to access the contained object as.  Must be a 
 *              pointer to the expected type of the contained object.
 * 
 * @param any_v - The AnyValue instance whose contained object is to be accessed.
 * 
 * @return A pointer to the contained object of type T.
 *
 * @see polymorphic_cast()
 * @see unsafe_cast()
 * @see try_as()
 * @see as()
 * @see AnyValue
 *
 * @relates AnyValue
 */
template <class To, class H, class C>
To exact_cast(const AnyValue<H, C>* any_v)
{
	static_assert(
		std::is_pointer_v<To>, 
		"exact_cast<T>(const AnyValue<H, C>*) requires that T be a pointer type."
	);
	using to_type = std::decay_t<std::remove_pointer_t<To>>;
	if(is<to_type>(*any_v))
		return std::addressof(
			static_cast<const detail::TypedValue<to_type, H, C>&>(*any_v).value()
		);
	else
		return nullptr;
}

/**
 * @brief Access the contained object without dynamic type checking.
 * 
 * If the type of contained object is not the same as the requested type (after
 * stripping off top-level reference and then cv qualifiers, i.e. std::decay), 
 * then the behavior is undefined.
 * 
 * @tparam To - The type to access the contained object as.  Must be the
 *              type of the contained object or const reference thereto.
 * 
 * @param any_v - The AnyValue instance whose contained object is to be accessed.
 * 
 * @return A pointer to the contained object of type T.
 *
 * @see exact_cast()
 * @see polymorphic_cast()
 * @see try_as()
 * @see as()
 * @see AnyValue
 * 
 * @relates AnyValue
 */
template <class To, class H, class C>
To unsafe_cast(const AnyValue<H, C>& any_v) 
{
	return static_cast<const detail::TypedValue<std::decay_t<To>, H, C>&>(any_v).value();
}

/**
 * @brief Get a pointer to the contained object of type T.  If the contained object is not
 *        an instance of type T, returns nullptr.
 *        
 *        This function works by attempting to return `te::exact_cast<const T*>(&self)` followed
 *        by a `te::polymorphic_cast<const T*>(&self)` if the exact_cast() failed.  The polymorphic_cast() 
 *        is only attempted if @p T is a non-final class type.
 *
 * @param any_v - The AnyValue instance whose contained object is to be accessed.
 *
 * @return A pointer of type `const T*` to the contained object if the contained object
 *         has type `T`, otherwise null.
 *
 * @see exact_cast()
 * @see polymorphic_cast()
 * @see unsafe_cast()
 * @see as()
 * @see AnyValue
 *
 * @relates AnyValue
 */
template <class T, class H, class C>
const T* try_as(const AnyValue<H, C>& any_v) 
{
	static_assert(std::is_object_v<T>);
	if(const T* p = exact_cast<const T*>(&any_v); static_cast<bool>(p))
		return p;
	else if constexpr(std::is_class_v<T> and not std::is_final_v<T>)
	{
		if(p = polymorphic_cast<const T*>(&any_v); static_cast<bool>(p))
			return p;
	}
	return nullptr;
}

/**
 * @brief Get a reference to the contained object of type T.  If the contained object is not
 *        an instance of type T, throws a std::bad_cast.
 *        
 *        This function works by attempting an exact_cast() followed by, if the exact_cast() failed,
 *        a polymorphic_cast().  The polymorphic_cast() is only attempted if @p T is a non-final class
 *        type.  
 *
 * @param any_v - The AnyValue instance whose contained object is to be accessed.
 *
 * @return A reference of type `const T&` to the contained object.
 *
 * @throws std::bad_cast
 *
 * @see try_as()
 * @see exact_cast()
 * @see polymorphic_cast()
 * @see unsafe_cast()
 * @see AnyValue
 *
 * @relates AnyValue
 */
template <class T, class H, class C>
const T& as(const AnyValue<H, C>& self)
{
	if(const auto* p = try_as<T>(self); static_cast<bool>(p))
		return *p;
	throw std::bad_cast();
}

/// @} Casts and Accessors


template <class T, class H, class C>
bool is(const AnyValue<H, C>& any_v)
{ return any_v.typeinfo() == typeid(T); }

/** 
 * @brief Overloads compute_hash() for AnyValue. 
 * 
 * @relates AnyValue
 */
template <class H, class C>
std::size_t compute_hash(const AnyValue<H, C>& any_v)
{ return any_v.hash; }

} /* namespace te */	

#endif /* ANY_NODE_H */

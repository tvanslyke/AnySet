#ifndef VALUE_HOLDER_H
#define VALUE_HOLDER_H

#include <type_traits>

namespace te {

template <class Value, std::size_t Tag = 0, bool = std::is_class_v<Value> and not std::is_final_v<Value>>
struct ConstValueHolder;

template <class, class> 
struct AnyValue;

template <class Value, std::size_t Tag>
struct ConstValueHolder<Value, Tag, true>:
	public Value
{
	using self_type = ConstValueHolder<Value, Tag, true>;

	using Value::Value;

	ConstValueHolder(const Value& v):
		Value(v)
	{
		
	}

	ConstValueHolder(Value&& v):
		Value(std::move(v))
	{
		
	}

	ConstValueHolder(const ConstValueHolder& v) = default;
	ConstValueHolder(ConstValueHolder&& v) = delete;
	ConstValueHolder& operator=(const ConstValueHolder& v) = delete;
	ConstValueHolder& operator=(ConstValueHolder&& v) = delete;

	friend const Value& get_value(const self_type& self)
	{ return static_cast<const Value&>(self); }

	friend const Value&& get_value(const self_type&& self)
	{ return static_cast<const Value&&>(self); }

};


template <class Value, std::size_t Tag>
struct ConstValueHolder<Value, Tag, false>
{
	using self_type = ConstValueHolder<Value, Tag, false>;

	template <class ... T>
	ConstValueHolder(T&& ... args):
		value_(std::forward<T>(args)...)
	{
		
	}
	
	friend const Value& get_value(const self_type& self)
	{ return self.value_; }

	friend const Value&& get_value(const self_type&& self)
	{ return std::move(self.value_); }

private:
	const Value value_;
};


template <class Value, std::size_t Tag = 0, bool = std::is_class_v<Value> and not std::is_final_v<Value>>
struct ValueHolder;
	
template <class Value, std::size_t Tag>
struct ValueHolder<Value, Tag, true>:
	public Value
{
	using self_type = ValueHolder<Value, Tag, true>;
	using base_type = Value;
	
	using Value::Value;

	ValueHolder(const Value& v):
		Value(v)
	{
		
	}

	ValueHolder(Value&& v):
		Value(std::move(v))
	{
		
	}

	ValueHolder(const ValueHolder& v) = default;
	ValueHolder(ValueHolder&& v) = default;
	
	ValueHolder& operator=(const ValueHolder& v) = default;
	ValueHolder& operator=(ValueHolder&& v) = default;

	friend const Value& get_value(const self_type& self)
	{ return static_cast<const Value&>(self); }

	friend const Value&& get_value(const self_type&& self)
	{ return static_cast<const Value&&>(self); }

	friend Value& get_value(self_type& self)
	{ return static_cast<Value&>(self); }

	friend Value&& get_value(self_type&& self)
	{ return static_cast<Value&&>(self); }

};


template <class Value, std::size_t Tag>
struct ValueHolder<Value, Tag, false>
{
	using self_type = ValueHolder<Value, Tag, false>;
	template <class ... T>
	ValueHolder(T&& ... args):
		value_(std::forward<T>(args)...)
	{
		
	}
	
	friend const Value& get_value(const self_type& self)
	{ return self.value_; }

	friend const Value&& get_value(const self_type&& self)
	{ return std::move(self.value_); }

	friend Value& get_value(self_type& self)
	{ return self.value_; }

	friend Value&& get_value(self_type&& self)
	{ return std::move(self.value_); }

private:
	Value value_;
};

	
} /* namespace te */

#endif /* VALUE_HOLDER_H */

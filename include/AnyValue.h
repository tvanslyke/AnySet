#ifndef ANY_VALUE_H
#define ANY_VALUE_H

namespace te {

template <class Hash, class Compare>
struct AnyValue
{
	using self_type = AnyValue<Hash, Compare>;
	virtual ~AnyValue() = default;

	AnyValue(const self_type&) = delete;
	AnyValue(self_type&&) = delete;


	virtual const std::type_info& typeinfo() const = 0;


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
	self_type* next;
	template <class, class, class>	
	friend class AnySet;
	friend class AnyList<Hash, Compare>;
};

} /* namespace te */

#endif /* ANY_VALUE_H */

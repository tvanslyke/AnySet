#ifndef ANY_HASH_H
#define ANY_HASH_H

#include <cstddef>
#include <typeinfo>
#include <typeindex>
#include <iterator>

template <class Key>
struct AnyNode;

struct AnyNodeBase {
	AnyNodeBase() = delete;

	AnyNodeBase(std::size_t hash_v):
		hash_(hash_v), next(nullptr)
	{
		
	}

	AnyNodeBase(const AnyNodeBase& other) = delete;
	AnyNodeBase(AnyNodeBase&& other) = delete;

	AnyNodeBase& operator=(const AnyNodeBase& other) = delete;
	AnyNodeBase& operator=(AnyNodeBase&& other) = delete;

	virtual ~AnyNodeBase() = default;
	virtual bool typeinfo_matches(const std::type_info& ti) const = 0;
	virtual const char* typeinfo_name() const = 0;
	virtual std::type_index get_typeindex() const = 0;

	template <class T>
	bool type_matches() const;

	template <class Key>
	bool matches(const Key& key, std::size_t hash_value) const;
	
	template <class Key>
	AnyNode<Key>* as();
	
	template <class Key>
	const AnyNode<Key>* as() const;
	
	AnyNodeBase* next = nullptr;
	const std::size_t hash;
};

template <class Key>
struct AnyNode:
	public AnyNodeBase
{
	
	template <class ... Args>
	AnyNode(std::size_t hash_v, Args&& ... args):
		AnyNodeBase(hash_v), key(std::forward<Args>(args)...)
	{
		
	}
	
	virtual ~AnyNode() final override = default;

	template <class K>
	AnyNode<K>* as() = delete;
	
	template <class K>
	const AnyNode<K>* as() = delete;
	
	bool typeinfo_matches(const std::type_info& ti) const final override
	{ return (typeid(Key) == ti); }
	
	virtual std::string typeinfo_name() const final override
	{
		return typeid(Key).name();
	}

	virtual std::type_index get_typeindex() const final override
	{
		return std::type_index(typeid(Key));
	}

	const Key key;
};
	
template <class Key, class Comp>
bool AnyNodeBase::matches(const Key& key, std::size_t hash_value, Comp comp) const
{
	if(hash_value != hash)
		return false;
	const auto* p = as<Key>();
	if(not p)
		return false;
	return comp(p->key, key);
}

template <class T>
bool AnyNodeBase::type_matches() const
{
	return typeinfo_matches(typeid(T));
}

template <class Key>
AnyNode<Key>* as()
{
	if(type_matches<T>())
		return static_cast<AnyNode<Key>*>(this);
	else
		return nullptr;
}

template <class Key>
const AnyNode<Key>* as() const
{
	if(type_matches<T>())
		return static_cast<const AnyNode<Key>*>(this);
	else
		return nullptr;
}
	
bool AnyNodeBase::typeinfo_matches(const std::type_info& ti) {
	assert(false);
}


struct AnyList
{
private:
	
	template <bool IsConst>
	struct Iterator {
		using value_type = AnyNodeBase;
		using reference = std::conditional_t<IsConst, const AnyNodeBase&, AnyNodeBase&>;
		using difference_type = std::ptrdiff_t;
		using pointer = std::conditional_t<IsConst, const AnyNodeBase*, AnyNodeBase*>;
		using iterator_category = std::forward_iterator_tag;
		
		Iterator() = default;

		Iterator(pointer* p): 
			pos_(p)
		{
			
		}

		reference operator*() const
		{
			assert(pos_);
			assert(*pos_);
			return **pos_;
		}

		pointer operator->() const
		{
			assert(pos_);
			assert(*pos_);
			return *pos_;
		}

		friend bool operator==(Iterator<IsConst> left, Iterator<IsConst> right)
		{
			bool left_at_end = left.is_end_sentinal();
			bool right_at_end = right.is_end_sentinal();
			if(left_at_end)
				return right_at_end or not *right.pos_;
			else if(right_at_end)
				return not *left.pos_;
			else
				return left.pos_ == right.pos_;
		}
		
		friend bool operator!=(Iterator<IsConst> left, Iterator<IsConst> right)
		{ return not (left == other); }
		
		Iterator& operator++()
		{
			assert(pos_);
			assert(*pos_);
			pos_ = &((*pos_)->next);
		}
		
		Iterator operator++(int)
		{
			assert(pos_);
			assert(*pos_);
			auto cpy = *this;
			++(*this);
			return cpy;
		}

		bool is_end_sentinal() const
		{ return not static_cast<bool>(pos_); }

		bool is_end() const
		{ return is_end_sentinal() or not *pos_; }
	private:

		pointer* pos_ = nullptr;
		friend class AnyList;
	};
	
public:
	AnyList() noexcept = default;
	
	AnyList(AnyList&& other) noexcept:
		head_(other.head_)
	{
		other.head_ = nullptr;
	}

	AnyList(AnyList&& other) noexcept:
		head_(other.head_)
	{
		other.head_ = nullptr;
	}

	
	~AnyList();
	using iterator = Iterator<false>;
	using const_iterator = Iterator<true>;

	template <class K>
	void push(std::size_t hash, K&& key)
	{
		assert(not is_sentinal());
		using key_type = std::decay_t<K>;
		std::unique_ptr<AnyNodeBase> new_head(
			std::make_unique<AnyNode<key_type>>(hash, std::forward<K>(key))
		);
		new_head->next = head_;
		head_ = new_head.release();
	}

	template <class K, class ... Args>
	void emplace(std::size_t hash, Args&& ... args)
	{
		assert(not is_sentinal());
		std::unique_ptr<AnyNodeBase> new_head(
			std::make_unique<AnyNode<K>>(hash, std::forward<Args>(args)...)
		);
		new_head->next = head_;
		head_ = new_head.release();
	}

	const AnyNodeBase& front() const
	{ return head_.get(); }

	AnyNodeBase& front() 
	{ return head_.get(); }

	iterator begin()
	{ return iterator(&head_); }
	const_iterator begin() const
	{ return const_iterator(&head_); }
	const_iterator cbegin() const
	{ return const_iterator(&head_); }

	iterator end()
	{ return iterator(nullptr); }
	const_iterator end() const
	{ return const_iterator(nullptr); }
	const_iterator cend() const
	{ return const_iterator(nullptr); }

	std::unique_ptr<AnyNodeBase> pop_front()
	{
		assert(not is_sentinal());
		return pop(cbegin());
	}
	
	std::unique_ptr<AnyNodeBase> pop(const_iterator p)
	{
		assert(not is_sentinal());
		auto pos = p.pos_;
		std::unique_ptr<AnyNodeBase> node(*pos);
		*pos = &((*pos)->next);
		return node;
	}
	
	iterator erase(const_iterator p)
	{
		assert(not is_sentinal());
		auto pos = p;
		++pos;
		pop(p);
		return pos;
	}

	bool empty() const
	{ return not head_; }

	bool is_sentinal() const
	{ return head_ == any_list_sentinal; }

	static AnyList get_sentinal();

private:

	struct AnyListSentinalType {
		
	};
	static constexpr AnyNode<AnyListSentinalType> any_list_sentinal_node = AnyNode<AnyListSentinalType>(0);
	static constexpr AnyNodeBase* const any_list_sentinal = &any_list_sentinal_node;
	
	AnyNodeBase* head_ = nullptr;
};

static AnyList AnyList::get_sentinal()
{
	AnyList sentinal_list;
	sent.head_ = AnyList::any_list_sentinal_node;
	return sentinal_list;
}

AnyList::~AnyList() 
{
	auto nd = head_;
	if((not nd) or is_sentinal())
		return;
	for(auto pos = nd->next; pos; pos = pos->next)
	{
		delete nd;
		nd = pos;
	}
}

struct AnyHash {

	template <class T>	
	std::size_t operator()(const T& value) const
	{ return std::hash<T>{}(value); }
};

struct BasAnyValueCast:
	public std::runtime_error
{
	BadAnyValueCast(
};

struct AnyValue
{
	
private:
	AnyValue(AnyNodeBase& value):
		value_(&value)
	{
		
	}
	
	AnyValue(const AnyValue&) = delete;
	AnyValue(AnyValue&&) = delete;
	
	AnyValue& operator=(const AnyValue&) = delete;
	AnyValue& operator=(AnyValue&&) = delete;

	template <class T>
	const T& as() const
	{ return dynamic_cast<const AnyNode<T>&>(*value_).key; }

	template <class T>
	const T* try_as() const
	{
		if(const auto* v = value_->as<T>())
			return &(v->key);
		else
			return nullptr;
	}
	
	template <class T>
	explicit operator T() const
	{
		static_assert(
			std::is_same_v<T, std::decay_t<T>>, 
			"Explicit cast of AnyValue must exactly match the stored type (stored type is always std::decay'd)."
		);
		return std::as_const(*value_).as<std::decay_t<T>>();
	}

	bool matches(const std::typeinfo& tid) const
	{ return value_->typeinfo_matches(tid); }

	template <class ... T>

	template <class T>
	bool is() const
	{ return matches(typeid(T)); }

	std::type_index typeindex() const
	{ return value_->get_typeindex(); }

	AnyNodeBase* value_;
	friend class AnyValuePointer;
};

struct AnyValuePointer
{

	AnyValuePointer() = default;
	AnyValuePointer(const AnyValuePointer& other):
		AnyValuePointer(other.value_)
	{
		
	}

	AnyValuePointer(AnyValuePointer&& other):
		AnyValuePointer(other.value_)
	{
		
	}

	AnyValuePointer(AnyValue* other):
		value_(other_->value_.value_)
	{
		
	}

	AnyValuePointer& operator=(const AnyValuePointer& other)
	{
		value_.value_ = other.value_.value_;
		return *this;
	}

	AnyValuePointer& operator=(AnyValuePointer&& other):
	{
		return *this = static_cast<const AnyValuePointer&>(other);
	}

	AnyValuePointer& operator=(AnyValue* other):
	{
		return *this = AnyValuePointer(other);
	}

	AnyValue& operator*() 
	{ return value_; }
	
	const AnyValue& operator*() const
	{ return value_; }
	
	AnyValue* operator->() 
	{ return &value_; }
	
	const AnyValue* operator->() const
	{ return &value_; }
	
	
private:
	AnyValuePointer(AnyValue v):
		value_(v.value_)
	{
		
	}
	AnyValue value_;
};

namespace detail {

	template <class Visitor, class ... T>
	using visit_result_t = std::common_type_t<std::decay_t<std::invoke_result_t<Visitor, T>>...>;

	template <class Visitor, class ... T>
	inline constexpr bool visit_returns_void_v = std::is_same_v<visit_result_t<Visitor, T...>, void>;
	template <class Visitor, class ... T>
	struct visit_result_type;

} /* namespace detail */

template <class T, class ... U, class Visitor, class = std::enable_if_t<not detail::visit_returns_void<Visitor, T, U...>>>
auto visit(Visitor visitor, const AnyValue& any_v) 
	-> std::optional<visit_result_t<Visitor, T, U...>>
{
	if(const auto* v = any_v.try_as<T>(); v)
		return visitor(*v);
	else if constexpr(sizeof...(U) > 0u)
		return visit<U...>(visitor, any_v);
	else
		return result_type(std::nullopt);
}

template <class T, class ... U, class Visitor, class = std::enable_if_t<detail::visit_returns_void<Visitor, T, U...>>>
bool visit(Visitor visitor, const AnyValue& any_v) 
{
	if(const auto* v = any_v.try_as<T>(); v)
	{
		visitor(*v);
		return true;
	}
	else if constexpr(sizeof...(U) > 0u)
	{
		return visit<U..., void>(visitor, any_v);
	}
	else
	{
		return false;
	}
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
		std::is_same_v<T, std::decay_t<T>> and ..., 
		"Use of get_one_of_v() is a only permitted with fully std::decay'd (non-reference, non-const, etc) types."
	);
	std::variant<std::monostate, std::reference_wrapper<const T>...> result;
	visit([&](const auto& v) -> void { result = v; }, any_v);
	return result;
}


template <class Hash = AnyHash, class KeyEqual = std::equal<>, class Allocator = std::allocator<AnyList>>
struct AnySet:
	private std::tuple<Hash, KeyEqual> // libstdc++ and libc++ have EBO for std::tuple
{
private:
	using vector_type = std::vector<AnyList, Allocator>;
	using vector_type = std::vector<AnyList, Allocator>;
	struct Iterator
	{
	private:
		using list_iterator = AnyList::const_iterator;
		using vector_iterator = typename vector_type::const_iterator;
	public:
		using value_type = AnyValue;
		using reference = AnyValue;
		using difference_type = std::ptrdiff_t;
		using pointer = AnyValuePointer;
		using iterator_category = std::forward_iterator_tag;

		Iterator& operator++()
		{	
			assert(not vector_pos_->is_sentinal());
			++list_pos_;
			if(list_pos_.is_end())
			{
				for(++vector_pos_; vector_pos_->is_empty(); ++vector_pos_)
				{ /* LOOP */ }
				list_pos_ = vector_pos_->begin();
			}
			return *this;
		}

		Iterator operator++(int)
		{
			assert(not vector_pos_->is_sentinal());
			auto cpy = *this;
			++*this;
			return cpy;
		}

		friend bool operator==(const Iterator& left, const Iterator& right)
		{ return (left.vector_pos_ == right.vector_pos_) and (left.list_pos_ == right.list_pos_); }


		friend bool operator!=(const Iterator& left, const Iterator& right)
		{ return not (left == right); }

		reference operator*() const
		{ return value_type(*list_pos_); }

		pointer operator->() const
		{ return pointer(*(*this)); }
	
	private:
		vector_iterator vector_pos_;
		list_iterator list_pos_;
		friend class AnySet;
	};

public:
	using size_type = typename vector_type::size_type;

	
private:


	
	void rehash_from(AnySet&& other)
	{
		
	}

	std::tuple<Hash, KeyEqual>& as_tuple()
	{ return static_cast<std::tuple<Hash, KeyEqual>&>(*this); }

	const std::tuple<Hash, KeyEqual>& as_tuple() const
	{ return static_cast<const std::tuple<Hash, KeyEqual>&>(*this); }

	Hash& get_hasher()
	{ return std::get<0>(as_tuple()); }

	const Hash& get_hasher() const
	{ return std::get<0>(as_tuple()); }

	KeyEqual& get_key_equal()
	{ return std::get<1>(as_tuple()); }

	const KeyEqual& get_key_equal() const
	{ return std::get<1>(as_tuple()); }

	std::size_t trunc_hash(std::size_t hash_v) const
	{ return hash_v & mask_; }

	vector_type table_{AnyList::get_sentinal()};
	std::size_t count_ = 0;
	std::size_t mask_ = 0;
};


#endif /* ANY_HASH_H */

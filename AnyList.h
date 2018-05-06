#ifndef ANY_LIST_H
#define ANY_LIST_H
#include "AnyNode.h"
#include <iterator>
#include <memory>
#include <algorithm>
#include <iosfwd>

namespace te {

struct AnyListSentinalType {
	
};

template <class Hash, class Compare>
struct AnyList
{
	using value_type = AnyValue<Hash, Compare>;
	using reference = value_type&;
	using const_reference = const value_type&;
	using pointer = value_type*;
	using const_pointer = const value_type*;
	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;
private:
	using self_type = AnyList<Hash, Compare>;
	using node_base_type = AnyNodeBase<Hash, Compare>;
	template <class T>
	using node_type = AnyNode<T, Hash, Compare>;
	

	template <bool IsConst>
	struct Iterator {
		using value_type = typename self_type::value_type;
		using reference = std::conditional_t<
			IsConst, 
			typename self_type::const_reference,
			typename self_type::reference
		>;
		using difference_type = typename self_type::difference_type;
		using pointer = std::conditional_t<
			IsConst, 
			typename self_type::const_pointer,
			typename self_type::pointer
		>;
		using iterator_category = std::forward_iterator_tag;

		Iterator() = default;
		Iterator(const Iterator<false>& other):
			pos_(other.pos_)
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
			if(left.is_end_sentinal())
				return right.is_end();
			else if(left.is_normal_end())
				return right.is_end_sentinal() or (left.pos_ == right.pos_);
			else
				return left.pos_ == right.pos_;
		}

		friend bool operator!=(Iterator<IsConst> left, Iterator<IsConst> right)
		{ return not (left == right); }

		Iterator& operator++()
		{
			assert(pos_);
			assert(*pos_);
			pos_ = &((*pos_)->next);
			return *this;
		}

		Iterator operator++(int)
		{
			assert(pos_);
			assert(*pos_);
			auto cpy = *this;
			++(*this);
			return cpy;
		}

		bool is_end() const
		{ return is_end_sentinal() or (not *pos_); }

	private:
		bool is_end_sentinal() const
		{ return not static_cast<bool>(pos_); }
		
		bool is_normal_end() const 
		{ return not *pos_; }


		using pos_type = std::conditional_t<IsConst, node_base_type* const*, node_base_type**>;
		Iterator(pos_type p): 
			pos_(p)
		{
			
		}
		
		Iterator<false> to_non_const() const
		{
			return Iterator<false>(&const_cast<node_base_type*&>(*pos_));
		}
		
		pos_type pos_ = nullptr;
		friend class AnyList<Hash, Compare>;
		template <class, class, class>
		friend class AnySet;
	};

public:

	AnyList() noexcept = default;

	AnyList(const self_type&) = delete;

	AnyList(self_type&& other) noexcept:
		head_(other.head_)
	{
		other.head_ = nullptr;
	}
	
	AnyList& operator=(const self_type&) = delete;
	struct MakeCopyTag{};
	// Make copying possible, but not conventionally
	AnyList(const self_type& other, MakeCopyTag)
	{
		auto it = begin();
		for(const auto& any_v: other)
		{
			splice(it, any_v.clone());
			++it;
		}
	}

	~AnyList()
	{
		clear();
	}
		
	
	self_type& operator=(self_type&& other) noexcept
	{
		clear();
		head_ = other.head_;
		other.head_ = nullptr;
		return *this;
	}


	using iterator = Iterator<false>;
	using const_iterator = Iterator<true>;
	
	void swap(self_type& other)
	{ std::swap(head_, other.head_); }

	std::size_t clear()
	{
		if(static_cast<bool>(head_) and (not is_sentinal()))
		{
			std::size_t count = 0;
			do { 
				pop_front();
				++count;
			} while(not empty());
			return count;
		}
		return 0;
	}
	
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

	template <class K>
	static iterator insert(const_iterator pos, std::size_t hash, K&& key)
	{
		return emplace<std::decay_t<K>>(pos, hash, std::forward<K>(key));
	}

	template <class K, class ... Args>
	static iterator emplace(const_iterator pos, std::size_t hash, Args&& ... args)
	{
		assert(pos.pos_);
		std::unique_ptr<node_base_type> new_node(
			std::make_unique<node_type<K>>(hash, std::forward<Args>(args)...)
		);
		node_base_type*& node_pos = *pos.to_non_const().pos_;
		new_node->next = node_pos;
		node_pos = new_node.release();
		return std::next(pos).to_non_const();
	}

	const value_type& front() const
	{ return *head_; }

	value_type& front() 
	{ return *head_; }

	std::unique_ptr<value_type> pop_front()
	{
		assert(not is_sentinal());
		return pop(cbegin());
	}
	
	static std::unique_ptr<value_type> pop(const_iterator p)
	{
		assert(not p.is_end());
		std::unique_ptr<value_type> node(*p.pos_);
		node_base_type*& pos = *p.to_non_const().pos_;
		pos = pos->next;
		// TODO: it *should* be okay to not set pos->next to nullptr
		node->get_next() = nullptr;
		return node;
	}
	
	static iterator splice(const_iterator p, std::unique_ptr<value_type> node) noexcept
	{
		auto& pos = *p.to_non_const().pos_;
		node->get_next() = pos;
		pos = static_cast<node_base_type*>(node.release());
		return p.to_non_const();
	}

	static iterator erase(const_iterator p)
	{
		pop(p);
		return p.to_non_const();
	}

	static std::pair<iterator, std::size_t> erase(const_iterator first, const_iterator last)
	{
		if(first == last)
			return std::make_pair(first.to_non_const(), 0);
		std::size_t count = 0;
		auto* last_pos = *last.pos_;
		while(*first.pos_ != last_pos)
		{
			erase(first);
			++count;
		}
		return std::make_pair(first.to_non_const(), count);
	}

	static std::pair<iterator, std::size_t> erase_to_end(const_iterator first)
	{
		std::size_t count = 0;
		for(count = 0; *first.pos_; ++count)
		{ erase(first); }
		return std::make_pair(first.to_non_const(), count);
	}

	bool empty() const
	{ return not head_; }

	bool is_sentinal() const
	{ return head_ == any_list_sentinal; }

	static self_type get_sentinal() {
		self_type tmp;
		tmp.head_ = any_list_sentinal;
		return tmp;
	}

	friend void swap(self_type& left, self_type& right)
	{ left.swap(right); }

	static constexpr const MakeCopyTag make_copy = MakeCopyTag{};
private:

	static node_type<AnyListSentinalType> any_list_sentinal_node;
	static node_base_type* const any_list_sentinal;
	
	// Can't use unique_ptr because iterators need to be able to 
	// store a pointer-to-pointer.  
	node_base_type* head_{nullptr};
		
	template <class, class, class>
	friend class AnySet;
};

template <class Hash, class Compare>
AnyNode<AnyListSentinalType, Hash, Compare> AnyList<Hash, Compare>::any_list_sentinal_node = 
	AnyNode<AnyListSentinalType, Hash, Compare>(0);

template <class Hash, class Compare>
typename AnyList<Hash, Compare>::node_base_type* const AnyList<Hash, Compare>::any_list_sentinal 
	= &AnyList<Hash, Compare>::any_list_sentinal_node;

template <class Hash, class Compare>
bool operator==(const AnyList<Hash, Compare>& left, const AnyList<Hash, Compare>& right) 
{
	auto lpos = left.begin();
	auto rpos = right.begin();
	while((not lpos.is_end()) and (not rpos.is_end()))
	{
		if(not (*lpos++ == *rpos++))
			return false;
	}
	assert(lpos.is_end() or rpos.is_end());
	return lpos.is_end() and rpos.is_end();
}

template <class Hash, class Compare>
bool operator!=(const AnyList<Hash, Compare>& left, const AnyList<Hash, Compare>& right) 
{ return not (left == right); }

template <class Hash, class Compare>
std::ostream& operator<<(std::ostream& os, const AnyList<Hash, Compare>& list)
{
	auto pos = list.begin();
	auto stop = list.end();
	os << "AnyList(";
	if(pos != stop)
	{
		os << *pos++;
		while(pos != stop)
			os << ", " << *pos++;
	}
	return os << ')';
}

} /* namespace te */


#endif /* ANY_LIST_H */

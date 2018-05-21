#ifndef ANY_LIST_H
#define ANY_LIST_H

#ifdef _MSC_VER
# include <iso646.h>
#endif 

#include "AnyNode.h"
#include <iterator>
#include <memory>
#include <algorithm>
#include <iosfwd>

/// @internal
namespace te::detail {

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
			return left.pos_ == right.pos_;
		}

		friend bool operator!=(Iterator<IsConst> left, Iterator<IsConst> right)
		{ return not (left == right); }

		Iterator& operator++()
		{
			assert(pos_);
			assert(*pos_);
			pos_ = std::addressof((*pos_)->next);
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
		{
			assert(pos_);
			return not static_cast<bool>(*pos_);
		}

		
		bool is_null() const
		{
			return not static_cast<bool>(pos_);
		}
	
	private:
		using pos_type = std::conditional_t<IsConst, value_type* const*, value_type**>;

		Iterator(pos_type p): 
			pos_(p)
		{
			
		}
		
		Iterator<false> to_non_const() const
		{
			return Iterator<false>(&const_cast<value_type*&>(*pos_));
		}
		
		pos_type pos_ = nullptr;

		friend struct AnyList<Hash, Compare>;
		template <class, class, class>
		friend struct ::te::AnySet;
	};

public:
	using iterator = Iterator<false>;
	using const_iterator = Iterator<true>;

	void _assert_invariants() const
	{
		assert(not static_cast<bool>(*tail_));
		if(empty())
		{
			assert(size() == 0u);
			assert(tail_ == std::addressof(head_));
			assert(not static_cast<bool>(head_));
		}
		else
		{
			assert(size() != 0u);
			assert(tail_ != std::addressof(head_));
			assert(static_cast<bool>(head_));
		}
		assert(static_cast<size_type>(std::distance(begin(), end())) == size());
	}

	AnyList() noexcept = default;

	AnyList(const self_type&) = delete;

	AnyList(self_type&& other) noexcept:
		head_(other.head_), tail_(other.tail_), count_(other.count_)
	{
		other.head_ = nullptr;
		other.tail_ = &(other.head_);
		other.count_ = 0u;
		if(size() == 0)
		{
			assert(tail_ == std::addressof(other.head_));
			tail_ = std::addressof(head_);
		}
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
		count_ = other.count_;
		tail_ = (other.tail_ == &(other.head_)) ? &head_ : other.tail_;
		other.head_ = nullptr;
		other.tail_ = &(other.head_);
		other.count_ = 0u;
		return *this;
	}
	
	iterator begin()
	{ return iterator(&head_); }
	const_iterator begin() const
	{ return const_iterator(&head_); }
	const_iterator cbegin() const
	{ return const_iterator(&head_); }

	iterator end()
	{
		assert(static_cast<bool>(tail_));
		assert(not static_cast<bool>(*tail_));
		return iterator(tail_);
	}
	const_iterator end() const
	{
		assert(static_cast<bool>(tail_));
		assert(not static_cast<bool>(*tail_));
		return const_iterator(tail_);
	}

	const_iterator cend() const
	{
		assert(static_cast<bool>(tail_));
		assert(not static_cast<bool>(*tail_));
		return const_iterator(tail_);
	}


	void swap(self_type& other) noexcept
	{
		std::swap(head_, other.head_);
		std::swap(tail_, other.tail_);
		std::swap(count_, other.count_);
		if(tail_ == std::addressof(other.head_))
		{
			assert(size() == 0);
			tail_ = std::addressof(head_);
		}
		if(other.tail_ == std::addressof(head_))
		{
			assert(other.size() == 0);
			other.tail_ = std::addressof(other.head_);
		}
	}

	std::size_t clear() noexcept
	{
		auto cnt = count_;
		erase(begin(), end());
		assert(not static_cast<bool>(head_));
		assert(std::addressof(head_) == tail_);
		assert(empty());
		return cnt;
	}
	
	iterator push_back(std::unique_ptr<value_type>&& node)
	{
		assert(not static_cast<bool>(*tail_));
		assert(static_cast<bool>(node));
		assert(not static_cast<bool>(node->next));
		*tail_ = node.release();
		iterator old_tail(tail_);
		++count_;
		tail_ = std::addressof((*tail_)->next);
		assert(not static_cast<bool>(*tail_));
		return old_tail;
	}
	
	std::pair<std::unique_ptr<value_type>, iterator> pop(const_iterator p)
	{
		assert(not static_cast<bool>(*tail_));
		assert(not empty());
		std::unique_ptr<value_type> node(*p.pos_);
		value_type*& pos = *p.to_non_const().pos_;
		if(not static_cast<bool>(pos->next))
			tail_ = &pos;
		pos = pos->next;
		node->next = nullptr;
		--count_;
		assert(not static_cast<bool>(*tail_));
		return std::make_pair(std::move(node), iterator(&pos));
	}
	
	iterator splice(const_iterator p, std::unique_ptr<value_type>&& node) noexcept
	{
		assert(not static_cast<bool>(*tail_));
		value_type*& pos = *p.to_non_const().pos_;
		if(not static_cast<bool>(pos))
			tail_ = std::addressof(node->next);
		node->next = pos;
		pos = node.release();
		++count_;
		assert(not static_cast<bool>(*tail_));
		return p.to_non_const();
	}

	iterator erase(const_iterator p)
	{
		assert(not empty());
		pop(p);
		return p.to_non_const();
	}

	std::pair<iterator, std::size_t> erase(const_iterator first, const_iterator last)
	{
		if(first == last)
			return std::make_pair(first.to_non_const(), 0);
		std::size_t init_size = size();
		auto* last_pos = *last.pos_;
		while(*first.pos_ != last_pos)
			first = erase(first);
		return std::make_pair(first.to_non_const(), init_size - size());
	}
	
	size_type size() const noexcept
	{ return count_; }

	bool empty() const noexcept
	{
		assert(static_cast<bool>(size()) == static_cast<bool>(head_));
		assert((tail_ == &head_) ? size() == 0u : size() > 0u);
		return not static_cast<bool>(size());
	}

	friend void swap(self_type& left, self_type& right)
	{ left.swap(right); }

	static constexpr const MakeCopyTag make_copy = MakeCopyTag{};
private:

	// Can't use unique_ptr because iterators need to be able to 
	// store a pointer-to-pointer.  
	value_type* head_{nullptr};
	value_type** tail_{&head_};
	size_type count_{0u};
};

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

} /* namespace te::detail */

/// @endinternal

#endif /* ANY_LIST_H */

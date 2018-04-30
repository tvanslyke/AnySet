#ifndef ANY_LIST_H
#define ANY_LIST_H
#include "AnyNode.h"
#include <iterator>
#include <memory>
#include <algorithm>
#include <iosfwd>

struct AnyList
{
private:

	template <bool IsConst>
	struct Iterator {
		using value_type = AnyValue;
		using reference = std::conditional_t<IsConst, const AnyValue&, AnyValue&>;
		using difference_type = std::ptrdiff_t;
		using pointer = std::conditional_t<IsConst, const AnyValue*, AnyValue*>;
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


		using pos_type = std::conditional_t<IsConst, AnyNodeBase* const*, AnyNodeBase**>;
		Iterator(pos_type p): 
			pos_(p)
		{
			
		}
		
		Iterator<false> to_non_const() const
		{
			return Iterator<false>(&const_cast<AnyNodeBase*&>(*pos_));
		}
		
		pos_type pos_ = nullptr;
		friend class AnyList;
		template <class, class, class>
		friend class AnySet;
	};

public:
	AnyList() noexcept = default;

	AnyList(const AnyList&) = delete;

	AnyList(AnyList&& other) noexcept:
		head_(other.head_)
	{
		other.head_ = nullptr;
	}


	~AnyList()
	{
		clear();
	}
		
	
	AnyList& operator=(AnyList&& other) noexcept
	{
		clear();
		head_ = other.head_;
		other.head_ = nullptr;
	}


	using iterator = Iterator<false>;
	using const_iterator = Iterator<true>;
	
	void swap(AnyList& other)
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
		std::unique_ptr<AnyNodeBase> new_node(
			std::make_unique<AnyNode<K>>(hash, std::forward<Args>(args)...)
		);
		AnyNodeBase*& node_pos = *pos.to_non_const().pos_;
		new_node->next = node_pos;
		node_pos = new_node.release();
		return std::next(pos).to_non_const();
	}

	const AnyValue& front() const
	{ return *head_; }

	AnyValue& front() 
	{ return *head_; }

	std::unique_ptr<AnyValue> pop_front()
	{
		assert(not is_sentinal());
		return pop(cbegin());
	}
	
	static std::unique_ptr<AnyValue> pop(const_iterator p)
	{
		std::unique_ptr<AnyValue> node(*p.pos_);
		auto& pos = *p.to_non_const().pos_;
		pos = pos->next;
		// NOTE: node's "next" pointer is not null
		return node;
	}
	
	static iterator splice(const_iterator p, std::unique_ptr<AnyValue> node) noexcept
	{
		auto& pos = *p.to_non_const().pos_;
		node->get_next() = pos;
		pos = static_cast<AnyNodeBase*>(node.release());
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

	static AnyList get_sentinal();

	friend void swap(AnyList& left, AnyList& right)
	{ left.swap(right); }

private:

	struct AnyListSentinalType {
		
	};
	static const AnyNode<AnyListSentinalType> any_list_sentinal_node;
	static const AnyNodeBase* const any_list_sentinal;
	
	// Can't use unique_ptr because iterators need to be able to 
	// store a pointer-to-pointer.  
	AnyNodeBase* head_{nullptr};
};

const AnyNode<AnyList::AnyListSentinalType> AnyList::any_list_sentinal_node = AnyNode<AnyList::AnyListSentinalType>(0);
const AnyNodeBase* const AnyList::any_list_sentinal = &any_list_sentinal_node;

bool operator==(const AnyList& left, const AnyList& right) 
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

bool operator!=(const AnyList& left, const AnyList& right) 
{ return not (left == right); }

std::ostream& operator<<(std::ostream& os, const AnyList& list)
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

#endif /* ANY_LIST_H */

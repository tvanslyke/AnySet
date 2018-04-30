#ifndef ANY_HASH_H
#define ANY_HASH_H

#include <cstddef>
#include <cmath>
#include "AnyList.h"
#include <vector>
#include <algorithm>


struct AnyHash {

	template <class T>	
	std::size_t operator()(const T& value) const
	{ return std::hash<T>{}(value); }
};

namespace detail {

template <typename T, typename = void>
struct is_iterator
{
	static constexpr const bool value = false;
};

template <typename T>
struct is_iterator<T, std::enable_if_t<!std::is_same_v<typename std::iterator_traits<T>::value_type, void>>>
{
	static constexpr const bool value = true;
};

template <class T>
inline constexpr const bool is_iterator_v = is_iterator<T>::value;


} /* namespace detail */

template <class Hash = AnyHash, class KeyEqual = std::equal_to<>, class Allocator = std::allocator<AnyList>>
struct AnySet:
	private std::tuple<Hash, KeyEqual> // libstdc++ and libc++ have EBO for std::tuple
{
private:
	using vector_type = std::vector<AnyList, Allocator>;
	using self_type = AnySet<Hash, KeyEqual, Allocator>;

	template <bool IsConst>
	struct Iterator
	{
	private:
		using list_iterator = std::conditional_t<IsConst,
			AnyList::const_iterator,
			AnyList::iterator
		>;
		using vector_iterator = std::conditional_t<IsConst,
			typename vector_type::const_iterator,
			typename vector_type::iterator
		>;
	public:
		using value_type = AnyValue;
		using reference = std::conditional_t<IsConst, const AnyValue&, AnyValue&>;
		using difference_type = std::ptrdiff_t;
		using pointer = std::conditional_t<IsConst, const AnyValue*, AnyValue*>;
		using iterator_category = std::forward_iterator_tag;

		Iterator() = default;

		Iterator& operator++()
		{	
			assert(not vector_pos_->is_sentinal());
			++list_pos_;
			ensure_valid();
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
		Iterator(vector_iterator v_pos, list_iterator l_pos):
			vector_pos_(v_pos), list_pos_(l_pos)
		{
			
		}

		std::size_t erase()
		{
			list_pos_ = AnyList::erase(list_pos_);
			ensure_valid();
			return 1;
		}

		std::size_t erase_range(Iterator<IsConst> last)
		{
			std::size_t count = 0;
			assert(not vector_pos_.empty());
			assert(not last.vector_pos_.empty());
			if(vector_pos_ != last.vector_pos_)
			{
				assert(vector_pos_ < last.vector_pos_);
				auto [_, qty] = AnyList::erase_to_end(list_pos_);
				for(auto pos = vector_pos_ + 1; pos != last.vector_pos_; ++pos)
					count += pos->clear();
				vector_pos_ = last.vector_pos_;
				list_pos_ = vector_pos_->begin();
			}
			auto [pos, qty] = AnyList::erase(list_pos_, last.list_pos_);
			count += qty;
			list_pos_ = pos;
			ensure_valid();
			return count;
		}

		void ensure_valid()
		{
			if(list_pos_.is_end())
			{
				for(++vector_pos_; vector_pos_->is_empty(); ++vector_pos_)
				{ /* LOOP */ }
				list_pos_ = vector_pos_->begin();
			}
		}
		vector_iterator vector_pos_;
		list_iterator list_pos_;
		friend class AnySet;
	};

public:
	using size_type = typename vector_type::size_type;
	using difference_type = typename vector_type::difference_type;
	using key_equal = KeyEqual;
	using hasher = Hash;
	using allocator_type = typename vector_type::allocator_type;
	using value_type = AnyValue;
	using key_type = AnyValue;
	using reference = AnyValue&;
	using const_reference = const AnyValue&;
	using pointer = AnyValue*;
	using const_pointer = const AnyValue*;
	using iterator = Iterator<false>;
	using const_iterator = Iterator<true>;
	using local_iterator = AnyList::iterator;
	using const_local_iterator = AnyList::const_iterator;

	/// ITERATORS ///

	// BEGIN
	const_iterator cbegin() const
	{
		if(empty())
			return cend();
		const_iterator it(table_.cbegin(), table_[0].cbegin());
		it.ensure_valid();
		return it;
	}

	const_iterator begin() const
	{ return cbegin(); }

	iterator begin() 
	{ return to_non_const_iterator(cbegin()); }

	// END
	const_iterator cend() const
	{ return const_iterator(table_.cend() - 1, table_.back().cbegin()); }

	const_iterator end() const
	{ return cend(); }

	iterator end()
	{ return to_non_const_iterator(cend()); }

	/// CAPACITY AND SIZE ///
	size_type size() const
	{ return count_; }

	bool empty() const
	{ return static_cast<bool>(size()); }

	bool max_size() const
	{ return std::numeric_limits<size_type>::max(); }


	/// MODIFIERS ///
	// CLEAR
	void clear()
	{
		for(size_type i = 0, len = table_size(); i < len; ++i)
			table_[i].clear();
		count_ = 0;
	}
	
	// EMPLACEMENT
	template <class T, class ... Args>
	std::pair<iterator, bool> emplace(Args&& ... args)
	{
		auto node(std::make_unique<T>(std::forward<Args>(args)...));
		auto& value = node->value;
		auto hash_v = get_hasher()(value);
		auto bucket_pos = table_.begin() + (hash_v & (table_size() - 1));
		auto [pos, success] = insert_impl(std::forward<T>(value), hash_v, *bucket_pos, std::move(node));
		return std::make_pair(iterator(bucket_pos, pos), success);
	}
	
	template <class T, class ... Args>
	std::pair<iterator, bool> emplace_hint(const_iterator, Args&& ... args)
	{ return emplace<T>(std::forward<Args>(args)...); }

	// INSERTION
	template <class T>
	std::pair<iterator, bool> insert(T&& value)
	{
		auto hash_v = get_hasher()(value);
		auto bucket_pos = table_.begin() + (hash_v & (table_size() - 1));
		auto [pos, success] = insert_impl(std::forward<T>(value), hash_v, *bucket_pos);
		return std::make_pair(iterator(bucket_pos, pos), success);
	}

	template <class T>
	std::pair<iterator, bool> insert(const_iterator, T&& value)
	{ return insert(std::forward<T>(value)); }

	template <class It>
	void insert(It first, It last)
	{
		using iter_cat = typename std::iterator_traits<It>::iterator_category;
		range_insert(first, last, iter_cat{});
	}

	template <
		class T, 
		class U, 
		class ... V,
		class = std::enable_if_t<
			(not std::is_same_v<std::decay_t<T>, std::decay_t<U>> and detail::is_iterator_v<std::decay_t<T>>)
			and (not std::is_same_v<std::decay_t<T>, const_iterator>)
			and (not std::is_same_v<std::decay_t<T>, iterator>)
		>
	>
	void insert(T&& first, U&& second, V&& ... args)
	{
		arg_insert(
			std::forward<T>(first),
			std::forward<U>(second),
			std::forward<V>(args) ...
		);
	}
	
	template <class T>
	void insert(std::initializer_list<T> ilist)
	{
		insert(ilist.begin(), ilist.end());
	}

	// ERASURE
	iterator erase(const_iterator pos)
	{
		count_ -= pos.erase();
		return to_non_const_iterator(pos);
	}

	iterator erase(const_iterator first, const_iterator last)
	{
		count -= first.erase_range(last);
		return to_non_const_iterator(first);
	}

	template <class T>
	size_type erase(const T& value)
	{
		auto hash_v = get_hasher()(value);
		auto bucket_pos = table_.begin() + hash_v & (table_size() - 1);
		auto [list_pos, found] = find_position(*bucket_pos, hash_v, value);
		assert(not list_pos.is_end());
		if(found)
			erase(iterator(bucket_pos, list_pos));
		return found;
	}

	// SWAP
	void swap(self_type& other)
	{
		using std::swap;
		swap(table_, other.table_);
		swap(as_tuple(), other.as_tuple());
		swap(count_, other.count_);
		swap(max_load_factor_, other.max_load_factor_);
	}

	/// LOOKUP ///
	template <class T>
	size_type count(const T& value) const
	{
		auto hash_v = get_hasher()(value);
		bool found = false;
		std::tie(std::ignore, found) = find_position(
			table_[hash_v & (table_size() - 1)],
			hash_v,
			value
		);
		return found;
	}

	template <class T>
	const_iterator find(const T& value) const
	{
		auto hash_v = get_hasher()(value);
		auto bucket_pos = table_.cbegin() + hash_v & (table_size() - 1);
		auto [pos, found] = find_position(
			*bucket_pos,
			hash_v,
			value
		);
		if(found)
			return const_iterator(bucket_pos, pos);
		else
			return cend();
	}

	template <class T>
	iterator find(const T& value)
	{
		return to_non_const_iterator(
			static_cast<const AnySet*>(this)->find(value)
		);
	}

	template <class T>
	std::pair<const_iterator, const_iterator> equal_range(const T& value) const
	{
		auto pos = find(value);
		if(pos != cend())
			return std::make_pair(pos, std::next(pos));
		else
			return std::make_pair(cend(), cend());
	}

	template <class T>
	std::pair<iterator, iterator> equal_range(const T& value)
	{
		auto [first, last] = static_cast<const AnySet*>(this)->equal_range(value);
		return std::make_pair(
			to_non_const_iterator(first),
			to_non_const_iterator(last)
		);
	}


	/// BUCKET INTERFACE ///
	// BUCKET ITERATORS
	const_local_iterator cbegin(size_type buck) const
	{
		assert(buck < bucket_count());
		return table_[buck].cbegin();
	}

	const_local_iterator begin(size_type buck) const
	{ return cbegin(buck); }

	local_iterator begin(size_type buck)
	{ return cbegin(buck).to_non_const(); }

	const_local_iterator cend(size_type buck) const
	{
		assert(buck < bucket_count());
		return table_[buck].cend();
	}

	const_local_iterator end(size_type buck) const
	{ return cend(buck); }

	local_iterator end(size_type buck)
	{ return cend(buck).to_non_const(); }

	// BUCKET QUANTITIES
	size_type bucket_count() const
	{ return table_size(); }

	size_type max_bucket_count() const
	{ 
		return std::min(
			table_.max_size() - 1,
			((~static_cast<size_type>(0)) >> 1) + 1
		);
	}

	size_type bucket_size(size_type buck) const
	{
		assert(buck < bucket_count());
		return static_cast<size_type>(
			std::distance(cbegin(buck), cend(buck))
		);
	}

	template <class T>
	size_type bucket(const T& value) const
	{ return bucket_index(get_hasher()(value)); }

	/// HASH POLICY ///
	// LOAD FACTOR
	void max_load_factor(float f)
	{
		assert(f > 0.0);
		max_load_factor_ = f;
	}
	
	float max_load_factor() const
	{ return max_load_factor_; }

	float load_factor() const
	{ return static_cast<double>(count_) / table_size(); }
	
	void rehash(size_type nbuckets)
	{
		size_type bcount = bucket_count();
		assert(bcount > 0u);
		auto load_factor_good = [&]() {
			return (static_cast<double>(size()) / bcount) <= max_load_factor_;
		};
		if(nbuckets < bcount)
		{
			while(nbuckets < bcount and load_factor_good())
				bcount /= 2;
			if(bcount == bucket_count())
				return;
			
		}
		else if(nbuckets > bcount)
		{
			while(nbuckets > bcount)
				bcount *= 2;
		}
		while(not load_factor_good())
			bcount *= 2;
		if(bcount > bucket_count())
			grow_table(bcount);
		else if(bcount < bucket_count())
			shrink_table(bcount);
	}

	void reserve(size_type count)
	{
		rehash(static_cast<size_type>(std::ceil(count / max_load_factor())));
	}

	hasher hash_function() const
	{ return get_hasher(); }

	key_equal key_eq() const
	{ return get_key_equal(); }

	friend void swap(self_type& left, self_type& right)
	{ return left.swap(right); }

	friend bool operator==(const self_type& left, const self_type& right)
	{
		if(left.size() != right.size())
			return false;
		if(left.table_size() == right.table_size())
			return std::equal(left.table.begin(), left.table.end(), right.table.begin());
		// iterate over the set with the smaller table.
		const auto& iter_set = (left.table_size() > right.table_size()) ? right : left;
		// search through the set with the larger table
		const auto& search_set = (&left == &iter_set) ? right : left;
		// iterate through 'count' elements to avoid excessive iteration.
		// TODO: if/when the AnySet implementation is changed to one big linked-list, 
		//  change this to a normal range-for
		size_type count = iter_set.size();
		auto pos = iter_set.begin();
		auto stop = iter_set.begin();
		while(true)
		{
			if(search_set.cend() == search_set.find_matching_value(*pos))
				return false;
			--count;
			assert(pos != stop);
			if(count == 0)
			{
				assert(std::next(pos) == stop);
				break;
			}
			++pos;
		}
		return true;
	}

	friend bool operator!=(const self_type& left, const self_type& right)
	{ return not (left == right); }

private:

	const_iterator find_matching_value(const AnyValue& any_v) const
	{
		auto hash_v = any_v.get_hash();
		auto bucket_pos = table_.cbegin() + bucket_index(hash_v);
		auto pos = std::find_if(bucket_pos->begin(), bucket_pos->end(), 
			[=](const auto& v){ return any_v.get_hash() >= hash_v; }
		);
		while((not pos.is_end()) and pos->get_hash() == hash_v)
		{
			if(*pos == any_v)
				return const_iterator(bucket_pos, pos);
		}
		return cend();
	}

	iterator to_non_const_iterator(const_iterator pos)
	{
		auto table_idx = pos.vector_pos_ - table_.cbegin();
		return iterator(table_.begin() + table_idx, pos.list_pos_.to_non_const());
	}

	template <class ... T>
	void arg_insert(T&& ... args)
	{
		preemptive_reserve(sizeof...(args));
		(..., insert_impl<false>(std::forward<T>(args)));
		assert(load_factor() < max_load_factor());
	}

	template <class It>
	void range_insert(It first, It last, std::input_iterator_tag)
	{
		while(first != last)
			insert_impl<true>(*first++);
	}

	template <class It>
	void range_insert(It first, It last, std::forward_iterator_tag)
	{
		preemptive_reserve(std::distance(first, last));
		while(first != last)
			insert_impl<false>(*first++);
		assert(load_factor() < max_load_factor());
	}

	void preemptive_reserve(std::size_t ins_count)
	{
		auto new_count = count_ + ins_count;
		auto new_table_size = table_size();
		auto compute_new_load_factor = [&](){
			return static_cast<double>(new_count) / new_table_size;
		};
		while(compute_new_load_factor() > max_load_factor())
			new_table_size *= 2;
		if(new_table_size > table_size())
			grow_table(new_table_size);
	}

	template <bool CheckLoadFactor, class T>
	std::pair<local_iterator, bool> insert_impl(
		T&& value, 
		std::size_t hash_v, 
		AnyList& buck, 
		std::unique_ptr<AnyValue> existing = nullptr
	)
	{
		using ValueType = std::decay_t<T>;
		auto [pos, found] = find_position(buck, hash_v, static_cast<const ValueType&>(value));
		
			
		if(not found)
		{
			if(not existing)
				existing = std::make_unique<AnyNode<ValueType>>(
					hash_v, std::forward<T>(value)
				);
			++count_;
			if(CheckLoadFactor and load_factor() > max_load_factor())
			{
				--count_;
				grow_table(2 * table_size() * 2);
				++count_;
				auto& new_buck = table_[bucket_index(hash_v)];
				pos = find_position_unsafe(new_buck, hash_v);
				pos = new_buck.splice(pos, std::move(existing));
			}
			else 
			{
				pos = buck.splice(pos, std::move(existing));
			}
		}
		else
		{
			assert(pos != buck.end());
		}
		return std::make_pair(pos, not found);
	}



	void shrink_table(size_type new_size)
	{
		assert(new_size < table_size());
		// new size must always be a power of two
		assert(new_size & (new_size - 1) == 0u);
		size_type mask = new_size - 1;
		for(size_type i = new_size; i < table_size(); ++i)
		{
			auto& list = table_[i];
			while(not list.empty())
			{
				std::unique_ptr<AnyValue> any_v = list.pop_front();
				std::size_t hash_v = any_v->get_hash();
				auto& new_list = table_[hash_v & mask];
				auto pos = std::find_if(new_list.begin(), new_list.end(), 
					[=](const auto& v) { return v.get_hash() > hash_v; }
				);
				new_list.splice(pos, std::move(any_v));
			}
		}
		assert(
			std::all_of(
				table_.begin() + new_size(), 
				table_.begin() + table_size(), 
				[](const auto& v) { return v.empty(); }
			)
		);
		table_.resize(new_size + 1);
		table_.back() = AnyList::get_sentinal();
	}

	void grow_table(size_type new_size)
	{
		assert(new_size > table_size());
		// new size must always be a power of two
		assert(new_size & (new_size - 1) == 0u);
		auto old_size = table_size();
		auto old_mask = old_size - 1;
		auto new_mask = new_size - 1;
		// allocate one extra item for the sentinal.
		// this is the only operation that can fail.
		table_.resize(new_size + 1);
		table_.back() = AnyList::get_sentinal();
		// overwrite the old sentinal
		table_[old_size] = AnyList();
		for(size_type i = 0; i < old_size; ++i)
		{
			auto& list = table_[i];
			auto first = list.begin();
			auto last = list.end();
			while(first != last)
			{
				std::size_t hash_v = first->get_hash();
				auto buck = bucket_index(hash_v);
				if(buck == i) 
				{
					// node can stay in this bucket
					++first;
					continue;
				}
				// move the node to its new bucket
				std::unique_ptr<AnyValue> any_v(list.pop(first));
				auto& dest_list = table_[buck];
				auto pos = std::find_if(dest_list.begin(), dest_list.end(), 
					[=](const auto& node){ return node.get_hash() > hash_v; }
				);
				dest_list.splice(pos, std::move(any_v));
			}
		}
	}

	size_type bucket_index(std::size_t hash) const
	{ return hash & (size(table_) - 1); }

	template <class Value>
	std::pair<const_local_iterator, bool> find_position(const AnyList& buck, std::size_t hash, const Value& value) const
	{
		auto pos = buck.begin();
		auto last = buck.end();
		pos = std::find_if(pos, last, [=](const auto& any_v){ return any_v.get_hash() >= hash; });
		bool match = false;
		auto& key_eql = get_key_equal();
		pos = std::find_if(pos, last,
			[&](const AnyValue& any_v) {
				if(any_v.get_hash() > hash)
					return true; // all following elements have hashes larger than 'hash'
				
				if(auto v = any_v.try_as<Value>(); static_cast<bool>(v))
					return (match = key_eql(*v, value)); // hashes and types match.  check for equality
				else
					return false; // hashes matched but nodes have different types.  continue.
			}
		);
		return std::make_pair(pos, match);
	}

	const_local_iterator find_position_unsafe(const AnyList& buck, std::size_t hash) const
	{
		return std::find_if(buck.begin(), buck.end(), [=](const auto& v){ return v.get_hash() > hash; });
	}

	size_type table_size() const
	{ return table_.size() - 1; }

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

	vector_type table_{AnyList::get_sentinal()};
	std::size_t count_ = 0;
	float max_load_factor_{1.0};
	static constexpr const size_type min_bucket_count_ = 1;
};
	

#endif /* ANY_HASH_H */

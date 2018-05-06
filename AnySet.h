#ifndef ANY_SET_H
#define ANY_SET_H

#include <functional>
#include <cstddef>
#include <cmath>
#include "AnyList.h"
#include <vector>
#include <algorithm>
#include <numeric>
#include <type_traits>
#include "AnyHash.h"

#include <iostream>

namespace te {




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

template <class Hash = AnyHash, class KeyEqual = std::equal_to<>, class Allocator = std::allocator<AnyValue<Hash, KeyEqual>>>
struct AnySet:
	private std::tuple<Hash, KeyEqual> // libstdc++ and libc++ have EBO for std::tuple
{
private:
	using self_type = AnySet<Hash, KeyEqual, Allocator>;
	using list_type = AnyList<Hash, KeyEqual>;
	using table_allocator = typename std::allocator_traits<Allocator>::template rebind_alloc<list_type>;
	using vector_type = std::vector<list_type, table_allocator>;
	
	using node_base_type = typename list_type::node_base_type;

	using vector_iterator = typename vector_type::iterator;
	using const_vector_iterator = typename vector_type::const_iterator;

	template <class T>
	using node_type = typename list_type::template node_type<T>;
public:
	using value_type = typename list_type::value_type;
	using size_type = typename vector_type::size_type;
	using difference_type = typename vector_type::difference_type;
	using key_equal = KeyEqual;
	using hasher = Hash;
	using allocator_type = typename vector_type::allocator_type;
	using key_type = value_type;
	using reference = value_type&;
	using const_reference = const value_type&;
	using pointer = value_type*;
	using const_pointer = const value_type*;
	using node_handle = std::unique_ptr<value_type>;
	// struct node_handle:
	// 	public std::unique_ptr<value_type>
	// {
	// 	using std::unique_ptr<value_type>::unique_ptr;

	// 	template <class T, class D>
	// 	node_handle(std::unique_ptr<node_type<T>, D> other):
	// 		std::unique_ptr<value_type>(static_cast<value_type*>(other.release()))
	// 	{
	// 		
	// 	}

	// 	
	// };

	template <bool IsConst>
	struct Iterator
	{
	private:
		using list_iterator = std::conditional_t<IsConst,
			typename list_type::const_iterator,
			typename list_type::iterator
		>;
		using vector_iterator = std::conditional_t<IsConst,
			typename vector_type::const_iterator,
			typename vector_type::iterator
		>;
	public:
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
			vector_pos_(other.vector_pos_), list_pos_(other.list_pos_)
		{
			
		}

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
		{ return /* (left.vector_pos_ == right.vector_pos_) and */ (left.list_pos_ == right.list_pos_); }

		friend bool operator!=(const Iterator& left, const Iterator& right)
		{ return not (left == right); }

		reference operator*() const
		{ return *list_pos_; }

		pointer operator->() const
		{ return std::addressof(*list_pos_); }
	
	private:
		Iterator(vector_iterator v_pos, list_iterator l_pos):
			vector_pos_(v_pos), list_pos_(l_pos)
		{
			
		}

		std::size_t erase()
		{
			list_pos_ = list_type::erase(list_pos_);
			ensure_valid();
			return 1;
		}

		std::size_t erase_range(Iterator<IsConst> last)
		{
			std::size_t count = 0;
			assert(not vector_pos_->empty());
			assert(not last.vector_pos_->empty());
			// if(vector_pos_ != last.vector_pos_)
			// {
			// 	assert(vector_pos_ < last.vector_pos_);
			// 	auto [_, qty] = list_type::erase_to_end(list_pos_);
			// 	for(auto pos = vector_pos_ + 1; pos != last.vector_pos_; ++pos)
			// 		count += pos->clear();
			// 	vector_pos_ = last.vector_pos_;
			// 	list_pos_ = vector_pos_->begin();
			// }
			// auto [pos, qty] = list_type::erase(list_pos_, last.list_pos_);
			// count += qty;
			// list_pos_ = pos;
			// ensure_valid();
			while(true)
			{
				auto after = std::next(*this);
				bool last_one = (after == last);
				count += erase();
				if(last_one)
					break;
			}
			return count;
		}

		void ensure_valid()
		{
			if(list_pos_.is_end())
			{
				do {
					++vector_pos_;
				} while(vector_pos_->empty());
				list_pos_ = vector_pos_->begin();
			}
		}
		vector_iterator vector_pos_;
		list_iterator list_pos_;
		friend class Iterator<not IsConst>;
		friend class AnySet;
	};

public:
	using iterator = Iterator<false>;
	using const_iterator = Iterator<true>;
	using local_iterator = typename list_type::iterator;
	using const_local_iterator = typename list_type::const_iterator;

private:
	static size_type next_highest_pow2(size_type n)
	{
		size_type i = 1;
		while(i <= n)
		{
			assert(i * 2 > i);
			i *= 2;
		}
		return i;
	}

	allocator_type alloc_socca() const
	{
		return std::allocator_traits<allocator_type>::select_on_container_copy_construction(
			get_allocator()
		);
	}

public:	
	void _assert_invariants() const
	{
		assert(table_size() > 0u);
		assert(((table_size() & (table_size() - 1)) == 0u) and "Table size not a power of two.");
		assert(std::none_of(table_.cbegin(), std::prev(table_.cend()), std::mem_fn(&list_type::is_sentinal)));
		assert(table_.back().is_sentinal());
		assert(max_load_factor_ > 0.0);
		auto iter_dist = std::distance(cbegin(), cend());
		assert(iter_dist >= 0u);
		assert(static_cast<size_type>(iter_dist) == count_);
		auto acc_list_size = [](size_type count, const list_type& list) {
			size_type sz = 0;
			for(const auto& _: list)
			{
				(void)_;
				++sz;
			}
			return count + sz;
		};
		assert(std::accumulate(table_.cbegin(), std::prev(table_.cend()), size_type(0), acc_list_size) == count_);
		auto list_is_sorted = [](const auto& list) {
			return std::is_sorted(list.begin(), list.end(), 
				[](const value_type& left, const value_type& right) {
					return left.hash < right.hash;
				}
			);
		};
		assert(std::all_of(table_.cbegin(), table_.cend() - 1, list_is_sorted));
	}

	/// CONSTRUCTORS ///
	AnySet(): AnySet(size_type(0)) { }

	explicit AnySet(
		size_type bucket_count,
		const Hash& hash = Hash(),
		const KeyEqual& equal = KeyEqual(),
		const Allocator& alloc = Allocator()
	):
		std::tuple<Hash, KeyEqual>(hash, equal),
		table_(next_highest_pow2(bucket_count) + 1, allocator_type(alloc))
	{
		table_.back() = list_type::get_sentinal();
		_assert_invariants();
	}

	AnySet(size_type bucket_count, const Allocator& alloc):
		AnySet(bucket_count, Hash(), KeyEqual(), alloc) 
	{
		_assert_invariants();
	}
	
	AnySet(size_type bucket_count, const Hash& hash, const Allocator& alloc):
		AnySet(bucket_count, hash, KeyEqual(), alloc)
	{
		_assert_invariants();
	}

	template <class InputIt>
	AnySet(
		InputIt first, 
		InputIt last,
		size_type bucket_count = 0,
		const Hash& hash = Hash(),
		const KeyEqual& equal = KeyEqual(),
		const Allocator& alloc = Allocator()
	):
		AnySet(bucket_count, hash, equal, alloc)
	{
		insert(first, last);
		_assert_invariants();
	}

	template <class InputIt>
	AnySet(
		InputIt first, 
		InputIt last,
		size_type bucket_count,
		const Allocator& alloc
	):
		AnySet(first, last, bucket_count, Hash(), KeyEqual(), alloc)
	{
		_assert_invariants();
	}

	template <class InputIt>
	AnySet(
		InputIt first, 
		InputIt last,
		size_type bucket_count,
		const Hash& hash,
		const Allocator& alloc
	):
		AnySet(first, last, bucket_count, hash, KeyEqual(), alloc)
	{
		_assert_invariants();
	}

	AnySet(const self_type& other, const Allocator& alloc):
		std::tuple<Hash, KeyEqual>(other.as_tuple()), table_(0, alloc)
	{
		table_.resize(other.table_.size());
		std::transform(other.table_.begin(), other.table_.end(), table_.begin(), 
			[](const auto& list) { return list_type(list, list_type::make_copy); }
		);
		count_ = other.count_;
		max_load_factor_ = other.max_load_factor_;
		table_.back() = list_type::get_sentinal();
		_assert_invariants();
	}

	AnySet(const self_type& other):
		std::tuple<Hash, KeyEqual>(other.as_tuple()), table_(0, other.alloc_socca())
	{
		table_.resize(other.table_.size());
		std::transform(other.table_.begin(), other.table_.end(), table_.begin(), 
			[](const auto& list) { return list_type(list, list_type::make_copy); }
		);
		count_ = other.count_;
		max_load_factor_ = other.max_load_factor_;
		table_.back() = list_type::get_sentinal();
		_assert_invariants();
	}

	AnySet& operator=(const self_type& other)
	{ return *this = std::move(self_type(other)); }

	AnySet& operator=(self_type&& other) noexcept(std::is_nothrow_move_assignable_v<vector_type>)
	{
		table_ = std::move(other.table_);
		count_ = other.count_;
		max_load_factor_ = other.max_load_factor_;
		table_.back() = list_type::get_sentinal();
		_assert_invariants();
		return *this;
	}

	template <class T>
	AnySet(
		std::initializer_list<T> ilist,
		size_type bucket_count = 0,
		const Hash& hash = Hash(),
		const KeyEqual& equal = KeyEqual(),
		const Allocator& alloc = Allocator()
	):
		AnySet(ilist.begin(), ilist.end(), bucket_count, hash, equal, alloc)
	{
		_assert_invariants();
	}

	template <class T>
	AnySet(
		std::initializer_list<T> ilist,
		size_type bucket_count,
		const Allocator& alloc
	):
		AnySet(ilist, bucket_count, Hash(), KeyEqual(), alloc)
	{
		_assert_invariants();
	}

	template <class T>
	AnySet(
		std::initializer_list<T> ilist,
		size_type bucket_count,
		const Hash& hash,
		const Allocator& alloc
	):
		AnySet(ilist, bucket_count, hash, KeyEqual(), alloc)
	{
		_assert_invariants();
	}

	template <class ... T>
	AnySet(
		std::tuple<T ...>&& tup,
		size_type bucket_count = 2 * (sizeof...(T)),
		const Hash& hash = Hash(),
		const KeyEqual& equal = KeyEqual(),
		const Allocator& alloc = Allocator()
	):
		AnySet(bucket_count, hash, equal, alloc)
	{
		std::apply(
			[&](auto&& ... args) { 
				this->arg_insert(std::forward<decltype(args)>(args)...); 
			},
			std::move(tup)
		);
		_assert_invariants();
	}

	template <class ... T>
	AnySet(
		std::tuple<T ...>&& tup,
		size_type bucket_count,
		const Allocator& alloc
	):
		AnySet(std::move(tup), bucket_count, Hash(), KeyEqual(), alloc)
	{
		_assert_invariants();
	}

	template <class ... T>
	AnySet(
		std::tuple<T ...> tup,
		size_type bucket_count,
		const Hash& hash,
		const Allocator& alloc
	):
		AnySet(std::move(tup), bucket_count, hash, KeyEqual(), alloc)
	{
		_assert_invariants();
	}

	/// ITERATORS ///
	// BEGIN
	const_iterator cbegin() const
	{
		if(empty())
			return cend();
		auto table_pos = std::find_if(
			table_.cbegin(),
			table_.cend() - 1,
			std::not_fn(std::mem_fn(&list_type::empty))
		);
		return const_iterator(table_pos, table_pos->cbegin());
	}

	const_iterator begin() const
	{ return cbegin(); }

	iterator begin() 
	{ return to_non_const_iterator(cbegin()); }

	// END
	const_iterator cend() const
	{
		auto pos = const_iterator(table_.cend() - 1, table_.back().cbegin());
		assert(pos.vector_pos_->is_sentinal());
		return pos;
	}

	const_iterator end() const
	{ return cend(); }

	iterator end()
	{ return to_non_const_iterator(cend()); }

	/// CAPACITY AND SIZE ///
	size_type size() const
	{ return count_; }

	bool empty() const
	{ return not static_cast<bool>(size()); }

	bool max_size() const
	{ return std::numeric_limits<size_type>::max(); }


	/// MODIFIERS ///
	// CLEAR
	void clear()
	{
		for(size_type i = 0, len = table_size(); i < len; ++i)
			table_[i].clear();
		count_ = 0;
		_assert_invariants();
	}
	
	// EMPLACEMENT
	template <class T, class ... Args>
	std::pair<iterator, bool> emplace(Args&& ... args)
	{
		auto node(std::make_unique<node_type<T>>(get_hasher(), std::forward<Args>(args)...));
		const T& value = node->value();
		auto hash_v = get_hasher()(value);
		assert(hash_v == node->hash);
		auto bucket_pos = table_.begin() + bucket_index(hash_v);
		return insert_impl<true>(
			value, hash_v, bucket_pos, [&](){ return node_handle(node.release()); }()
		);
	}
	
	template <class T, class ... Args>
	std::pair<iterator, bool> emplace_hint(const_iterator, Args&& ... args)
	{ return emplace<T>(std::forward<Args>(args)...); }

	// INSERTION
	template <class T>
	std::pair<iterator, bool> insert(T&& value)
	{
		auto hash_v = get_hasher()(value);
		auto bucket_pos = table_.begin() + bucket_index(hash_v);
		return insert_impl<true>(std::forward<T>(value), hash_v, bucket_pos);
	}

	template <class T>
	std::pair<iterator, bool> insert(const_iterator, T&& value)
	{ return insert(std::forward<T>(value)); }

	template <class It, class = std::enable_if_t<
			std::is_copy_constructible_v<typename std::iterator_traits<It>::value_type>
			or std::is_rvalue_reference_v<decltype(*std::declval<It>())>
		>
	>
	void insert(It first, It last)
	{
		using iter_cat = typename std::iterator_traits<It>::iterator_category;
		range_insert(first, last, iter_cat{});
		_assert_invariants();
	}

	template <
		class T, 
		class U, 
		class ... V,
		class = std::enable_if_t<
			(not (std::is_same_v<std::decay_t<T>, std::decay_t<U>> and detail::is_iterator_v<std::decay_t<T>>))
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
		_assert_invariants();
	}
	
	template <class T, class = std::enable_if_t<std::is_copy_constructible_v<T>>>
	void insert(std::initializer_list<T> ilist)
	{
		insert(ilist.begin(), ilist.end());
		_assert_invariants();
	}

	// ERASURE
	iterator erase(const_iterator pos)
	{
		count_ -= pos.erase();
		_assert_invariants();
		return to_non_const_iterator(pos);
	}

	iterator erase(const_iterator first, const_iterator last)
	{
		count_ -= to_non_const_iterator(first).erase_range(to_non_const_iterator(last));
		_assert_invariants();
		return to_non_const_iterator(first);
	}

	template <class T, class = std::enable_if_t<not (std::is_same_v<const_iterator, T> or std::is_same_v<iterator, T>)>>
	size_type erase(const T& value)
	{
		auto hash_v = get_hasher()(value);
		auto bucket_pos = table_.begin() + bucket_index(hash_v);
		auto [list_pos, found] = find_position(*bucket_pos, hash_v, value);
		assert(list_pos.is_end() ? not found : true);
		if(found)
			erase(const_iterator(bucket_pos, list_pos));
		_assert_invariants();
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
		_assert_invariants();
		other._assert_invariants();
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
		auto bucket_pos = table_.cbegin() + bucket_index(hash_v);
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
			static_cast<const self_type*>(this)->find(value)
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
		auto [first, last] = static_cast<const self_type*>(this)->equal_range(value);
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
		size_type maxm = ~((~size_type(0)) >> 1);
		while(maxm > table_.max_size())
			maxm >>= 1;
		return maxm;
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
		_assert_invariants();
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
			if((bcount == bucket_count()) and load_factor_good())
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
		_assert_invariants();
	}

	void reserve(size_type count)
	{
		rehash(static_cast<size_type>(std::ceil(count / max_load_factor())));
		_assert_invariants();
	}

	hasher hash_function() const
	{ return get_hasher(); }

	key_equal key_eq() const
	{ return get_key_equal(); }

	allocator_type get_allocator() const
	{ return table_.get_allocator(); }

	friend void swap(self_type& left, self_type& right)
	{ return left.swap(right); }

	friend bool operator==(const self_type& left, const self_type& right)
	{
		if(left.size() != right.size())
			return false;
		else if(left.table_size() == right.table_size())
			return std::equal(left.table_.begin(), left.table_.end() - 1, right.table_.begin());
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
		auto comp = [&](const value_type& l, const value_type& r){ return l == r; };
		// return std::is_permutation(left.begin(), left.end(), right.begin(), right.end(), comp);
		for(const auto& item: iter_set)
		{
			if(search_set.cend() == search_set.find_matching_value(item, comp))
				return false;
		}
		// while(true)
		// {
		// 	if(search_set.cend() == search_set.find_matching_value(*pos, comp))
		// 		return false;
		// 	--count;
		// 	assert(pos != stop);
		// 	if(count == 0)
		// 	{
		// 		assert(std::next(pos) == stop);
		// 		break;
		// 	}
		// 	++pos;
		// }
		left._assert_invariants();
		right._assert_invariants();
		return true;
	}

	friend bool operator!=(const self_type& left, const self_type& right)
	{ return not (left == right); }


	self_type& update(const self_type& other)
	{
		preemptive_reserve(other.size());
		for(size_type i = 0, len = other.table_size(); i < len; ++i)
		{
			const auto& list = other.table_[i];
			for(const auto& any_v: list)
			{
				auto [pos, found] = find_insertion_position(any_v);
				if(not found)
					continue;
				unsafe_splice_at(pos, any_v.clone());
			}
		}
		assert(load_factor_satisfied());
		_assert_invariants();
		other._assert_invariants();
		return *this;
	}

	self_type& update(self_type&& other)
	{
		preemptive_reserve(other.size());
		for(auto table_pos = table_.begin(), stop = (table_.end() - 1); table_pos != stop; ++table_pos)
		{
			for(auto list_pos = table_pos->begin(); not list_pos.is_end(); ++list_pos)
			{
				auto [ins_pos, found] = find_insertion_position(*list_pos);
				if(not found)
					continue;
				unsafe_splice_at(ins_pos, other.pop(const_iterator(table_pos, list_pos)));
			}
		}
		assert(load_factor_satisfied());
		_assert_invariants();
		other._assert_invariants();
		return *this;
	}

	std::pair<node_handle, iterator> pop(const_iterator pos)
	{
		auto any_v = list_type::pop(pos.list_pos_);
		pos.ensure_valid();
		--count_;
		_assert_invariants();
		return std::make_pair(std::move(any_v), to_non_const_iterator(pos)); 
	}

	node_handle dup(const_iterator pos) const
	{ return pos->clone(); }

	std::pair<iterator, node_handle> push(node_handle node)
	{
		auto [pos, found] = find_insertion_position(*node);
		if(not found)
			return std::make_pair(to_non_const_iterator(pos), std::move(node));
		pos = safely_splice_at(pos, std::move(node), node->hash);
		_assert_invariants();
		return std::make_pair(to_non_const_iterator(pos), node_handle(nullptr));
	}

	std::tuple<iterator, iterator, bool> splice(self_type& other, const_iterator pos)
	{ return splice_or_copy(std::move(other), pos); }

	std::pair<iterator, iterator> splice(self_type& other, const_iterator first, const_iterator last)
	{ return splice_or_copy(std::move(other), first, last); }

	template <class T, class = std::enable_if_t<std::is_same_v<std::decay_t<T>, self_type>>>
	std::pair<iterator, iterator> splice_or_copy(
		T&& other, const_iterator first, const_iterator last
	)
	{
		preemptive_reserve(std::distance(first, last));
		auto pos = first;
		while(pos != last)
			std::tie(std::ignore, pos, std::ignore) = splice_or_copy(std::forward<T>(other), pos);
		_assert_invariants();
		other._assert_invariants();
		return std::make_pair(first, pos);
	}

	template <class T, class = std::enable_if_t<std::is_same_v<std::decay_t<T>, self_type>>>
	auto splice_or_copy(T&& other, const_iterator pos)
		-> std::tuple<iterator, decltype(other.begin()), bool>
	{
		auto hash_v = pos->hash;
		auto [ins_pos_, success] = find_insertion_position(*pos, hash_v);
		auto ins_pos = to_non_const_iterator(ins_pos_);
		if constexpr(std::is_rvalue_reference_v<decltype(other)>)
		{
			if(not success)
				return std::make_tuple(
					ins_pos, other.to_non_const_iterator(std::next(pos)), false
				);
			auto [node, next] = other.pop(pos);
			try
			{
				return std::make_tuple(
					safely_splice_at(ins_pos, std::move(node), hash_v), next, true
				);
			}
			catch(const std::bad_alloc& e)
			{
				// put back the node if didn't make it in
				if(node)
					other.unsafe_splice_at(pos, std::move(node));
				throw e;
			}
		}
		else
		{
			if(not success)
				return std::make_tuple(ins_pos, std::next(pos), false);
			return std::make_tuple(
				safely_splice_at(ins_pos, other.dup(pos), hash_v), std::next(pos), true
			);
		}
	}

	bool contains_value(const value_type& any_v) const
	{ return find_matching_value(any_v, get_key_equal()) != cend(); }

	template <class T, class = std::enable_if_t<not std::is_same_v<value_type, T>>>	
	bool contains(const T& value) const
	{ return count(value); }
	
private:
	bool load_factor_satisfied() const
	{ return load_factor() <= max_load_factor(); }

	iterator safely_splice_at(const_iterator pos, node_handle node, std::size_t hash_v)
	{
		++count_;
		if(not load_factor_satisfied())
		{
			--count_;
			grow_table(2 * table_size());
			++count_;
			auto buck_pos = table_.begin() + bucket_index(hash_v);
			auto list_pos = std::find_if(
				buck_pos->begin(), 
				buck_pos->end(), 
				[=](const auto& v){ return v.hash > hash_v; }
			);
			list_pos = list_type::splice(list_pos, std::move(node));
			_assert_invariants();
			return iterator(buck_pos, list_pos);
		}
		else 
		{
			auto list_pos = list_type::splice(pos.list_pos_, std::move(node));
			_assert_invariants();
			return to_non_const_iterator(const_iterator(pos.vector_pos_, list_pos));
		}
	}
	
	iterator unsafe_splice_at(const_iterator pos, node_handle node)
	{
		++count_;
		pos.list_pos_ = list_type::splice(pos.list_pos_, std::move(node));
		_assert_invariants();
		return to_non_const_iterator(pos);
	}

	local_iterator unsafe_splice_at(const_local_iterator pos, node_handle node)
	{
		++count_;
		pos = list_type::splice(pos, std::move(node));
		_assert_invariants();
		return pos.to_non_const();
	}

	std::pair<const_iterator, bool> find_insertion_position(const value_type& any_v) const
	{ return find_insertion_position(any_v, any_v.hash); }

	std::pair<const_iterator, bool> find_insertion_position(const value_type& any_v, std::size_t hash_v) const
	{
		assert(hash_v == any_v.hash);
		auto bucket_pos = table_.cbegin() + bucket_index(hash_v);
		auto pos = std::find_if(bucket_pos->begin(), bucket_pos->end(), 
			[&](const auto& v){ return v.hash >= hash_v; }
		);
		for(; (not pos.is_end()) and pos->hash == hash_v; ++pos)
		{
			if(equal_values(*pos, any_v))
				return std::make_pair(const_iterator(bucket_pos, pos), false);
		}
		return std::make_pair(const_iterator(bucket_pos, pos), true);
	}

	template <class Compare>
	const_iterator find_matching_value(const value_type& any_v, Compare comp) const
	{
		const std::size_t hash_v = any_v.hash;
		auto bucket_pos = table_.cbegin() + bucket_index(hash_v);
		auto pos = std::find_if(bucket_pos->begin(), bucket_pos->end(), 
			[&](const value_type& v){ return v.hash >= hash_v; }
		);
		for(; (not pos.is_end()) and (pos->hash == hash_v); ++pos)
		{
			if(comp(*pos, any_v))
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
		assert(load_factor() <= max_load_factor());
		_assert_invariants();
	}

	template <class It>
	void range_insert(It first, It last, std::input_iterator_tag)
	{
		while(first != last)
			insert_impl<true>(*first++);
		_assert_invariants();
	}

	template <class It, class = std::enable_if_t<std::is_copy_constructible_v<typename std::iterator_traits<It>::value_type>>>
	void range_insert(It first, It last, std::forward_iterator_tag)
	{
		preemptive_reserve(std::distance(first, last));
		while(first != last)
			insert_impl<false>(*first++);
		assert(load_factor() <= max_load_factor());
		_assert_invariants();
	}

	void preemptive_reserve(std::size_t ins_count)
	{
		auto new_count = count_ + ins_count;
		size_type new_table_size = table_size();
		assert(new_table_size > 0u);
		auto compute_new_load_factor = [&](){
			return (static_cast<double>(new_count) / new_table_size);
		};
		while(compute_new_load_factor() > max_load_factor())
			new_table_size *= 2;
		if(new_table_size > table_size())
			grow_table(new_table_size);
		_assert_invariants();
	}

	template <bool CheckLoadFactor, class T>
	std::pair<iterator, bool> insert_impl(T&& value)
	{
		auto hash_v = get_hasher()(value);
		return insert_impl<CheckLoadFactor>(std::forward<T>(value), hash_v, table_.begin() + bucket_index(hash_v));
	}
	
	template <bool CheckLoadFactor, class T>
	std::pair<iterator, bool> insert_impl(
		T&& value, 
		std::size_t hash_v, 
		vector_iterator buck, 
		node_handle existing = nullptr
	)
	{
		using ValueType = std::decay_t<T>;
		iterator ins_pos;
		auto [pos, found] = find_position(*buck, hash_v, static_cast<const ValueType&>(value));
		
		if(not found)
		{
			if constexpr(std::is_copy_constructible_v<std::decay_t<T>>)
			{
				if(not existing)
					existing = node_handle(
						static_cast<node_base_type*>(std::make_unique<node_type<ValueType>>(
							hash_v, std::forward<T>(value)
						).release())
					);
			}
			else
			{
				assert(existing);
			}

			if constexpr(CheckLoadFactor)
			{
				ins_pos = safely_splice_at(
					const_iterator(buck, pos), 
					std::move(existing), 
					hash_v
				);
			}
			else
			{
				ins_pos = iterator(buck, unsafe_splice_at(pos, std::move(existing)));
			}
		}
		else
		{
			ins_pos = iterator(buck, pos.to_non_const());
			assert(pos != buck->end());
		}
		_assert_invariants();
		return std::make_pair(ins_pos, not found);
	}

	void shrink_table(size_type new_size)
	{
		assert(new_size < table_size());
		// new size must always be a power of two
		assert((new_size & (new_size - 1)) == 0u);
		size_type mask = new_size - 1;
		for(size_type i = new_size; i < table_size(); ++i)
		{
			auto& list = table_[i];
			while(not list.empty())
			{
				node_handle any_v = list.pop_front();
				std::size_t hash_v = any_v->hash;
				auto& new_list = table_[hash_v & mask];
				auto pos = std::find_if(new_list.begin(), new_list.end(), 
					[=](const auto& v) { return v.hash > hash_v; }
				);
				list_type::splice(pos, std::move(any_v));
			}
		}
		assert(
			std::all_of(
				table_.begin() + new_size, 
				table_.begin() + table_size(), 
				[](const auto& v) { return v.empty(); }
			)
		);
		table_.resize(new_size + 1);
		table_.back() = list_type::get_sentinal();
		_assert_invariants();
	}

	void grow_table(size_type new_size)
	{
		assert(new_size > table_size());
		// new size must always be a power of two
		assert((new_size & (new_size - 1)) == 0u);
		auto old_size = table_size();
		// auto old_mask = old_size - 1;
		// auto new_mask = new_size - 1;
		// allocate one extra item for the sentinal.
		// this is the only operation that can fail.
		table_.resize(new_size + 1);
		table_.back() = list_type::get_sentinal();
		// overwrite the old sentinal
		table_[old_size] = list_type();
		for(size_type i = 0; i < old_size; ++i)
		{
			auto& list = table_[i];
			auto first = list.begin();
			auto last = list.end();
			while(first != last)
			{
				std::size_t hash_v = first->hash;
				auto buck = bucket_index(hash_v);
				if(buck == i) 
				{
					// node can stay in this bucket
					++first;
					continue;
				}
				// move the node to its new bucket
				node_handle any_v(list_type::pop(first));
				auto& dest_list = table_[buck];
				auto pos = std::find_if(dest_list.begin(), dest_list.end(), 
					[=](const auto& v){ return v.hash > hash_v; }
				);
				dest_list.splice(pos, std::move(any_v));
			}
		}
		_assert_invariants();
	}

	size_type bucket_index(std::size_t hash) const
	{ return hash & (table_size() - 1); }

	template <class Value>
	std::pair<const_local_iterator, bool> find_position(const list_type& buck, const std::size_t hash_v, const Value& value) const
	{
		auto pos = buck.begin();
		auto last = buck.end();
		pos = std::find_if(pos, last, [=](const auto& v){ return v.hash >= hash_v; });
		bool match = false;
		auto& key_eql = get_key_equal();
		pos = std::find_if(pos, last,
			[&](const value_type& any_v) {
				if(any_v.hash > hash_v)
				{
					// all following elements have hashes larger than 'hash'
					return true;
				}
				assert(any_v.hash == hash_v);
				if(const Value* v = try_as<Value>(any_v); static_cast<bool>(v) and key_eql(*v, value))
				{
					// hashes and types match.  check for equality
					match = true;
					return true;
				}
				else
				{
					// hashes matched but values did not
					return false;
				}
			}
		);
		_assert_invariants();
		return std::make_pair(pos, match);
	}

	const_local_iterator find_position_unsafe(const list_type& buck, std::size_t hash_v) const
	{
		return std::find_if(buck.begin(), buck.end(), [=](const auto& v){ return v.hash > hash_v; });
	}

	size_type table_size() const
	{ return table_.size() - 1; }

	bool equal_values(const value_type& left, const value_type& right) const
	{ return left.compare_to(right, get_key_equal()); }

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
public:
	vector_type table_{list_type::get_sentinal()};
	std::size_t count_ = 0;
	float max_load_factor_{1.0};
	static constexpr const size_type min_bucket_count_ = 1;
};




} /* namespace te */	

#endif /* ANY_SET_H */

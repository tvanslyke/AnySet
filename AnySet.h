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


template <class T>
struct CompressedPairBase:
	private detail::ValueHolder<T>
{
	template <class ... Args>
	CompressedPairBase(Args&& ... args):
		detail::ValueHolder<T>(std::forward<Args>(args)...)
	{
		
	}

	const T& first() const &
	{ return static_cast<const detail::ValueHolder<T>&>(*this).value(); }

	const T&& first() const &&
	{ return std::move(static_cast<const detail::ValueHolder<T>&&>(*this).value()); }

	T& first() &
	{ return static_cast<detail::ValueHolder<T>&>(*this).value(); }

	T&& first() &&
	{ return std::move(static_cast<detail::ValueHolder<T>&&>(*this).value()); }

};

} /* namespace detail */

template <class T, class U>
struct CompressedPair:
	public detail::CompressedPairBase<T>,
	private detail::ValueHolder<U>
{
private:
	using left_base = detail::CompressedPairBase<T>;
	using right_base = detail::ValueHolder<U>;
	using self_type = CompressedPair<T, U>;
public:
	constexpr CompressedPair() = default;

	template <
		class Left,
		class Right,
		class = std::enable_if_t<std::is_same_v<std::decay_t<Left>, T>>,
		class = std::enable_if_t<std::is_same_v<std::decay_t<Right>, U>>
	>
	CompressedPair(Left&& left, Right&& right):
		left_base(std::forward<Left>(left)), right_base(std::forward<Right>(right))
	{
		
	}
	
	using left_base::first;

	const U& second() const &
	{ return static_cast<const detail::ValueHolder<U>&>(*this).value(); }

	const U&& second() const &&
	{ return std::move(static_cast<const detail::ValueHolder<U>&&>(*this).value()); }

	U& second() &
	{ return static_cast<detail::ValueHolder<U>&>(*this).value(); }

	U&& second() &&
	{ return std::move(static_cast<detail::ValueHolder<U>&&>(*this).value()); }

	void swap(CompressedPair& other) 
		noexcept(std::is_nothrow_swappable_v<T> and std::is_nothrow_swappable_v<U>)
	{
		using std::swap;
		swap(first(), other.first());
		swap(second(), other.second());
	}
};

template <class T, class U>
CompressedPair<std::decay_t<T>, std::decay_t<U>> make_compressed_pair(T&& left, U&& right)
{
	return CompressedPair<std::decay_t<T>, std::decay_t<U>>(
		std::forward<T>(left), std::forward<U>(right)
	);
}

template <class T, class U>
void swap(CompressedPair<T, U>& left, CompressedPair<T, U>& right) noexcept(noexcept(left.swap(right)))
{ left.swap(right); }

/** 
 * 
 *
 */
template <class Hash = AnyHash, class KeyEqual = std::equal_to<>, class Allocator = std::allocator<AnyValue<Hash, KeyEqual>>>
struct AnySet:
	private CompressedPair<Hash, KeyEqual>
{
private:
	using self_type = AnySet<Hash, KeyEqual, Allocator>;
	using list_type = AnyList<Hash, KeyEqual>;
	using table_allocator = typename std::allocator_traits<Allocator>::template rebind_alloc<list_type>;
	using vector_type = std::vector<list_type, table_allocator>;

	using pair_type = CompressedPair<Hash, KeyEqual>;
	
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
			while(true)
			{
				auto after = std::next(*this);
				bool is_last_one = (after == last);
				count += erase();
				if(is_last_one)
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

	/**
	 * @brief Constructs an empty set with 1 bucket.  Sets max_load_factor() to 1.0.
	 */
	AnySet(): AnySet(size_type(0)) { }

	/**
	 * @brief Construct an empty AnySet instance.  Sets max_load_factor() to 1.0.
	 * @param bucket_count - Minimum number of buckets to initialize the set with.
	 * @param hash         - Hash function to initialize the set with.
	 * @param equal        - Equality comparison function to initialize the set with.
	 * @param alloc        - Allocator to initialize the set with.
	 */
	explicit AnySet(
		size_type bucket_count,
		const Hash& hash = Hash(),
		const KeyEqual& equal = KeyEqual(),
		const Allocator& alloc = Allocator()
	):
		pair_type(hash, equal),
		table_(next_highest_pow2(bucket_count) + 1, allocator_type(alloc))
	{
		table_.back() = list_type::get_sentinal();
	}

	/**
	 * @brief Construct an empty AnySet instance.  Sets max_load_factor() to 1.0.
	 * @param bucket_count - Minimum number of buckets to initialize the set with.
	 * @param alloc        - Allocator to initialize the set with.
	 */
	AnySet(size_type bucket_count, const Allocator& alloc):
		AnySet(bucket_count, Hash(), KeyEqual(), alloc) 
	{
		
	}
	
	/**
	 * @brief Construct an empty AnySet instance.  Sets max_load_factor() to 1.0.
	 * @param bucket_count - Minimum number of buckets to initialize the set with.
	 * @param hash         - Hash function to initialize the set with.
	 * @param alloc        - Allocator to initialize the set with.
	 */
	AnySet(size_type bucket_count, const Hash& hash, const Allocator& alloc):
		AnySet(bucket_count, hash, KeyEqual(), alloc)
	{
		
	}

	/**
	 * @brief Construct an AnySet instance from the range [first, last). 
	 *        Sets max_load_factor() to 1.0.  If multiple elements in the
	 *        range compare equivalent, only the first encountered is inserted.
	 * @param first        - Iterator to the first element in the range.
	 * @param last         - Iterator one position past the last element in the range.
	 * @param bucket_count - Minimum number of buckets to initialize the set with.
	 * @param hash         - Hash function to initialize the set with.
	 * @param equal        - Equality comparison function to initialize the set with
	 * @param alloc        - Allocator to initialize the set with.
	 */
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
	}

	/**
	 * @brief Construct an AnySet instance from the range [first, last). 
	 *        Sets max_load_factor() to 1.0.  If multiple elements in the
	 *        range compare equivalent, only the first encountered is inserted.
	 * @param first        - Iterator to the first element in the range.
	 * @param last         - Iterator one position past the last element in the range.
	 * @param bucket_count - Minimum number of buckets to initialize the set with.
	 * @param alloc        - Allocator to initialize the set with.
	 */
	template <class InputIt>
	AnySet(
		InputIt first, 
		InputIt last,
		size_type bucket_count,
		const Allocator& alloc
	):
		AnySet(first, last, bucket_count, Hash(), KeyEqual(), alloc)
	{
		
	}

	/**
	 * @brief Construct an AnySet instance from the range [first, last). 
	 *        Sets max_load_factor() to 1.0.  If multiple elements in the
	 *        range compare equivalent, only the first encountered is inserted.
	 * @param first        - Iterator to the first element in the range.
	 * @param last         - Iterator one position past the last element in the range.
	 * @param bucket_count - Minimum number of buckets to initialize the set with.
	 * @param hash         - Hash function to initialize the set with.
	 * @param alloc        - Allocator to initialize the set with.
	 */
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
		
	}

	/**
	 * @brief Copy constructs an AnySet instance from other.
	 *        Constructs the set with the copy of the contents of other.  
	 *        Copies the load factor, the predicate, and the hash function as well.
	 * @param other - The set whose contents will be copied.
	 * @param alloc - Allocator to initialize the set with.
	 */
	AnySet(const AnySet& other, const Allocator& alloc):
		pair_type(other.as_pair()), table_(0, alloc)
	{
		table_.resize(other.table_.size());
		std::transform(other.table_.begin(), other.table_.end(), table_.begin(), 
			[](const auto& list) { return list_type(list, list_type::make_copy); }
		);
		count_ = other.count_;
		max_load_factor_ = other.max_load_factor_;
		table_.back() = list_type::get_sentinal();
		
	}

	/**
	 * @brief Copy constructs an AnySet instance from other.
	 *        Constructs the set with the copy of the contents of other.  
	 *        Copies the load factor, the predicate, and the hash function as well.
	 *        
	 * @param other - The set whose contents will be copied.
	 */
	AnySet(const AnySet& other):
		pair_type(other.as_pair()), table_(0, other.alloc_socca())
	{
		table_.resize(other.table_.size());
		std::transform(other.table_.begin(), other.table_.end(), table_.begin(), 
			[](const auto& list) { return list_type(list, list_type::make_copy); }
		);
		count_ = other.count_;
		max_load_factor_ = other.max_load_factor_;
		table_.back() = list_type::get_sentinal();
	}

	/**
	 * @brief Construct an AnySet with the contents of the initializer list ilist.
	 *        Same as AnySet(init.begin(), init.end()).  Sets max_load_factor() to 1.0.
	 *        If multiple elements in the list compare equivalent, only the first 
	 *        encountered is inserted.
	 * @param ilist        - Initializer list to initialize the elements of the set with.
	 * @param bucket_count - Minimum number of buckets to initialize the set with.
	 * @param hash         - Hash function to initialize the set with.
	 * @param equal        - Equality comparison function to initialize the set with
	 * @param alloc        - Allocator to initialize the set with.
	 */
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
		
	}

	/**
	 * @brief Construct an AnySet with the contents of the initializer list ilist.
	 *        Same as AnySet(init.begin(), init.end()).  Sets max_load_factor() to 1.0.
	 *        If multiple elements in the list compare equivalent, only the first 
	 *        encountered is inserted.
	 * @param ilist        - Initializer list to initialize the elements of the set with.
	 * @param bucket_count - Minimum number of buckets to initialize the set with.
	 * @param alloc        - Allocator to initialize the set with.
	 */
	template <class T>
	AnySet(
		std::initializer_list<T> ilist,
		size_type bucket_count,
		const Allocator& alloc
	):
		AnySet(ilist, bucket_count, Hash(), KeyEqual(), alloc)
	{
		
	}

	/**
	 * @brief Construct an AnySet with the contents of the initializer list ilist.
	 *        Same as AnySet(init.begin(), init.end()).  Sets max_load_factor() to 1.0.
	 *        If multiple elements in the list compare equivalent, only the first 
	 *        encountered is inserted.
	 * @param ilist        - Initializer list to initialize the elements of the set with.
	 * @param bucket_count - Minimum number of buckets to initialize the set with.
	 * @param hash         - Hash function to initialize the set with.
	 * @param alloc        - Allocator to initialize the set with.
	 */
	template <class T>
	AnySet(
		std::initializer_list<T> ilist,
		size_type bucket_count,
		const Hash& hash,
		const Allocator& alloc
	):
		AnySet(ilist, bucket_count, hash, KeyEqual(), alloc)
	{
		
	}

	/**
	 * @brief Construct an AnySet with the contents of the tuple tup. Sets max_load_factor()
	 *        to 1.0.  If multiple elements in the tuple have the same type and compare 
	 *        equivalent,only the first encountered is inserted.
	 * @param ilist        - Initializer list to initialize the elements of the set with.
	 * @param bucket_count - Minimum number of buckets to initialize the set with.
	 * @param hash         - Hash function to initialize the set with.
	 * @param equal        - Equality comparison function to initialize the set with
	 * @param alloc        - Allocator to initialize the set with.
	 */
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
		
	}

	/**
	 * @brief Construct an AnySet with the contents of the tuple tup. Sets max_load_factor()
	 *        to 1.0.  If multiple elements in the tuple have the same type and compare 
	 *        equivalent,only the first encountered is inserted.
	 * @param ilist        - Initializer list to initialize the elements of the set with.
	 * @param bucket_count - Minimum number of buckets to initialize the set with.
	 * @param alloc        - Allocator to initialize the set with.
	 */
	template <class ... T>
	AnySet(
		std::tuple<T ...>&& tup,
		size_type bucket_count,
		const Allocator& alloc
	):
		AnySet(std::move(tup), bucket_count, Hash(), KeyEqual(), alloc)
	{
		
	}

	/**
	 * @brief Construct an AnySet with the contents of the tuple tup. Sets max_load_factor()
	 *        to 1.0.  If multiple elements in the tuple have the same type and compare 
	 *        equivalent,only the first encountered is inserted.
	 * @param ilist        - Initializer list to initialize the elements of the set with.
	 * @param bucket_count - Minimum number of buckets to initialize the set with.
	 * @param hash         - Hash function to initialize the set with.
	 * @param alloc        - Allocator to initialize the set with.
	 */
	template <class ... T>
	AnySet(
		std::tuple<T ...>&& tup,
		size_type bucket_count,
		const Hash& hash,
		const Allocator& alloc
	):
		AnySet(std::move(tup), bucket_count, hash, KeyEqual(), alloc)
	{
		
	}

	/**
	 * @brief Copy assigns the contents of this AnySet instance from the contents of other.
	 *        Copies the load factor, the predicate, the hash function, and allocator as well.
	 *        
	 * @param other - The set whose contents will be copied.
	 *
	 * @return *this;
	 */
	AnySet& operator=(const AnySet& other)
	{ return *this = std::move(self_type(other)); }

	/**
	 * @brief Move assigns the contents of this AnySet instance from the contents of other.
	 *        Moves the load factor, the predicate, the hash function, and allocator as well.
	 *        
	 * @param other - The set whose contents will be moved.
	 *
	 * @return *this;
	 */
	AnySet& operator=(AnySet&& other) noexcept(std::is_nothrow_move_assignable_v<vector_type>)
	{
		table_ = std::move(other.table_);
		count_ = other.count_;
		max_load_factor_ = other.max_load_factor_;
		table_.back() = list_type::get_sentinal();
		as_pair() = std::move(other.as_pair());
		return *this;
	}

	/**
	 * @brief Assigns the contents of this AnySet instance with the contents of ilist.
	 *        
	 * @param ilist - The initializer list whose contents will be copied.
	 *
	 * @return *this;
	 */
	template <class T>
	AnySet& operator=(std::initializer_list<T> ilist)
	{ return *this = self_type(ilist); }

	/**
	 * @brief Get a const_iterator to the first element in the set.
	 *        
	 * @return const_iterator to the first element in the set.
	 */
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

	/**
	 * @brief Get a const_iterator to the first element in the set.
	 *        
	 * @return const_iterator to the first element in the set.
	 */
	const_iterator begin() const
	{ return cbegin(); }

	/**
	 * @brief Get a iterator to the first element in the set.
	 *        
	 * @return iterator to the first element in the set.
	 */
	iterator begin() 
	{ return to_non_const_iterator(cbegin()); }

	/**
	 * @brief Get a past-the-end const_iterator for this set.
	 * 
	 * @return const_iterator 1 past the last element in the set.
	 */
	const_iterator cend() const
	{
		auto pos = const_iterator(table_.cend() - 1, table_.back().cbegin());
		assert(pos.vector_pos_->is_sentinal());
		return pos;
	}

	/**
	 * @brief Get a past-the-end const_iterator for this set.
	 * 
	 * @return const_iterator 1 past the last element in the set.
	 */
	const_iterator end() const
	{ return cend(); }

	/**
	 * @brief Get a past-the-end iterator for this set.
	 * 
	 * @return iterator 1 past the last element in the set.
	 */
	iterator end()
	{ return to_non_const_iterator(cend()); }

	/**
	 * @brief Checks if the set has no elements, i.e. whether begin() == end().
	 * 
	 * @return true if the set is empty, false otherwise.
	 */
	[[nodiscard]]
	bool empty() const noexcept
	{ return not static_cast<bool>(size()); }

	/**
	 * @brief Gets the number of elements in the set, i.e. std::distance(begin(), end()).
	 * 
	 * @return The number of elements in the set.
	 */
	size_type size() const noexcept
	{ return count_; }

	/**
	 * @brief Get the maximum number of elements the container is able to hold due to system 
	 *        or library implementation limitations, i.e. std::distance(begin(), end()) for 
	 *        the largest container. 
	 * 
	 * @return Maximum number of elements.
	 */
	bool max_size() const noexcept
	{ return std::numeric_limits<size_type>::max(); }


	/**
	 * @brief Removes all elements from the container.  Invalidates any references, pointers, 
	 *        or iterators referring to contained elements. Does not invalidate past-the-end
	 *        iterators. 
	 */
	void clear() noexcept
	{
		for(size_type i = 0, len = table_size(); i < len; ++i)
			table_[i].clear();
		count_ = 0;
	}
	
	/**
	 * @brief Inserts a new element into the container constructed in-place with the given args 
	 *        if there is no element with the same type and value in the container.
	 * 
	 * @param args - Arguments to forward to the constructor of the element.
	 *
	 * @return Returns a pair consisting of an iterator to the inserted element, or the 
	 *         already-existing element if no insertion happened, and a bool denoting whether 
	 *         the insertion took place. true for insertion, false for no insertion.
	 * 
	 * @remark AnySet follows the convention of STL node-based containers where emplacement 
	 *         is implemented by first allocating a new node, constructing the element within
	 *         the allocated node, and *then* inserting.  This means that AnySet::emplace() allocates
	 *         a node even when insertion fails (i.e. the element already exists in the set).
	 *         
	 *         Emplacement is the only way of inserting objects of non-movable, non-copyable types into
	 *         an AnySet instance.
	 */
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
	
	/**
	 * @brief Inserts a new element into the container constructed in-place with the given args 
	 *        if there is no element with the same type and value in the container, using an iterator 
	 *        hint as a suggestion for where the new element should be placed.
	 * 
	 * @param args - Arguments to forward to the constructor of the element.
	 * @param hint - Iterator, used as a suggestion as to where to insert the new element.
	 *
	 * @return Returns a pair consisting of an iterator to the inserted element, or the 
	 *         already-existing element if no insertion happened, and a bool denoting whether 
	 *         the insertion took place. true for insertion, false for no insertion.
	 * 
	 * @remark This member function is provided for consistency.  The @p hint argument is ignored 
	 *         and instead this function simply calls AnySet::emplace<T>(forward<Args>(args)...).
	 *         
	 *         AnySet follows the convention of STL node-based containers where emplacement 
	 *         is implemented by first allocating a new node, constructing the element within
	 *         the allocated node, and *then* inserting.  This means that AnySet::emplace() allocates
	 *         a node even when insertion fails (i.e. the element already exists in the set).
	 *         
	 *         Emplacement is the only way of inserting objects of non-movable, non-copyable types into
	 *         an AnySet instance.
	 */
	template <class T, class ... Args>
	std::pair<iterator, bool> emplace_hint(const_iterator hint, Args&& ... args)
	{ return emplace<T>(std::forward<Args>(args)...); }

	/**
	 * @brief Inserts an element into the set, if the set doesn't already contain an element 
	 *        with an equivalent value and type.
	 * 
	 * @param value - element value to insert.
	 *
	 * @return Returns a pair consisting of an iterator to the inserted element, or the 
	 *         already-existing element if no insertion happened, and a bool denoting whether 
	 *         the insertion took place. true for insertion, false for no insertion.
	 */
	template <class T>
	std::pair<iterator, bool> insert(T&& value)
	{
		auto hash_v = get_hasher()(value);
		auto bucket_pos = table_.begin() + bucket_index(hash_v);
		return insert_impl<true>(std::forward<T>(value), hash_v, bucket_pos);
	}

	/**
	 * @brief Inserts an element into the set, if the set doesn't already contain an element 
	 *        with an equivalent value and type.
	 * 
	 * @param value - element value to insert.
	 *
	 * @return Returns a pair consisting of an iterator to the inserted element, or the 
	 *         already-existing element if no insertion happened, and a bool denoting whether 
	 *         the insertion took place. true for insertion, false for no insertion.
	 * 
	 * @remark This member function is provided for consistency.  The @p hint argument is ignored 
	 *         and instead this function simply calls AnySet::insert<T>(forward<T>(value)).
	 */
	template <class T>
	std::pair<iterator, bool> insert(const_iterator hint, T&& value)
	{ return insert(std::forward<T>(value)); }

	/**
	 * @brief Inserts elements from the range [first, last) that do not already exist in the set. 
	 *        If multiple elements in the range have values that compare equivalent, and there
	 *        is no such element already in the set, only the first encountered is inserted.
	 * 
	 * @param first - Iterator to first element in the range.
	 * @param last  - Iterator one position past the last element in the range.
	 * 
	 * @remark Range insertion assumes that all elements in the range [first, last) do not already
	 *         exist in the set and will preemptively reallocate the internal bucket array accordingly
	 *         to satisfy the current max_load_factor() if @p first and @p last are not input iterators.
	 * 
	 * @note Iterators are only invalidated by insertion if the value of AnySet::bucket_count() 
	 *       changes after calling AnySet::insert().
	 */
	template <
		class It, 
		class = std::enable_if_t<
			std::is_copy_constructible_v<typename std::iterator_traits<It>::value_type>
			or std::is_rvalue_reference_v<decltype(*std::declval<It>())>
		>
	>
	void insert(It first, It last)
	{
		using iter_cat = typename std::iterator_traits<It>::iterator_category;
		range_insert(first, last, iter_cat{});
	}

	/**
	 * @brief Inserts @p args if they do not already exist in the set.  If multiple values in @p args 
	 *        have the same type and have values that compare equivalent, and there is no such element 
	 *        already in the set, only the first encountered is inserted.
	 * 
	 * @param args - values to insert into the set.
	 * 
	 * @remark Variadic argument insertion assumes that all of the arguments do not already exist in the set 
	 *         and will preemptively reallocate the internal bucket array accordingly to satisfy the current
	 *         max_load_factor().
	 * 
	 * @note Iterators are only invalidated by insertion if the value of AnySet::bucket_count() 
	 *       changes after calling AnySet::insert().
	 */
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
	}
	
	/**
	 * @brief Inserts elements from the list @p ilist that do not already exist in the set. 
	 *        If multiple elements in the list have values that compare equivalent, and there
	 *        is no such element already in the set, only the first encountered is inserted.
	 * 
	 * @param ilist - Initializer list of elements to insert into the set.
	 * 
	 * @remark Initializer-list insertion assumes that all elements in the list do not already
	 *         exist in the set and will preemptively reallocate the internal bucket array
	 *         accordingly to satisfy the current max_load_factor().
	 * 
	 * @note Iterators are only invalidated by insertion if the value of AnySet::bucket_count() 
	 *       changes after calling AnySet::insert().
	 */
	template <class T, class = std::enable_if_t<std::is_copy_constructible_v<T>>>
	void insert(std::initializer_list<T> ilist)
	{
		insert(ilist.begin(), ilist.end());
	}

	/**
	 * @brief Remove the element at the position pointed to by @p pos from the set.
	 * 
	 * @param pos - Iterator to the element to erase.
	 * 
	 * @return Iterator to the element following the erased element.
	 * 
	 * @note Unlike std::unordered_set, erasure invalidates iterators to the
	 *       erased element *and* any iterators to the following element.
	 */
	iterator erase(const_iterator pos)
	{
		count_ -= pos.erase();
		return to_non_const_iterator(pos);
	}

	/**
	 * @brief Remove elements in the range [@p first, @p last).
	 * 
	 * @param first - Iterator to the first element in the range to erase.
	 * @param last  - Iterator one position past the last element to erase.
	 * 
	 * @return Iterator to the element following the last erased element.
	 * 
	 * @note Unlike std::unordered_set, erasure invalidates iterators to the
	 *       erased elements *and* any iterators to the element following the last 
	 *       erased element.
	 */
	iterator erase(const_iterator first, const_iterator last)
	{
		count_ -= to_non_const_iterator(first).erase_range(to_non_const_iterator(last));
		return to_non_const_iterator(first);
	}

	/**
	 * @brief Remove the element from the set whose type is the same type as @p value 
	 *        and whose value compares equal to @p value.
	 * 
	 * @param value - Value of the element to erase.
	 * 
	 * @return The number of elements erased (zero or one).
	 * 
	 * @note Unlike std::unordered_set, erasure invalidates iterators to the
	 *       erased element *and* any iterators to the following element.
	 */
	template <class T, class = std::enable_if_t<not (std::is_same_v<const_iterator, T> or std::is_same_v<iterator, T>)>>
	size_type erase(const T& value)
	{
		auto hash_v = get_hasher()(value);
		auto bucket_pos = table_.begin() + bucket_index(hash_v);
		auto [list_pos, found] = find_position(*bucket_pos, hash_v, value);
		assert(list_pos.is_end() ? not found : true);
		if(found)
			erase(const_iterator(bucket_pos, list_pos));
		return found;
	}

	/**
	 * @brief Exchanges the contents of the set with those of other. Does not 
	 *        invoke any move, copy, or swap operations on individual elements.
	 *        Additionally exchanges the hash functions, comparison functions, and 
	 *        max_load_factor()s of the sets.
	 * 
	 * @param other - The set to exchange contents with.
	 * 
	 * @note All iterators and references remain valid.
	 */
	void swap(AnySet& other) 
		noexcept(std::is_nothrow_swappable_v<vector_type> and std::is_nothrow_swappable_v<pair_type>)
	{
		using std::swap;
		swap(table_, other.table_);
		swap(as_pair(), other.as_pair());
		swap(count_, other.count_);
		swap(max_load_factor_, other.max_load_factor_);
	}

	/**
	 * @brief Returns the number of elements with a value that have the same type as, 
	 *        and compare equal to the @p value, which is either 1 or 0 since AnySet
	 *        does not allow duplicates.
	 * 
	 * @param value - The value to obtain the count of in the set.
	 * 
	 * @return The number of elements found.
	 *
	 * @note All iterators and references remain valid.
	 */
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

	/**
	 * @brief Obtain a const_iterator to the element that has the same type as, 
	 *        and compares equal to the @p value.
	 * 
	 * @param value - The value to obtain find.
	 * 
	 * @return const_iterator to the element found, or this->cend() if no such element exists.
	 */
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

	/**
	 * @brief Obtain an iterator to the element that has the same type as, 
	 *        and compares equal to the @p value.
	 * 
	 * @param value - The value to obtain find.
	 * 
	 * @return iterator to the element found, or this->end() if no such element exists.
	 */
	template <class T>
	iterator find(const T& value)
	{
		return to_non_const_iterator(
			static_cast<const self_type*>(this)->find(value)
		);
	}

	/**
	 * @brief Obtain a const_iterator range to the elements that have the same type as,
	 *        and compares equal to the @p value.
	 * 
	 * @param value - The value to obtain find.
	 * 
	 * @return std::pair containing a pair of const_iterators defining the requested range. If 
	 *         there are no such elements, past-the-end (see cend()) const_iterators are returned 
	 *         as both elements of the pair. 
	 */
	template <class T>
	std::pair<const_iterator, const_iterator> equal_range(const T& value) const
	{
		auto pos = find(value);
		if(pos != cend())
			return std::make_pair(pos, std::next(pos));
		else
			return std::make_pair(cend(), cend());
	}

	/**
	 * @brief Obtain an iterator range to the elements that have the same type as,
	 *        and compares equal to the @p value.
	 * 
	 * @param value - The value to obtain find.
	 * 
	 * @return std::pair containing a pair of iterators defining the requested range. If 
	 *         there are no such elements, past-the-end (see end()) iterators are returned 
	 *         as both elements of the pair. 
	 */
	template <class T>
	std::pair<iterator, iterator> equal_range(const T& value)
	{
		auto [first, last] = static_cast<const self_type*>(this)->equal_range(value);
		return std::make_pair(
			to_non_const_iterator(first),
			to_non_const_iterator(last)
		);
	}

	/**
	 * @brief Get a const_local_iterator to the first element in the bucket at index @p buck.
	 * 
	 * @param buck - Index of the bucket.
	 * 
	 * @return const_local_iterator to the first element in the bucket at index @p buck.
	 */
	const_local_iterator cbegin(size_type buck) const
	{
		assert(buck < bucket_count());
		return table_[buck].cbegin();
	}

	/**
	 * @brief Get a const_local_iterator to the first element in the bucket at index @p buck.
	 * 
	 * @param buck - Index of the bucket.
	 * 
	 * @return const_local_iterator to the first element in the bucket at index @p buck.
	 */
	const_local_iterator begin(size_type buck) const
	{ return cbegin(buck); }

	/**
	 * @brief Get a local_iterator to the first element in the bucket at index @p buck.
	 * 
	 * @param buck - Index of the bucket.
	 * 
	 * @return local_iterator to the first element in the bucket at index @p buck.
	 */
	local_iterator begin(size_type buck)
	{ return cbegin(buck).to_non_const(); }

	/**
	 * @brief Get a const_local_iterator to the element one past the least element in the
	 *        bucket at index @p buck.
	 * 
	 * @param buck - Index of the bucket.
	 * 
	 * @return const_local_iterator to the element one past the least element in the bucket 
	 *         at index @p buck.
	 */
	const_local_iterator cend(size_type buck) const
	{
		assert(buck < bucket_count());
		return table_[buck].cend();
	}

	/**
	 * @brief Get a const_local_iterator to the element one past the least element in the
	 *        bucket at index @p buck.
	 * 
	 * @param buck - Index of the bucket.
	 * 
	 * @return const_local_iterator to the element one past the least element in the bucket 
	 *         at index @p buck.
	 */
	const_local_iterator end(size_type buck) const
	{ return cend(buck); }

	/**
	 * @brief Get a local_iterator to the element one past the least element in the
	 *        bucket at index @p buck.
	 * 
	 * @param buck - Index of the bucket.
	 * 
	 * @return local_iterator to the element one past the least element in the bucket 
	 *         at index @p buck.
	 */
	local_iterator end(size_type buck)
	{ return cend(buck).to_non_const(); }

	/**
	 * @brief Get the number of buckets in the set.
	 * 
	 * @return The number of buckets in the set.
	 */
	size_type bucket_count() const noexcept
	{ return table_size(); }

	/**
	 * @brief Get the maximum possible number of buckets in the set.
	 * 
	 * @return The maximum possible number of buckets in the set.
	 */
	size_type max_bucket_count() const noexcept
	{ 
		size_type maxm = ~((~size_type(0)) >> 1);
		while(maxm > table_.max_size())
			maxm >>= 1;
		return maxm;
	}

	/**
	 * @brief Get the number of elements in the bucket at index @p buck.
	 * 
	 * @param buck - Index of the bucket.
	 * 
	 * @return The number of elements in the bucket.
	 */
	size_type bucket_size(size_type buck) const
	{
		assert(buck < bucket_count());
		return static_cast<size_type>(
			std::distance(cbegin(buck), cend(buck))
		);
	}

	/**
	 * @brief Get the bucket index of @p value.
	 * 
	 * @param value - Value whose bucket is to be returned.
	 * 
	 * @return Index of the bucket in which @p value belongs.
	 */
	template <class T>
	size_type bucket(const T& value) const
	{ return bucket_index(get_hasher()(value)); }

	/**
	 * @brief Set the maximum possible number of buckets in the set.
	 * 
	 * @param f - The new load factor.  Must be a positive number.
	 * 
	 * @return The maximum possible number of buckets in the set.
	 */
	void max_load_factor(float f)
	{
		assert(f > 0.0);
		max_load_factor_ = f;
	}
	
	/**
	 * @brief Get the maximum allowable load factor for the set.
	 * 
	 * @return The maximum allowable load factor for the set.
	 */
	float max_load_factor() const noexcept
	{ return max_load_factor_; }

	/**
	 * @brief Get the load factor for the set.
	 * 
	 * @return The load factor for the set.
	 */
	float load_factor() const noexcept
	{ return static_cast<double>(count_) / table_size(); }
	
	/**
	 * @brief Sets the number of buckets to the smallest possible value that is at least as 
	 *        large as @p nbuckets and rehashes the container, i.e. puts the elements into 
	 *        appropriate buckets considering that total number of buckets has changed. If 
	 *        the new number of buckets makes load factor more than maximum load factor, then
	 *        the new number of buckets is at least size() / max_load_factor(). 
	 * 
	 * @param nbuckets - The new number of buckets in the container after rehashing.
	 *
	 * @remark The current implementation uses only powers-of-two for the bucket count.
	 */
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
	}

	/**
	 * @brief Sets the number of buckets to the number needed to accomodate at least 
	 *        @p count elements without exceeding the maximum load factor and rehashes 
	 *        the container, i.e. puts the elements into appropriate buckets considering 
	 *        that total number of buckets has changed. Effectively calls 
	 *        @code rehash(std::ceil(count / max_load_factor())).
	 * 
	 * @param count - The number of elements to reserve space for.
	 *
	 * @remark The current implementation uses only powers-of-two for the bucket count.
	 */
	void reserve(size_type count)
	{
		rehash(static_cast<size_type>(std::ceil(count / max_load_factor())));
	}

	/**
	 * @brief Get a copy of the hash function.
	 * 
	 * @return A copy of hash function.
	 */
	hasher hash_function() const
	{ return get_hasher(); }

	/**
	 * @brief Get a copy of the equality comparison function.
	 * 
	 * @return A copy of the equality comparison function.
	 */
	key_equal key_eq() const
	{ return get_key_equal(); }

	/**
	 * @brief Get a copy of the allocator.
	 * 
	 * @return A copy of the allocator.
	 */
	allocator_type get_allocator() const
	{ return table_.get_allocator(); }

	/**
	 * @brief Calls left.swap(right).
	 * 
	 * @param left  - Set whose contents are to be swapped with @p right.
	 * @param right - Set whose contents are to be swapped with @p left.
	 */
	friend void swap(AnySet& left, AnySet& right)
	{ return left.swap(right); }

	/**
	 * @brief Compare the contents of two sets.
	 * 
	 * @param left  - Set whose contents are to be swapped with @p right.
	 * @param right - Set whose contents are to be swapped with @p left.
	 * 
	 * @return true if every element in @p left is contained in @p right
	 *         and vice-versa, using operator== to test for element equality,
	 *         otherwise false.
	 *
	 * @note If either set contains one or more objects of a type that does not
	 *       satisfy the EqualityComparable concept, this function returns false.
	 */
	friend bool operator==(const AnySet& left, const AnySet& right)
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
		return true;
	}

	/**
	 * @brief Compare the contents of two sets.
	 * 
	 * @param left  - Set whose contents are to be swapped with @p right.
	 * @param right - Set whose contents are to be swapped with @p left.
	 * 
	 * @return false if every element in @p left is contained in @p right
	 *         and vice-versa, using operator== to test for element equality, 
	 *         otherwise true.
	 *
	 * @note If either set contains one or more objects of a type that does not
	 *       satisfy the EqualityComparable concept, this function returns true.
	 */
	friend bool operator!=(const AnySet& left, const AnySet& right)
	{ return not (left == right); }


	/**
	 * @brief Add copies of elements from @p other.
	 * 
	 * @param other - Set whose contents are to be added to the given set.
	 * 
	 * @return *this.
	 *
	 * @note If @p other contains an element whose type does not satisfy
	 *       CopyConstructible, this function throws a te::CopyConstructionError.
	 * 
	 *       If an exception is thrown while this function is executing, @p this 
	 *       will be only partially updated.
	 * 
	 *       Iterators into @p this are only invalidated if @code{.cpp} 
	 *       this->bucket_count() @endcode changes.
	 */
	AnySet& update(const AnySet& other)
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
		return *this;
	}

	/**
	 * @brief Moves elements from @p other into the set.
	 * 
	 * @param other - Set whose contents are to be moved to the given set.
	 * 
	 * @return *this.
	 *
	 * @note No element copy or move constructors are invoked by this function.
	 *       Elements are moved by splicing internal node objects from @p other 
	 *       into @p this.
	 * 
	 *       If an exception is thrown while this function is executing, @p this 
	 *       will be only partially updated and @p other will be missing any elements
	 *       that added to @p this.
	 * 
	 *       Iterators into @p this are only invalidated if @code{.cpp} 
	 *       this->bucket_count() @endcode changes.
	 * 
	 *       Iterators to elements from @p other that were moved into @p this, as well 
	 *       as iterators to any elements following moved-from elements from @p other, 
	 *       are invalidated.
	 * 
	 * @remark This function effectively erases any elements contained in @p this from 
	 *         @p other.  
	 */
	AnySet& update(AnySet&& other)
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
		return *this;
	}

	/**
	 * @brief Remove and return the element at the position pointed to by @p pos 
	 *        from the set.
	 * 
	 * @param pos - Iterator to the element to pop.
	 * 
	 * @return A std::pair whose first member is a @code{.cpp} std::unique_ptr<value_type> 
	 *         @endcode that points to the removed element, and whose second member is an
	 *         iterator to the element following the popped element.
	 * 
	 * @note Invalidates iterators to the popped element *and* any iterators to the 
	 *       following element.
	 */
	std::pair<node_handle, iterator> pop(const_iterator pos)
	{
		auto any_v = list_type::pop(pos.list_pos_);
		pos.ensure_valid();
		--count_;
		return std::make_pair(std::move(any_v), to_non_const_iterator(pos)); 
	}

	/**
	 * @brief Copy and return the element at the position pointed to by @p pos.
	 * 
	 * @param pos - Iterator to the element to pop.
	 * 
	 * @return A @code{.cpp} std::unique_ptr<value_type> @endcode that points to 
	 *         the copied element.
	 * 
	 * @note If the value at @p pos is an instance of a type that does not satisfy
	 *       CopyConstructible, this function throws a te::CopyConstructionError.
	 */
	node_handle dup(const_iterator pos) const
	{ return pos->clone(); }

	/**
	 * @brief Insert the value pointed to by @p node to @p this.
	 * 
	 * @param node - @code{.cpp} std::unique_ptr<value_type> @endcode pointing to the
	 *               element to add.
	 *
	 * @return A std::pair whose first member is an iterator to the inserted node or the
	 *         the element that prevented the insertion, and a @code{.cpp} 
	 *         std::unique_ptr<value_type> @endcode that points to null if the insertion
	 *         was successful, or @p node if it was unsuccessful.  In otherwords, the caller
	 *         gets the node back only if it couldn't be inserted.
	 *
	 * @note Iterators into @p this are only invalidated if @code{.cpp} 
	 *       this->bucket_count() @endcode changes.
	 */
	std::pair<iterator, node_handle> push(node_handle node)
	{
		auto [pos, found] = find_insertion_position(*node);
		if(not found)
			return std::make_pair(to_non_const_iterator(pos), std::move(node));
		pos = safely_splice_at(pos, std::move(node), node->hash);
		return std::make_pair(to_non_const_iterator(pos), node_handle(nullptr));
	}

	
	/**
	 * @brief Moves the element at position @p pos from @p other into @p this.
	 * 
	 * @param other - The set to move the element from.
	 * @param pos - Iterator to the element to move.
	 *
	 * @return A std::tuple whose first member is an iterator to the position in @p this
	 *         where the element was inserted or the element that prevented the insertion,
	 *         whose second member is an iterator to the position in @p other of the element
	 *         after the element at position @p pos, and whose third member is a bool 
	 *         indicating whether the move occurred.
	 *
	 * @note Iterators into @p this are only invalidated if @code{.cpp} 
	 *       this->bucket_count() @endcode changes.
	 *       
	 *       If the element was successfully moved, invalidates iterators to the moved element 
	 *       at @p pos *and* any iterators to the following element.
	 *       
	 *       No element copy or move constructors are invoked by this function.
	 *       The element is moved by splicing internal node objects from @p other into @p this.
	 * 
	 *       If an exception is thrown due to a failure to allocate enough buckets for the 
	 *       new element, both @p this and @p other are unmodified. (conditional rollback semantics)
	 */
	std::tuple<iterator, iterator, bool> splice(AnySet& other, const_iterator pos)
	{ return splice_or_copy(std::move(other), pos); }

	/**
	 * @brief Moves the elements in the range [first, last) from @p other into @p this.
	 * 
	 * @param other - The set to move the element from.
	 * @param first - Iterator to the first element to move.
	 * @param last  - Iterator to the element after the last element to move.
	 *
	 * @return An iterator range to the elements in @p other, that were *not*
	 *         moved into @p this.
	 *
	 * @note Iterators into @p this are only invalidated if @code{.cpp} 
	 *       this->bucket_count() @endcode changes.
	 *       
	 *       No element copy or move constructors are invoked by this function.
	 *       Elements are moved by splicing internal node objects from @p other into @p this.
	 * 
	 *       Invalidates any iterators to the elements from @p other that were moved into 
	 *       @p this *and* any iterators to the elements following them.
	 */
	std::pair<iterator, iterator> splice(AnySet& other, const_iterator first, const_iterator last)
	{ return splice_or_copy(std::move(other), first, last); }

	/**
	 * @brief Copies or moves the elements in the range [first, last) from @p other into @p this.
	 * 
	 * @param other - The set to move the element from.
	 * @param first - Iterator to the first element to move.
	 * @param last  - Iterator to the element after the last element to move.
	 *
	 * @return An iterator range to the elements in @p other, that were *not*
	 *         moved into @p this.  If @p other is not an rvalue, no elements 
	 *         are moved and thus the returned range is simple [first, last).
	 *
	 * @note Iterators into @p this are only invalidated if @code{.cpp} 
	 *       this->bucket_count() @endcode changes.
	 *       
	 *       If @p other is an rvalue, invalidates any iterators to the elements from @p other that 
	 *       were moved into @p this *and* any iterators to the elements following them, otherwise, 
	 *       no iterators into @p other are invalidated.
	 * 
	 *       If @p other is const and any value in the range [@p first, @p last) is an instance of a 
	 *       type that does not satisfy CopyConstructible, this function throws a te::CopyConstructionError.  
	 *       If this occurs, only the elements preceding that value will have been added to @p this.
	 */
	template <class T, class = std::enable_if_t<std::is_same_v<std::decay_t<T>, self_type>>>
	std::pair<iterator, iterator> splice_or_copy(
		T&& other, const_iterator first, const_iterator last
	)
	{
		preemptive_reserve(std::distance(first, last));
		auto pos = first;
		while(pos != last)
			std::tie(std::ignore, pos, std::ignore) = splice_or_copy(std::forward<T>(other), pos);
		return std::make_pair(first, pos);
	}

	/**
	 * @brief Copies or moves the element at position @p pos from @p other into @p this.
	 * 
	 * @param other - The set to copy or move the element from.
	 * @param pos - Iterator to the element to move.
	 *
	 * @return A std::tuple whose first member is an iterator to the position in @p this
	 *         where the element was inserted or the element that prevented the insertion,
	 *         whose second member is an iterator (or const_iterator, if @p other is const) 
	 *         to the position in @p other of the element after the element at position @p pos, 
	 *         and whose third member is a bool indicating whether the copy/move occurred.
	 *
	 * @note Iterators into @p this are only invalidated if @code{.cpp} this->bucket_count() @endcode 
	 *       changes.
	 *       
	 *       If the element was successfully moved, invalidates iterators to the moved element 
	 *       at @p pos *and* any iterators to the following element.  If the element was only
	 *       copied (i.e. if @p other is const) no iterators into @p other are invalidated.
	 *       
	 *       No element copy or move constructors are invoked by this function if @p other is a 
	 *       non-const rvalue.  In that case, the element is moved by splicing an internal node 
	 *       object from @p other into @p this.
	 * 
	 *       If an exception is thrown due to a failure to allocate enough buckets for the 
	 *       new element, both @p this and @p other are unmodified. (conditional rollback semantics)
	 *
	 *       If @p other is const and the value at position @p pos is an instance of a type that does 
	 *       not satisfy CopyConstructible, then this function throws a te::CopyConstructionError.  
	 *       If this occurs, @p this and @p other are unmodified. (conditional rollback semantics)
	 */
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

	/**
	 * @brief Check if @p this contains the same value as another set.
	 * 
	 * @param any_v - AnyValue<Hash, KeyEqual> instance to search for.
	 * 
	 * @return true if this set contains an element whose type as is the same as 
	 *         @p any_v's value's type and whose value compares equal to the value 
	 *         of @p any_v using the KeyEqual comparator.
	 */
	bool contains_value(const value_type& any_v) const
	{
		return cend() != find_matching_value(
			any_v,
			[&](const value_type& left, const value_type& right) {
				return compare_values(left, right);
			}
		);
	}

	/**
	 * @brief Check if @p this contains the same value as another set.
	 * 
	 * @param any_v - AnyValue<Hash, KeyEqual> instance to search for.
	 * 
	 * @return true if this set contains an element whose type as is the same as 
	 *         @p any_v's value's type and whose value compares equal to the value 
	 *         of @p any_v using operator==().
	 */
	bool contains_value_eq(const value_type& any_v) const
	{ return find_matching_value(any_v, std::equal_to<>{}) != cend(); }

	/**
	 * @brief Check if @p this contains @p value.
	 * 
	 * @param value - Value to search for.
	 * 
	 * @return true if this set contains an element whose type as is the same as 
	 *         @p values's type and whose value compares equal to @p value using
	 *         the KeyEqual comparator.
	 */
	template <class T>
	bool contains(const T& value) const
	{
		static_assert(
			not std::is_same_v<value_type, T>, 
			"Use AnySet::contains_value() to check if an AnySet contains an instance of AnyValue."
		);
		return count(value);
	}
	
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
			return iterator(buck_pos, list_pos);
		}
		else 
		{
			auto list_pos = list_type::splice(pos.list_pos_, std::move(node));
			return to_non_const_iterator(const_iterator(pos.vector_pos_, list_pos));
		}
	}
	
	iterator unsafe_splice_at(const_iterator pos, node_handle node)
	{
		++count_;
		pos.list_pos_ = list_type::splice(pos.list_pos_, std::move(node));
		return to_non_const_iterator(pos);
	}

	local_iterator unsafe_splice_at(const_local_iterator pos, node_handle node)
	{
		++count_;
		pos = list_type::splice(pos, std::move(node));
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
	}

	template <class It>
	void range_insert(It first, It last, std::input_iterator_tag)
	{
		while(first != last)
			insert_impl<true>(*first++);
	}

	template <class It, class = std::enable_if_t<std::is_copy_constructible_v<typename std::iterator_traits<It>::value_type>>>
	void range_insert(It first, It last, std::forward_iterator_tag)
	{
		preemptive_reserve(std::distance(first, last));
		while(first != last)
			insert_impl<false>(*first++);
		assert(load_factor() <= max_load_factor());
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
		table_.resize(new_size + 1);
		table_.back() = list_type::get_sentinal();
	}

	void grow_table(size_type new_size)
	{
		assert(new_size > table_size());
		// new size must always be a power of two
		assert((new_size & (new_size - 1)) == 0u);
		auto old_size = table_size();
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

	// functions to access the hasher and comparator
	pair_type& as_pair() &
	{ return static_cast<pair_type&>(*this); }

	pair_type&& as_pair() &&
	{ return static_cast<pair_type&&>(*this); }

	const pair_type& as_pair() const &
	{ return static_cast<const pair_type&>(*this); }

	const pair_type&& as_pair() const &&
	{ return static_cast<const pair_type&&>(*this); }

	Hash& get_hasher() &
	{ return as_pair().first(); }

	Hash&& get_hasher() &&
	{ return std::move(std::move(as_pair()).first()); }

	const Hash& get_hasher() const &
	{ return as_pair().first(); }

	const Hash&& get_hasher() const &&
	{ return std::move(std::move(as_pair()).first()); }

	KeyEqual& get_key_equal() &
	{ return as_pair().second(); }

	KeyEqual&& get_key_equal() &&
	{ return std::move(std::move(as_pair()).second()); }

	const KeyEqual& get_key_equal() const &
	{ return as_pair().second(); }

	const KeyEqual&& get_key_equal() const &&
	{ return std::move(std::move(as_pair()).second()); }

public:
	vector_type table_{list_type::get_sentinal()};
	std::size_t count_ = 0;
	float max_load_factor_{1.0};
};




} /* namespace te */	

#endif /* ANY_SET_H */

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
#include <bitset>
#include "AnyHash.h"
#include "CompressedPair.h"

/**
 * @mainpage AnySet Docs
 * 
 * ## [Main Project Page](https://github.com/tvanslyke/AnySet)
 *
 * te::AnySet is a [std::unordered_set](http://en.cppreference.com/w/cpp/container/unordered_set)-like 
 * container that can hold instances of any constructible type.  te::AnySet uses type erasure to store 
 * values of arbitrary types uniformly.
 *
 * te::AnySet intentionally mimics the interface of [std::unordered_set](http://en.cppreference.com/w/cpp/container/unordered_set),
 * providing member functions with the same name and semantics of many of the member functions of
 * [std::unordered_set](http://en.cppreference.com/w/cpp/container/unordered_set).  The primary difference
 * is that te::AnySet stores its contained objects inside of a wrapper class te::AnyValue.
 *
 * ## Quick Reference
 * * te::AnySet - A type-erased hash set.
 *     * SetOperations.h - Free functions and operator overloads for common set operations on te::AnySet instances.
 * * te::AnyValue - Type of elements stored in AnySet instances.
 * * te::AnyHash - Generic hash function object.
 *     * te::Hash - Customization point for te::AnyHash.
 *     * hash_value() - Customization point for te::AnyHash (compatible with boost::container_hash).
 *     * extra-hash.h - Utilities for building hash functions, intended to be compatible with boost::container_hash.
 *                      Also includes specializations of te::Hash for standard types like std::pair and std::tuple.
 *
 * Example Usage:
 * @include anyset.example.cpp
 * 
 */


/// @brief Primary classes and utility functions for AnySet.

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

/**
 * @brief 
 * AnySet is an associative container that contains a set of unique objects of any constructible type.
 * Search, insertion, and removal have average constant-time complexity.  Internally, the elements are 
 * not sorted in any particular order, but organized into buckets. Which bucket an element is placed into 
 * depends entirely on the hash of its value. This allows fast access to individual elements, since once 
 * a hash is computed, it refers to the exact bucket the element is placed into.
 *
 * Container elements may not be modified (even by non const iterators) since modification could change an 
 * element's hash and corrupt the container.
 * 
 * AnySet meets the requirements of [Container](http://en.cppreference.com/w/cpp/concept/Container), 
 * [AllocatorAwareContainer](http://en.cppreference.com/w/cpp/concept/AllocatorAwareContainer), and
 * [UnorderedAssociativeContainer](http://en.cppreference.com/w/cpp/concept/UnorderedAssociativeContainer).
 * 
 * @remark In order to provide certain functionality functionality and mimic the behavior of similar standard
 *         containers (such as std::unordered_set<>), AnySet must sacrifice some static type safety in the name
 *         of usability.  For example, AnySet instances are always copy constructible.  However, if an attempt 
 *         is made to copy construct or assign (or similar) from an AnySet instance that contains instances of 
 *         non-copy-constructible types, an exception is thrown.  
 * 
 * @tparam HashFn    - The type of the function object to use when computing the hash codes of elements.
 * @tparam KeyEqual  - The type of the function object to use when comparing elements for equality.
 * @tparam Allocator - The type of the allocator to use when allocating the internal bucket table.
 * 
 * @remark The STL's allocator model is too rigid for AnySet to sanely support customized allocation of its internal
 *         nodes.  Thus its internal elements/nodes are simply allocated using std::make_unique() (effectively using
 *         the keyword 'new').  Allocator support is only provided for the internal bucket table.
 *
 * @see AnyValue - User-visible @p value_type for AnySet instances.
 * @see AnyHash  - The default value of HashFn.  It is intended to be sufficiently extensible such that users 
 *                 should not need to roll their own HashFn types.
 */
template <
	class HashFn = AnyHash,
	class KeyEqual = std::equal_to<>,
	class Allocator = std::allocator<AnyValue<HashFn, KeyEqual>>
>
struct AnySet:
	private CompressedPair<HashFn, KeyEqual>
{
private:
	using self_type = AnySet<HashFn, KeyEqual, Allocator>;
	using list_type = detail::AnyList<HashFn, KeyEqual>;
	using list_iterator = typename list_type::iterator;
	using const_list_iterator = typename list_type::const_iterator;

public:
	/**
	 * @brief Forward iterator type returned from non-const operations on AnySet instances.
	 * 
	 * The iterator essentially wraps an `AnyValue**` that points into a linked list.  The
	 * decision to use a pointer-to-pointer makes erasure and insertion simpler to implement
	 * but makes iterator invalidation a little less intuitive.  See specific AnySet member 
	 * functions for details regarding iterator invalidation.
	 * 
	 * @see AnySet
	 * @see const_iterator
	 */ 
	struct iterator:
		public list_iterator
	{
		using value_type        = typename AnySet::value_type;
		using reference         = typename AnySet::reference;
		using pointer           = typename AnySet::pointer;
		using difference_type   = typename AnySet::difference_type;
		using iterator_category = std::forward_iterator_tag;

		using list_iterator::list_iterator;
		using list_iterator::operator=;

		iterator(const list_iterator& it):
			list_iterator(it)
		{
			
		}
	private:
		
		iterator to_non_const() const
		{ 
			return iterator(
				static_cast<const list_iterator&>(*this).to_non_const()
			); 
		}
		friend struct AnySet;
	};

	/**
	 * @brief Forward iterator type returned from const operations on AnySet instances.
	 * 
	 * The iterator essentially wraps an `AnyValue* const*` that points into a linked list.  The
	 * decision to use a pointer-to-pointer makes erasure and insertion simpler to implement
	 * but makes iterator invalidation a little less intuitive.  See specific AnySet member 
	 * functions for details regarding iterator invalidation.
	 * 
	 * @see AnySet
	 * @see iterator 
	 */ 
	struct const_iterator:
		public const_list_iterator
	{
		using value_type        = typename AnySet::value_type;
		using reference         = typename AnySet::const_reference;
		using pointer           = typename AnySet::const_pointer;
		using difference_type   = typename AnySet::difference_type;
		using iterator_category = std::forward_iterator_tag;

		const_iterator(const const_list_iterator& it):
			const_list_iterator(it)
		{
			
		}

		const_iterator(const list_iterator& it):
			const_list_iterator(it)
		{
			
		}

		const_iterator& operator=(const iterator& other)
		{
			return *this = const_iterator(other);
		}

		using const_list_iterator::const_list_iterator;
		using const_list_iterator::operator=;
	private:

		iterator to_non_const() const
		{ 
			return iterator(
				static_cast<const const_list_iterator&>(*this).to_non_const()
			); 
		}
		friend struct AnySet;
	};

private:
	using table_allocator = typename std::allocator_traits<Allocator>::template rebind_alloc<iterator>;
	using vector_type = std::vector<iterator, table_allocator>;
	using pair_type = CompressedPair<HashFn, KeyEqual>;
	using vector_iterator = typename vector_type::iterator;
	using const_vector_iterator = typename vector_type::const_iterator;

	template <class T>
	struct KeyInfo {
		const T& value;
		const std::size_t hash;
		std::size_t bucket;
	};

public:
	/// AnyValue.
	using value_type = AnyValue<HashFn, KeyEqual>;
	/// Size type.
	using size_type = typename vector_type::size_type;
	/// Difference type.
	using difference_type = typename vector_type::difference_type;
	/// Key equality comparator type.
	using key_equal = KeyEqual;
	/// %Hash function type.
	using hasher = HashFn;
	/// Allocator type.
	using allocator_type = Allocator;
	/// AnyValue.  Here for consistency with std::unordered_set.
	using key_type = value_type;
	/// Reference to AnyValue.
	using reference = value_type&;
	/// Reference to const AnyValue.
	using const_reference = const value_type&;
	/// Pointer to AnyValue.
	using pointer = value_type*;
	/// Pointer to const AnyValue.
	using const_pointer = const value_type*;
	/// Node type to allow elements to persist after being removed, and for 
	/// splicing between AnySet instances.
	using node_handle = std::unique_ptr<value_type>;

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

	template <bool IsConst>	
	struct BucketIterator:
		private std::conditional_t<IsConst, const_iterator, iterator>
	{
	private:
		using self_type = BucketIterator<IsConst>;
		using base_type = std::conditional_t<IsConst, const_iterator, iterator>;
	public:
		using value_type        = typename base_type::value_type;
		using reference         = typename base_type::reference;
		using pointer           = typename base_type::pointer;
		using difference_type   = typename base_type::difference_type;
		using iterator_category = typename base_type::iterator_category;

		BucketIterator() = default;

		friend bool operator==(const BucketIterator<IsConst>& left, const BucketIterator<IsConst>& right)
		{
			if((left.bucket_ != right.bucket_) or (left.mask_ != right.mask_))
			{
				// Different buckets or different masks.
				return false;
			}
			if(
				(left.get_pos().is_null() or left.is_past_the_end()) 
				and (right.get_pos().is_null() or right.is_past_the_end())
			)
			{
				return true;
			}
			return left.get_pos() == right.get_pos();
		}

		friend bool operator!=(const BucketIterator<IsConst>& left, const BucketIterator<IsConst>& right)
		{ return not (left == right); }

		self_type& operator++()
		{
			++get_pos();
			return *this;
		}

		self_type operator++() const
		{
			auto cpy = *this;
			++*this;
			return cpy;
		}

		using base_type::operator->;
		using base_type::operator*;

	private:
		BucketIterator(base_type pos, std::size_t bucket, std::size_t table_size):
			base_type(pos), bucket_(bucket), mask_(table_size - 1u)
		{
			
		}

		bool is_past_the_end() const
		{
			assert(not get_pos().is_null());
			// Returns true if 'pos_' is past-the-end for the whole list or the element pointed to by
			// 'pos_' has a hash in a different bucket (past-the-end for this bucket).
			return get_pos().is_end() or ((get_pos()->hash & mask_) != bucket_);
		}

		BucketIterator<false> to_non_const() const
		{
			return BucketIterator<false>(
				get_pos().to_non_const(), bucket_, mask_ + 1
			);
		}

		const base_type& get_pos() const
		{ return static_cast<const base_type&>(*this); }

		base_type& get_pos()
		{ return static_cast<base_type&>(*this); }

		std::size_t bucket_{0};
		std::size_t mask_{0};
		template <class, class, class>
		friend struct AnySet;
	};
	
public:
	/// Iterator type suitable for traversal through an individual bucket.
	using local_iterator = BucketIterator<false>;
	/// Const iterator type suitable for traversal through an individual bucket.
	using const_local_iterator = BucketIterator<true>;

	void _assert_invariants(bool check_load_factor = false) const
	{
		list_._assert_invariants();
		assert(table_size() > 0u);
		assert(((table_size() & (table_size() - 1)) == 0u) and "Table size not a power of two.");
		assert(max_load_factor_ > 0.0);
		auto iter_dist = std::distance(cbegin(), cend());
		assert(iter_dist >= 0);
		assert(static_cast<size_type>(iter_dist) == size());
		assert(std::all_of(table_.begin(), table_.end(), [](const auto& v){ return v.is_null() or not v.is_end();}));
		for(size_type i = 0; i < bucket_count(); ++i)
		{
			// each bucket is sorted WRT hash values of the nodes in the bucket
			assert(std::is_sorted(
				this->begin(i),
				this->end(i),
				[](const auto& l, const auto& r){ return l.hash < r.hash; }
			));
			assert(std::all_of(begin(i), end(i), [&](const auto& v){ return bucket_index(v.hash) == i; }));
		}
		// the load factor is allowed to be not satisfied if the user changed the max_load_factor().
		// we only do this assertion when asked to
		if(check_load_factor)
			assert(load_factor_satisfied());
	}

	~AnySet() = default;

	/// @name Constructors
	/// @{

	/**
	 * @brief Constructs an empty set with 1 bucket.  Sets max_load_factor() to 1.0.
	 */
	AnySet(): AnySet(size_type(0)) { }

	/**
	 * @brief Construct an empty AnySet instance.  Sets max_load_factor() to 1.0.
	 * @param bucket_count - Minimum number of buckets to initialize the set with.
	 * @param hash         - %Hash function to initialize the set with.
	 * @param equal        - Equality comparison function to initialize the set with.
	 * @param alloc        - Allocator to initialize the set with.
	 */
	explicit AnySet(
		size_type bucket_count,
		const HashFn& hash = HashFn(),
		const KeyEqual& equal = KeyEqual(),
		const Allocator& alloc = Allocator()
	):
		pair_type(hash, equal),
		table_(next_highest_pow2(bucket_count), iterator(), allocator_type(alloc))
	{
		
	}

	/**
	 * @brief Construct an empty AnySet instance.  Sets max_load_factor() to 1.0.
	 * @param bucket_count - Minimum number of buckets to initialize the set with.
	 * @param alloc        - Allocator to initialize the set with.
	 */
	AnySet(size_type bucket_count, const Allocator& alloc):
		AnySet(bucket_count, HashFn(), KeyEqual(), alloc) 
	{
		
	}
	
	/**
	 * @brief Construct an empty AnySet instance.  Sets max_load_factor() to 1.0.
	 * @param bucket_count - Minimum number of buckets to initialize the set with.
	 * @param hash         - %Hash function to initialize the set with.
	 * @param alloc        - Allocator to initialize the set with.
	 */
	AnySet(size_type bucket_count, const HashFn& hash, const Allocator& alloc):
		AnySet(bucket_count, hash, KeyEqual(), alloc)
	{
		
	}

	/**
	 * @brief Construct an AnySet instance from the range [first, last). 
	 *        Sets max_load_factor() to 1.0.  If multiple elements in the
	 *        range compare equivalent, only the first encountered is inserted.
	 * 
	 * @tparam InputIt - Input iterator type.
	 * 
	 * @param first        - Iterator to the first element in the range.
	 * @param last         - Iterator one position past the last element in the range.
	 * @param bucket_count - Minimum number of buckets to initialize the set with.
	 * @param hash         - %Hash function to initialize the set with.
	 * @param equal        - Equality comparison function to initialize the set with
	 * @param alloc        - Allocator to initialize the set with.
	 */
	template <class InputIt>
	AnySet(
		InputIt first, 
		InputIt last,
		size_type bucket_count = 0,
		const HashFn& hash = HashFn(),
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
	 * 
	 * @tparam InputIt - Input iterator type.
	 * 
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
		AnySet(first, last, bucket_count, HashFn(), KeyEqual(), alloc)
	{
		
	}

	/**
	 * @brief Construct an AnySet instance from the range [first, last). 
	 *        Sets max_load_factor() to 1.0.  If multiple elements in the
	 *        range compare equivalent, only the first encountered is inserted.
	 * 
	 * @tparam InputIt - Input iterator type.
	 * 
	 * @param first        - Iterator to the first element in the range.
	 * @param last         - Iterator one position past the last element in the range.
	 * @param bucket_count - Minimum number of buckets to initialize the set with.
	 * @param hash         - %Hash function to initialize the set with.
	 * @param alloc        - Allocator to initialize the set with.
	 */
	template <class InputIt>
	AnySet(
		InputIt first, 
		InputIt last,
		size_type bucket_count,
		const HashFn& hash,
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
		pair_type(other.as_pair()),
		list_(other.list_),
		table_(0, alloc),
		max_load_factor_(other.max_load_factor_)
	{
		table_.assign(other.table_.size(), iterator());
		list_type tmp(std::move(list_));
		assert(size() == 0u);
		node_handle node{nullptr};
		while(not tmp.empty())
		{
			node = std::move(tmp.pop(tmp.begin()).first);
			node = std::move(push(std::move(node)));
			assert(not static_cast<bool>(node));
		}
		assert(tmp.empty());
	}

	/**
	 * @brief Copy constructs an AnySet instance from other.
	 *        Constructs the set with the copy of the contents of other.  
	 *        Copies the load factor, the predicate, and the hash function as well.
	 *        
	 * @param other - The set whose contents will be copied.
	 */
	AnySet(const AnySet& other):
		pair_type(other.as_pair()),
		list_(other.list_, list_type::make_copy),
		table_(0, other.alloc_socca()),
		max_load_factor_(other.max_load_factor_)
	{
		table_.resize(other.table_.size());
		list_type tmp(std::move(list_));
		assert(size() == 0u);
		node_handle node{nullptr};
		while(not tmp.empty())
		{
			node = std::move(tmp.pop(tmp.begin()).first);
			node = std::move(push(std::move(node)).second);
			assert(not static_cast<bool>(node));
		}
		assert(tmp.empty());
	}

	/**
	 * @brief Move constructs an AnySet instance from other.
	 *        Constructs the set with the copy of the contents of other.  
	 *        Copies the load factor, the predicate, and the hash function as well.
	 * @param other - The set whose contents will be copied.
	 * @param alloc - Allocator to initialize the set with.
	 */
	AnySet(AnySet&& other, const Allocator& alloc):
		pair_type(std::move(other.as_pair())),
		list_(std::move(other.list_)),
		table_(std::move(other.table_), alloc),
		max_load_factor_(other.max_load_factor_)
	{
		fix_table_after_move();
		assert(other.empty());
		if(other.table_.size() == 0u)
			other.table_.assign(1u, iterator());
	}

	/**
	 * @brief Copy constructs an AnySet instance from other.
	 *        Constructs the set with the copy of the contents of other.  
	 *        Copies the load factor, the predicate, and the hash function as well.
	 *        
	 * @param other - The set whose contents will be copied.
	 */
	AnySet(AnySet&& other):
		pair_type(std::move(other.as_pair())),
		list_(std::move(other.list_)),
		table_(std::move(other.table_)),
		max_load_factor_(other.max_load_factor_)
	{
		fix_table_after_move();
		assert(other.empty());
		if(other.table_.size() == 0u)
			other.table_.assign(1u, iterator());
	}

	/**
	 * @brief Construct an AnySet with the contents of the initializer list ilist.
	 *        Same as AnySet(init.begin(), init.end()).  Sets max_load_factor() to 1.0.
	 *        If multiple elements in the list compare equivalent, only the first 
	 *        encountered is inserted.
	 * 
	 * @tparam T - Type of the elements in the initializer list which will be added to the set.
	 * 
	 * @param ilist        - Initializer list to initialize the elements of the set with.
	 * @param bucket_count - Minimum number of buckets to initialize the set with.
	 * @param hash         - %Hash function to initialize the set with.
	 * @param equal        - Equality comparison function to initialize the set with
	 * @param alloc        - Allocator to initialize the set with.
	 */
	template <class T>
	AnySet(
		std::initializer_list<T> ilist,
		size_type bucket_count = 0,
		const HashFn& hash = HashFn(),
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
	 * 
	 * @tparam T - Type of the elements in the initializer list which will be added to the set.
	 * 
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
		AnySet(ilist, bucket_count, HashFn(), KeyEqual(), alloc)
	{
		
	}

	/**
	 * @brief Construct an AnySet with the contents of the initializer list ilist.
	 *        Same as AnySet(init.begin(), init.end()).  Sets max_load_factor() to 1.0.
	 *        If multiple elements in the list compare equivalent, only the first 
	 *        encountered is inserted.
	 * 
	 * @tparam T - Type of the elements in the initializer list which will be added to the set.
	 * 
	 * @param ilist        - Initializer list to initialize the elements of the set with.
	 * @param bucket_count - Minimum number of buckets to initialize the set with.
	 * @param hash         - %Hash function to initialize the set with.
	 * @param alloc        - Allocator to initialize the set with.
	 */
	template <class T>
	AnySet(
		std::initializer_list<T> ilist,
		size_type bucket_count,
		const HashFn& hash,
		const Allocator& alloc
	):
		AnySet(ilist, bucket_count, hash, KeyEqual(), alloc)
	{
		
	}

	/**
	 * @brief Construct an AnySet with the contents of the tuple tup. Sets max_load_factor()
	 *        to 1.0.  If multiple elements in the tuple have the same type and compare 
	 *        equivalent,only the first encountered is inserted.
	 * 
	 * @tparam T - Parameter pack types of the tuple elements that will be used to initialize the set.
	 * 
	 * @param ilist        - Initializer list to initialize the elements of the set with.
	 * @param bucket_count - Minimum number of buckets to initialize the set with.
	 * @param hash         - %Hash function to initialize the set with.
	 * @param equal        - Equality comparison function to initialize the set with
	 * @param alloc        - Allocator to initialize the set with.
	 */
	template <class ... T>
	AnySet(
		std::tuple<T ...>&& tup,
		size_type bucket_count = 2 * (sizeof...(T)),
		const HashFn& hash = HashFn(),
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
	 * 
	 * @tparam T - Parameter pack types of the tuple elements that will be used to initialize the set.
	 * 
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
		AnySet(std::move(tup), bucket_count, HashFn(), KeyEqual(), alloc)
	{
		
	}

	/**
	 * @brief Construct an AnySet with the contents of the tuple tup. Sets max_load_factor()
	 *        to 1.0.  If multiple elements in the tuple have the same type and compare 
	 *        equivalent,only the first encountered is inserted.
	 * 
	 * @tparam T - Parameter pack types of the tuple elements that will be used to initialize the set.
	 * 
	 * @param ilist        - Initializer list to initialize the elements of the set with.
	 * @param bucket_count - Minimum number of buckets to initialize the set with.
	 * @param hash         - %Hash function to initialize the set with.
	 * @param alloc        - Allocator to initialize the set with.
	 */
	template <class ... T>
	AnySet(
		std::tuple<T ...>&& tup,
		size_type bucket_count,
		const HashFn& hash,
		const Allocator& alloc
	):
		AnySet(std::move(tup), bucket_count, hash, KeyEqual(), alloc)
	{
		
	}

	/// @} Constructors

	/// @name Assignment
	/// @{

	/**
	 * @brief Copy assigns the contents of this AnySet instance from the contents of other.
	 *        Copies the load factor, the predicate, the hash function, and allocator as well.
	 *        
	 * @param other - The set whose contents will be copied.
	 *
	 * @throws te::NoCopyConstructorError if @p other contains an element of non-copy-constructible
	 *         type.
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
		as_pair() = std::move(other.as_pair());
		list_ = std::move(other.list_);
		table_ = std::move(other.table_);
		max_load_factor_ = std::move(other.max_load_factor_);
		fix_table_after_move();
		assert(other.empty());
		if(other.table_.size() == 0u)
			other.table_.assign(1u, iterator());
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

	/// @} Assignment

	/// @name Iterators
	/// @{

	/**
	 * @brief Get a const_iterator to the first element in the set.
	 *        
	 * @return const_iterator to the first element in the set.
	 */
	const_iterator cbegin() const
	{ return list_.cbegin(); }

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
	{ return list_.begin(); }

	/**
	 * @brief Get a past-the-end const_iterator for this set.
	 * 
	 * @return const_iterator 1 past the last element in the set.
	 */
	const_iterator cend() const
	{ return list_.cend(); }

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
	{ return list_.end(); }

	/// @} defgroup AnySetIterators

	/// @name Capacity
	/// @{

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
	{ return list_.size(); }

	/**
	 * @brief Get the maximum number of elements the container is able to hold due to system 
	 *        or library implementation limitations, i.e. std::distance(begin(), end()) for 
	 *        the largest container. 
	 * 
	 * @return Maximum number of elements.
	 */
	bool max_size() const noexcept
	{ return std::numeric_limits<typename list_type::size_type>::max(); }

	/// @}

	/// @name Modifiers
	/// @{

	/**
	 * @brief Removes all elements from the container.  Invalidates any references, pointers, 
	 *        or iterators referring to contained elements.
	 */
	void clear() noexcept
	{
		list_.clear();
		std::fill(table_.begin(), table_.end(), iterator());
	}
	
	/**
	 * @brief Inserts a new element into the container constructed in-place with the given args 
	 *        if there is no element with the same type and value in the container.
	 * 
	 * @param args - Arguments to forward to the constructor of the element.
	 * 
	 * @tparam T - Type of the element to emplace.  Must be a constructible non-reference type.
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
	 * 
	 * @note References, pointers, and iterators remain valid after emplacement, however the values 
	 *       pointed to by iterators may change.
	 */
	template <class T, class ... Args>
	std::pair<iterator, bool> emplace(Args&& ... args)
	{
		static_assert(
			std::is_same_v<T, std::decay_t<T>>,
			"Cannot emplace references, arrays, or functions into an AnySet."
		);
		auto node(make_any_value<T, HashFn, KeyEqual>(get_hasher(), std::forward<Args>(args)...));
		auto hash_v = node->hash;
		KeyInfo<T> ki{unsafe_cast<const T&>(*node), hash_v, bucket_index(hash_v)};
		return insert_impl<true>(ki.value, ki, std::move(node));
	}
	
	/**
	 * @brief Inserts a new element into the container constructed in-place with the given args 
	 *        if there is no element with the same type and value in the container, using an iterator 
	 *        hint as a suggestion for where the new element should be placed.
	 * 
	 * @tparam T - Type of the element to emplace.  Must be a constructible non-reference type.
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
	 * 
	 * @note References, pointers, and iterators remain valid after emplacement, however the values 
	 *       pointed to by iterators may change.
	 */
	template <class T, class ... Args>
	std::pair<iterator, bool> emplace_hint([[maybe_unused]] const_iterator hint, Args&& ... args)
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
	 * 
	 * @note References, pointers, and iterators remain valid after insertion, however the values 
	 *       pointed to by iterators may change.
	 */
	template <class T>
	std::pair<iterator, bool> insert(T&& value)
	{
		return insert_impl<true>(std::forward<T>(value), make_key_info(value));
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
	 * 
	 * @note References, pointers, and iterators remain valid after insertion, however the values 
	 *       pointed to by iterators may change.
	 */
	template <class T>
	std::pair<iterator, bool> insert([[maybe_unused]] const_iterator hint, T&& value)
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
	 * @note References, pointers, and iterators remain valid after insertion, however the values 
	 *       pointed to by iterators may change.
	 */
	template <
		class It, 
		class = std::enable_if_t<
			std::is_copy_constructible_v<typename std::iterator_traits<It>::value_type>
			or std::is_rvalue_reference_v<decltype(*std::declval<It>())>
		>
	>
	size_type insert(It first, It last)
	{
		using iter_cat = typename std::iterator_traits<It>::iterator_category;
		return range_insert(first, last, iter_cat{});
	}

	/**
	 * @brief Inserts @p args if they do not already exist in the set.  If multiple values in @p args 
	 *        have the same type and have values that compare equivalent, and there is no such element 
	 *        already in the set, only the first encountered is inserted.
	 * 
	 * @param first  - first value to insert into the set.
	 * @param second - second value to insert into the set.
	 * @param args   - subsequent values to insert into the set.
	 * 
	 * @remark Variadic argument insertion assumes that all of the arguments do not already exist in the set 
	 *         and will preemptively reallocate the internal bucket array accordingly to satisfy the current
	 *         max_load_factor().
	 * 
	 * @return A bitset where the ith bit is set to `true` iff the ith argument was successfully inserted.
	 * 
	 * @note References, pointers, and iterators remain valid after insertion, however the values 
	 *       pointed to by iterators may change.
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
	std::bitset<2ull + sizeof...(V)> insert(T&& first, U&& second, V&& ... args)
	{
		return arg_insert(
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
	 * @return The number of values successfully inserted.
	 * 
	 * @note References, pointers, and iterators remain valid after insertion, however the values 
	 *       pointed to by iterators may change.
	 */
	template <class T, class = std::enable_if_t<std::is_copy_constructible_v<T>>>
	size_type insert(std::initializer_list<T> ilist)
	{
		return insert(ilist.begin(), ilist.end());
	}

	/**
	 * @brief Remove the element at the position pointed to by @p pos from the set.
	 * 
	 * @param pos - Iterator to the element to erase.
	 * 
	 * @return Iterator to the element following the erased element.
	 * 
	 * @note Unlike std::unordered_set, erasure invalidates only iterators to the
	 *       element following the erased element.  The iterator returned by erase()
	 *       is a suitable replacement for the invalidated iterator.
	 */
	iterator erase(const_iterator pos)
	{
		return pop(pos).second;
	}

	/**
	 * @brief Remove elements in the range [@p first, @p last).
	 * 
	 * @param first - Iterator to the first element in the range to erase.
	 * @param last  - Iterator one position past the last element to erase.
	 * 
	 * @return Iterator to the element following the last erased element.
	 * 
	 * @note Unlike std::unordered_set, erasure invalidates only iterators to the
	 *       element following the erased element.  The iterator returned by erase()
	 *       is a suitable replacement for the invalidated iterator.  
	 *
	 * @note For range-erasure, this specifically means that the returned iterator
	 *       is a suitable replacement for iterators equivalent to the @p last parameter.
	 */
	iterator erase(const_iterator first, const_iterator last)
	{
		iterator pos = first.to_non_const();
		if(first == last)
			return pos;
		bool done = false;
		do {
			// hop over 'pos' and check if we've reached the end
			auto nxt = std::next(first);
			done = (nxt == last);
			pos = erase(pos);
		} while(not done);
		return pos.to_non_const();
	}

	/**
	 * @brief Remove the element from the set whose type is the same type as @p value 
	 *        and whose value compares equal to @p value.
	 * 
	 * @param value - Value of the element to erase.
	 * 
	 * @return The number of elements erased (zero or one).
	 * 
	 * @note Unlike std::unordered_set, erasure invalidates only iterators to the
	 *       element following the erased element.  The iterator returned by erase()
	 *       is a suitable replacement for the invalidated iterator.
	 */
	template <class T, class = std::enable_if_t<not (std::is_same_v<const_iterator, T> or std::is_same_v<iterator, T>)>>
	size_type erase(const T& value)
	{
		auto [pos, found] = find_position(make_key_info(value));
		if(found)
			erase(pos);
		return static_cast<size_type>(found);
	}

	/**
	 * @brief Exchanges the contents of the set with those of other. Does not 
	 *        invoke any move, copy, or swap operations on individual elements.
	 *        Additionally exchanges the hash functions, comparison functions, and 
	 *        max_load_factor()s of the sets.
	 * 
	 * @param other - The set to exchange contents with.
	 * 
	 * @note All iterators and references remain valid.  'begin()' iterators do not
	 *       change their allegiance.
	 */
	void swap(AnySet& other) 
		noexcept(std::is_nothrow_swappable_v<vector_type> and std::is_nothrow_swappable_v<pair_type>)
	{
		using std::swap;
		swap(as_pair(), other.as_pair());
		swap(list_, other.list_);
		swap(table_, other.table_);
		swap(max_load_factor_, other.max_load_factor_);
		this->fix_table_after_move();
		other.fix_table_after_move();
	}

	/**
	 * @brief Add copies of elements from @p other.
	 * 
	 * @param other - Set whose contents are to be added to the given set.
	 * 
	 * @return *this.
	 *
	 * @note If @p other contains an element whose type does not satisfy
	 *       CopyConstructible, this function throws a te::NoCopyConstructorError.
	 * 
	 * @note If an exception is thrown while this function is executing, @p this 
	 *       will be only partially updated.
	 */
	AnySet& update(const AnySet& other)
	{
		preemptive_reserve(other.size());
		for(const auto& any_v: other)
		{
			auto [pos, found] = find_position(make_key_info(any_v));
			if(found)
			{
				continue;
			}
			else
			{
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
	 * @note If an exception is thrown while this function is executing, @p this 
	 *       will be only partially updated and @p other will be missing any elements
	 *       that added to @p this.
	 * 
	 * @note No iterators are invalidated by this function, but iterators pointing 
	 *       into @p other may point to into @p this after the call to this function.
	 *       Whether or not a particular iterator is past-the-end, and in which set
	 *       it is past-the-end, may be changed.
	 *
	 * @remark This function effectively erases any elements contained in @p this from 
	 *         @p other.  
	 */
	AnySet& update(AnySet&& other)
	{
		preemptive_reserve(other.size());
		for(auto pos = other.begin(); pos != other.end(); )
		{
			auto [ins_pos, found] = find_position(make_key_info(*pos));
			if(found)
			{
				++pos;
			}
			else
			{
				node_handle node;
				std::tie(node, pos) = other.pop(pos);
				unsafe_splice_at(ins_pos, std::move(node));
			}
		}
		return *this;
	}

	/// @} Modifiers

	/// @name Lookup
	/// @{

	/**
	 * @brief Returns the number of elements with a value that have the same type as, 
	 *        and compare equal to the @p value, which is either 1 or 0 since AnySet
	 *        does not allow duplicates.
	 * 
	 * @param value - The value to obtain the count of in the set.
	 * 
	 * @return The number of elements found.
	 */
	template <class T>
	size_type count(const T& value) const
	{
		return static_cast<size_type>(
			find_position(make_key_info(value)).second
		);
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
		auto [pos, found] = find_position(make_key_info(value));
		if(found)
			return pos;
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
		return static_cast<const self_type*>(this)->find(value).to_non_const();
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
			first.to_non_const(),
			last.to_non_const()
		);
	}

	/**
	 * @brief Check if @p this contains the same value as another set.
	 * 
	 * @param any_v - AnyValue<%Hash, KeyEqual> instance to search for.
	 * 
	 * @return true if this set contains an element whose type as is the same as 
	 *         @p any_v's value's type and whose value compares equal to the value 
	 *         of @p any_v using the KeyEqual comparator.
	 */
	bool contains_value(const value_type& any_v) const
	{
		return contains(any_v);
	}

	/**
	 * @brief Check if @p this contains the same value as another set.
	 * 
	 * @param any_v - AnyValue<%Hash, KeyEqual> instance to search for.
	 * 
	 * @return true if this set contains an element whose type as is the same as 
	 *         @p any_v's value's type and whose value compares equal to the value 
	 *         of @p any_v using operator==().
	 */
	bool contains_value_eq(const value_type& any_v) const
	{ return find_matching_value(any_v) != cend(); }

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
		return count(value);
	}
	
	/**
	 * @brief Check if @p this contains @p value.
	 * 
	 * @param value - Value to search for.
	 * 
	 * @return true if this set contains an element whose type as is the same as 
	 *         @p value's type and whose value compares equal to @p value using operator==().
	 */
	template <class T>
	bool contains_eq(const T& value) const
	{ return find_matching_value(value) != cend(); }
	
	/// @} Lookup

	/// @name Bucket Interface
	/// @{

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
		return const_local_iterator(table_[buck], buck, table_size());
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
		return const_local_iterator(const_iterator(), buck, table_size());
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

	/// @} Bucket Interface

	/// @name Hash Policy
	/// @{

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
	{ return static_cast<double>(size()) / table_size(); }
	
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
			if(nbuckets == 0u)
				nbuckets = 1u;
			while(nbuckets < bcount and load_factor_good())
				bcount /= 2;
			if((bcount == bucket_count()) and load_factor_good())
				return;
			
		}
		else if(nbuckets > bcount)
		{
			assert(max_bucket_count() >= nbuckets);
			while(nbuckets > bcount)
			{
				assert(bcount > 0u);
				bcount *= 2;
			}
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
	 *        rehash(std::ceil(count / max_load_factor())).
	 * 
	 * @param count - The number of elements to reserve space for.
	 *
	 * @remark The current implementation uses only powers-of-two for the bucket count.
	 */
	void reserve(size_type count)
	{
		rehash(static_cast<size_type>(std::ceil(count / max_load_factor())));
	}

	/// @} Hash Policy

	/// @name Observers
	/// @{

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

	/// @} Observers 

	/// @name Non-Member functions
	/// @{

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
		// iterate over the set with the smaller table.
		const auto& iter_set = (left.table_size() > right.table_size()) ? right : left;
		// search through the set with the larger table
		const auto& search_set = (&left == &iter_set) ? right : left;
		for(const auto& item: iter_set)
		{
			auto pos = search_set.find_matching_value(item);
			if(pos.is_end())
			{
				return false;
			}
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

	/// @}  Non-Member functions

	/// @name Node Interface
	/// @{

	/**
	 * @brief Remove and return the element at the position pointed to by @p pos 
	 *        from the set.
	 * 
	 * @param pos - Iterator to the element to pop.
	 * 
	 * @return A std::pair whose first member is a node_handle 
	 *         that points to the removed element, and whose second member is an iterator to the 
	 *         element following the popped element.
	 * 
	 * @note Invalidates any iterators to the element after the popped element.  Use the
	 *       iterator returned by this function in place of those.  Iterators to the popped element
	 *       will point to the element after the popped element and are not invalidated.  If the 
	 *       returned node_handle is subsequently added to a set by calling 'push()' any invalidated
	 *       iterators are subsequently "re-validated", but will point to a (possibly past-the-end)
	 *       element in the set in which the node_handle was pushed.
	 */
	std::pair<node_handle, iterator> pop(const_iterator pos_)
	{
		assert(not pos_.is_end());
		assert(not pos_.is_null());

		iterator pos = pos_.to_non_const();
		size_type buck_idx = iter_bucket_index(pos);
		iterator bucket_head = table_[buck_idx];

		assert(not bucket_head.is_null());
		assert(not bucket_head.is_end());

		if(bucket_head == pos)
		{
			// 'pos' is the first item in its bucket.
			auto [node, next] = std::move(list_.pop(pos));
			if(next.is_end())
			{
				// 'pos' was the last item in 'list_'.  The bucket is now empty.
				table_[buck_idx] = iterator();
				return std::make_pair(std::move(node), end());
			}

			if(size_type next_idx = iter_bucket_index(next); next_idx != buck_idx)
			{
				// The const_iterator after 'bucket_head' is in a different bucket.
				// 'pos' was the last item in its bucket and the bucket is now empty.
				// Since we just invalidated all iterators to the element following
				// 'pos', we need to fix iterator in the bucket whose first element
				// was pointed to by next(pos).
				table_[buck_idx] = iterator();
				// Fix the iterator we invalidated.
				table_[next_idx] = next;
			}
			else
			{
				// 'pos' was *not* the last element in its bucket.  The iterator
				// stored in the bucket should automagically point to the next
				// item.  Nothing to fix.
				assert(table_[buck_idx] == next);
			}
			return std::make_pair(std::move(node), next); 
		}
		else
		{
			// 'pos' is *not* the first item in its bucket.
			auto [node, next] = std::move(list_.pop(pos));
			if(next.is_end())
			{
				// 'pos' was the last item in the whole list.  Nothing to fix.
				return std::make_pair(std::move(node), end());
			}

			if(size_type next_idx = iter_bucket_index(next); next_idx != buck_idx)
			{
				// 'pos' was the last item in its bucket.  
				// Since we just invalidated all iterators to the element following
				// 'pos', we need to fix iterator in the bucket whose first element
				// was pointed to by next(pos).
				table_[next_idx] = next;
			}
			else
			{
				// 'pos' was *not* the last element in its bucket.  The iterator
				// stored in the bucket should automagically point to the next
				// item.  Nothing to fix.
				assert(table_[buck_idx] == next);
			}
			return std::make_pair(std::move(node), next); 
		}
	}

	/**
	 * @brief Copy and return the element at the position pointed to by @p pos.
	 * 
	 * @param pos - Iterator to the element to pop.
	 * 
	 * @return A node_handle that points to 
	 *         the copied element.
	 * 
	 * @note If the value at @p pos is an instance of a type that does not satisfy
	 *       CopyConstructible, this function throws a te::NoCopyConstructorError.
	 */
	node_handle dup(const_iterator pos) const
	{ return pos->clone(); }

	/**
	 * @brief Insert the value pointed to by @p node to @p this.
	 * 
	 * @param node - node_handle pointing to the element to add.
	 *
	 * @return A std::pair whose first member is an iterator to the inserted node or the
	 *         the element that prevented the insertion, and a node_handle that points 
	 *         to null if the insertion was successful, or @p node if it was unsuccessful.  
	 *         In otherwords, the caller gets the node back only if it couldn't be inserted.
	 *
	 * @note No iterators are invalidated.
	 */
	std::pair<iterator, node_handle> push(node_handle&& node)
	{
		auto [pos, found] = find_position(make_key_info(*node));
		if(found)
			return std::make_pair(pos.to_non_const(), std::move(node));
		iterator ins_pos = safely_splice_at(pos, std::move(node));
		return std::make_pair(ins_pos, node_handle(nullptr));
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
	 * @note No iterators are invalidated by this function.
	 *       
	 * @note No element copy or move constructors are invoked by this function.
	 *       The element is moved by splicing internal node objects from @p other into @p this.
	 * 
	 * @note If an exception is thrown due to a failure to allocate enough buckets for the 
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
	 * @note No iterators are invalidated by this function.
	 *       
	 * @note No element copy or move constructors are invoked by this function.
	 *       The element is moved by splicing internal node objects from @p other into @p this.
	 * 
	 * @note If an exception is thrown due to a failure to allocate enough buckets for the 
	 *       new element, both @p this and @p other are unmodified. (conditional rollback semantics)
	 */
	std::pair<iterator, iterator> splice(AnySet& other, const_iterator first, const_iterator last)
	{ return splice_or_copy(std::move(other), first, last); }

	/**
	 * @brief Copies or moves the elements in the range [first, last) from @p other into @p this.
	 * 
	 * Elements are only moved from @p other if @p other is an rvalue reference to non-const.  
	 * If @p other is a reference to const AnySet, then elements are copied.
	 * 
	 * @param other - The set to move the element from.
	 * @param first - Iterator to the first element to move.
	 * @param last  - Iterator to the element after the last element to move.
	 *
	 * @return An iterator range to the elements in @p other, that were *not*
	 *         moved into @p this.  If @p other is not an rvalue, no elements 
	 *         are moved and thus the returned range is simple [first, last).
	 *
	 * @note No iterators are invalidated by this function.
	 *       
	 * @note No element copy or move constructors are invoked by this function.
	 *       The element is moved by splicing internal node objects from @p other into @p this.
	 * 
	 * @note If an exception is thrown due to a failure to allocate enough buckets for the 
	 *       new element, both @p this and @p other are unmodified. (conditional rollback semantics)
	 * 
	 * @note If @p other is const and any value in the range [@p first, @p last) is an instance of a 
	 *       type that does not satisfy CopyConstructible, this function throws a te::NoCopyConstructorError.  
	 *       If this occurs, only the elements preceding that value will have been added to @p this.
	 */
	template <class T, class = std::enable_if_t<std::is_same_v<std::decay_t<T>, self_type>>>
	auto splice_or_copy(T&& other, const_iterator first, const_iterator last)
		-> std::pair<decltype(other.begin()), decltype(other.begin())> 
	{
		if(first == last)
			return std::make_pair(first.to_non_const(), last.to_non_const());
		preemptive_reserve(std::distance(first, last));
		auto pos = first;
		bool done = false;
		do {
			done = (std::next(pos) == last);
			std::tie(std::ignore, pos, std::ignore) = splice_or_copy(std::forward<T>(other), pos);
		} while(not done);
		return std::make_pair(first.to_non_const(), pos.to_non_const());
	}

	/**
	 * @brief Copies or moves the element at position @p pos from @p other into @p this.
	 * 
	 * Elements are only moved from @p other if @p other is an rvalue reference to non-const.
	 * If @p other is a reference to const AnySet, then elements are copied.
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
	 * @note No iterators are invalidated by this function.
	 *       
	 * @note No element copy or move constructors are invoked by this function if @p other is a 
	 *       non-const rvalue.  In that case, the element is moved by splicing an internal node 
	 *       object from @p other into @p this.
	 * 
	 * @note If an exception is thrown due to a failure to allocate enough buckets for the 
	 *       new element, both @p this and @p other are unmodified. (conditional rollback semantics)
	 *
	 * @note If @p other is const and the value at position @p pos is an instance of a type that does 
	 *       not satisfy CopyConstructible, then this function throws a te::NoCopyConstructorError.  
	 *       If this occurs, @p this and @p other are unmodified. (conditional rollback semantics)
	 */
	template <class T, class = std::enable_if_t<std::is_same_v<std::decay_t<T>, self_type>>>
	auto splice_or_copy(T&& other, const_iterator pos)
		-> std::tuple<iterator, decltype(other.begin()), bool>
	{
		using tuple_t = std::tuple<iterator, decltype(other.begin()), bool>;
		assert(not pos.is_end());
		auto ki = make_key_info(*pos);
		auto [ins_pos_, found] = find_position(ki);
		auto ins_pos = ins_pos_.to_non_const();
		if(found)
			return tuple_t(ins_pos, std::next(pos).to_non_const(), false);
		if constexpr(
			std::is_rvalue_reference_v<decltype(other)>
			and not std::is_const_v<std::remove_reference_t<decltype(other)>>
		)
		{
			auto [node, next] = other.pop(pos);
			try
			{
				assert(load_factor_satisfied());
				return tuple_t(
					safely_splice_at(ins_pos, ki, std::move(node)), next.to_non_const(), true
				);
			}
			catch(...)
			{
				// put back the node if didn't make it in
				if(node)
					other.unsafe_splice_at(pos, std::move(node));
				throw;
			}
		}
		else
		{
			assert(load_factor_satisfied());
			return tuple_t(
				safely_splice_at(ins_pos, ki, other.dup(pos)), 
				std::next(pos).to_non_const(), 
				true
			);
		}
	}

	/// @}  Node Interface

private:

	void fix_table_after_move()
	{
		if(size() > 0u)
			table_[iter_bucket_index(begin())] = begin();
	}

	float load_factor(size_type extra) const noexcept
	{ return static_cast<double>(size() + extra) / table_size(); }
	
	bool load_factor_satisfied(size_type extra = 0) const
	{ return load_factor(extra) <= max_load_factor(); }

	iterator safely_splice_at(const_iterator pos, node_handle&& node)
	{
		return safely_splice_at(pos, make_key_info(*node), std::move(node));
	}

	template <class Value>
	iterator safely_splice_at(const_iterator pos, const KeyInfo<Value>& ki, node_handle&& node)
	{
		// Insert the node, *then* check if a rehash is needed.  This optimizes for the case
		// where we don't rehash (which is almost certainly the common case) and doing it this 
		// way results in simpler code.
		iterator ins_pos = unsafe_splice_at(pos, ki, std::move(node));

		if(not load_factor_satisfied())	
		{
			// However, we want rollback semantics if resizing the bucket table throws an exception.
			// Use the scope guard pattern instead of a try-catch-rethrow.  This lets the compiler possibly
			// optimize the guard out entirely if callers don't have an exception handler (many 
			// implementations only propagate exceptions when there's a handler beneath the throw).
			struct RollbackScopeGuard {
				self_type& self; // *this
				const_iterator pos; // iterator to erase 
				bool good = false; // whether we
				
				~RollbackScopeGuard()
				{
					// If growing failed, erase the value we just inserted.
					// This gives us rollback/commit semantics and only pessimizes
					// the uncommon case.
					if(not good) 
						// Control flow only reach here if an exception is thrown by 'grow_table'.
						self.erase(pos);
				}
			} guard{*this, pos, false};

			// Before we grow the table, save the address of the inserted node, we'll need it
			// to find the node again after rehashing.
			const value_type* addr_save = std::addressof(*ins_pos);

			grow_table(2 * table_size());

			// No exceptions thrown.
			guard.good = true;

			// the key landed somewhere else now, go find it to give the caller their iterator.
			auto buck_idx = bucket_index(ki.hash);
			auto buck_pos = table_[buck_idx];
			assert(not buck_pos.is_null());
			assert(not buck_pos.is_end());
			while(std::addressof(*buck_pos) != addr_save)
			{
				++buck_pos;
				assert(iter_bucket_index(buck_pos) == buck_idx);
			}
			return buck_pos.to_non_const();
		}
		else
		{
			// No rehash, just return ins_pos.
			return ins_pos;
		}
	}

	template <class Value>
	iterator initialize_bucket(const KeyInfo<Value>& ki, node_handle&& node)
	{
		assert(table_[ki.bucket].is_null());
		return table_[ki.bucket] = list_.push_back(std::move(node));
	}

	iterator unsafe_splice_at(const_iterator pos, node_handle&& node)
	{
		return unsafe_splice_at(pos, make_key_info(*node), std::move(node));
	}

	template <class Value>
	iterator unsafe_splice_at(const_iterator pos, const KeyInfo<Value>& ki, node_handle&& node)
	{
		if(pos.is_null())
			return initialize_bucket(ki, std::move(node));
		else if(pos.is_end())
			return list_.splice(pos, std::move(node));

		size_type buck_idx = iter_bucket_index(pos);
		iterator ins_pos = list_.splice(pos, std::move(node));
		if(buck_idx != ki.bucket)
		{
			// The iterator that we just inserted changed the value of an iterator 
			// in another bucket.  The iterator whose value changed should now point 
			// to the 'next' of the node we just inserted.  Make it so.
			auto next_pos = std::next(ins_pos);
			assert(iter_bucket_index(next_pos) == buck_idx);
			table_[buck_idx] = next_pos;
		}
		return ins_pos;
	}

	template <class Value>
	bool insertion_compare(const Value& value, const value_type& any_v) const
	{ return compare(value, any_v, get_key_equal()); }

	template <class Value>
	bool insertion_compare(const value_type& any_v, const Value& value) const
	{ return compare(any_v, value, get_key_equal()); }

	template <class Value>
	std::size_t get_hash_value(const Value& value) const
	{
		if constexpr(std::is_same_v<Value, value_type>)
			return value.hash;
		else
			return get_hasher()(value);
	}
	
	size_type iter_bucket_index(const_iterator pos) const
	{
 		assert(not pos.is_end());
		return bucket_index(pos->hash);
	}

	template <class Value>
	KeyInfo<Value> make_key_info(const Value& val) const
	{
		return make_key_info(val, get_hash_value(val));
	}

	template <class Value>
	KeyInfo<Value> make_key_info(const Value& val, std::size_t hash_v) const
	{
		return KeyInfo<Value>{val, hash_v, bucket_index(hash_v)};
	}

	template <class Value>
	const_iterator find_matching_value(const Value& value) const
	{
		auto ki = make_key_info(value);
		auto [pos, last] = get_bucket_start(ki);
		if(pos.is_null() or (not last.is_null()))
			return cend();

		while((not pos.is_end()) and (pos->hash == ki.hash))
		{
			if(*pos == value)
				return const_iterator(pos);
			++pos;
		}
		return cend();
	}

	template <class ... T>
	std::bitset<sizeof...(T)> arg_insert(T&& ... args)
	{
		std::bitset<sizeof...(T)> bs;
		std::size_t pos = 0;
		preemptive_reserve(sizeof...(args));

		// set the ith bit if the ith arg was inserted
		(bs.set(pos++, insert_impl<false>(std::forward<T>(args)).second) , ...);

		assert(load_factor() <= max_load_factor());
		return bs;
	}

	template <class It>
	size_type range_insert(It first, It last, std::input_iterator_tag)
	{
		size_type count = 0;
		while(first != last)
			count += static_cast<size_type>(insert_impl<true>(*first++).second);
		return count;
	}

	template <class It, class = std::enable_if_t<std::is_copy_constructible_v<typename std::iterator_traits<It>::value_type>>>
	size_type range_insert(It first, It last, std::forward_iterator_tag)
	{
		size_type count = 0;
		preemptive_reserve(std::distance(first, last));
		while(first != last)
			count += static_cast<size_type>(insert_impl<false>(*first++).second);
		assert(load_factor() <= max_load_factor());
		return count;
	}

	void preemptive_reserve(std::size_t ins_count)
	{
		auto new_count = size() + ins_count;
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
		return insert_impl<CheckLoadFactor>(
			std::forward<T>(value), make_key_info(value), nullptr
		);
	}
	
	template <bool CheckLoadFactor, class T>
	std::pair<iterator, bool> insert_impl(
		T&& value,
		const KeyInfo<std::decay_t<T>>& ki,
		node_handle existing = nullptr
	)
	{
		using ValueType = std::decay_t<T>;
		iterator ins_pos;
		auto [pos, found] = find_position(ki);
		
		if(not found)
		{
			// if constexpr(std::is_copy_constructible_v<ValueType>)
			if constexpr(std::is_constructible_v<ValueType, decltype(value)>)
			{
				if(not existing)
				{
					existing = make_any_value<std::decay_t<T>, HashFn, KeyEqual>(
						ki.hash, std::forward<T>(value)
					);
				}
			}
			else
			{
				assert(existing);
			}

			if constexpr(CheckLoadFactor)
				ins_pos = safely_splice_at(pos, ki, std::move(existing));
			else
				ins_pos = unsafe_splice_at(pos, ki, std::move(existing));
			assert(not ins_pos.is_null());
			assert(not ins_pos.is_end());
		}
		else
		{
			ins_pos = pos.to_non_const();
		}
		return std::make_pair(ins_pos, not found);
	}

	void shrink_table(size_type new_size) noexcept
	{
		assert(new_size < table_size());
		// new size must always be a power of two
		assert((new_size & (new_size - 1)) == 0u);

		table_.assign(new_size, iterator());

		list_type tmp = std::move(list_);
		assert(size() == 0u);
		node_handle node{nullptr};
		while(not tmp.empty())
		{
			node = std::move(tmp.pop(tmp.begin()).first);
			node = std::move(push(std::move(node)).second);
			assert(not static_cast<bool>(node));
		}
		assert(tmp.empty());
	}

	void grow_table(size_type new_size)
	{
		assert(new_size > table_size());
		// new size must always be a power of two
		assert((new_size & (new_size - 1)) == 0u);
		table_.assign(new_size, iterator());
		list_type tmp = std::move(list_);
		assert(size() == 0u);
		node_handle node{nullptr};
		while(not tmp.empty())
		{
			node = std::move(tmp.pop(tmp.begin()).first);
			node = std::move(push(std::move(node)).second);
			assert(not static_cast<bool>(node));
		}
		assert(tmp.empty());
	}

	/**
	 * @brief Get a const_iterator to the element that matches @p ki, or 
	 *        a const_iterator to the position that @p ki could be inserted.
	 * @param ki - KeyInfo for the element to find.
	 * @return If a matching element is found, returns an iterator to that element
	 *         and the value 'true'.  Otherwise returns an iterator to the position
	 *         that a value matching @p ki could be inserted at and the value 'false'.
	 */
	template <class Value>
	std::pair<const_iterator, bool> find_position(const KeyInfo<Value>& ki) const
	{ return find_position(ki, get_key_equal()); }

	/**
	 * @brief Get a const_iterator to the element that matches @p ki, or 
	 *        a const_iterator to the position that @p ki could be inserted.
	 * @param ki   - KeyInfo for the element to find.
	 * @param comp - The comparator to use for equality testing.
	 * @return If a matching element is found, returns an iterator to that element
	 *         and the value 'true'.  Otherwise returns an iterator to the position
	 *         that a value matching @p ki could be inserted at and the value 'false'.
	 *         If the bucket that @p ki belongs in is empty, returns a 'null' iterator.
	 */
	template <class Value, class Comp>
	std::pair<const_iterator, bool> find_position(const KeyInfo<Value>& ki, Comp comp) const
	{
		if(auto [pos, last] = get_bucket_start(ki); pos.is_null() and last.is_null())
		{
			// Bucket is empty, return null iterator.
			return std::make_pair(const_iterator(), false);
		}
		else if(last.is_null())
		{
			assert(pos->hash >= ki.hash);
			for(; (not pos.is_end()) and pos->hash == ki.hash; ++pos)
			{
				if(compare(ki.value, *pos, comp))
					// found a match
					return std::make_pair(pos, true);
			}
			// No match, return const_iterator to first element whose hash is
			// greater than ki's hash, or the end of the list, which ever comes
			// first.
			return std::make_pair(pos, false);
		}
		else
		{
			assert(not pos.is_null());
			assert(last == cend() or iter_bucket_index(last) != ki.bucket);
			// No match, return end of the bucket.
			return std::make_pair(last, false);
		}
	}

	/**
	 * @brief Search the bucket that @p ki belongs in for the iterator to the first element 
	 *        whose hash is greater or equal to @p ki's hash.
	 * @param ki - KeyInfo for the element to search for.
	 * @return If the bucket is empty, returns two const_iterator instances for which '.is_null()'
	 *         evaluates to 'true'.  Otherwise if the bucket is nonempty but all elements have hashes 
	 *         less than @p ki's hash, returns a const_iterator to the first element in the bucket, 
	 *         and a const_iterator to the last element in the bucket (it follows that the second iterator 
	 *         is a suitable position to insert the element at).  Otherwise returns a const_iterator to the 
	 *         first element whose hash is greater than or equal to @p ki's hash and a const_iterator for 
	 *         which '.is_null()' evaluates to 'true'.
	 */ 
	template <class Value>
	std::pair<const_iterator, const_iterator> get_bucket_start(const KeyInfo<Value>& ki) const
	{
		auto first = table_[ki.bucket];
		if(first.is_null())
			// two 'null' const_iterators
			return std::make_pair(const_iterator(), const_iterator()); 
		assert(not first.is_end());
		auto pos = first;
		std::size_t pos_hash = pos->hash;
		do {
			if(pos_hash >= ki.hash)
				return std::make_pair(pos, const_iterator());
			++pos;
			if(pos.is_end())
				break;
			pos_hash = pos->hash;
		} while(bucket_index(pos_hash) == ki.bucket);
		return std::make_pair(first, pos);
	}

	size_type table_size() const
	{ return table_.size(); }

	size_type get_mask() const
	{ return table_size() - 1; }

	size_type bucket_index(std::size_t hash) const
	{ return hash & get_mask(); }

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

	HashFn& get_hasher() &
	{ return as_pair().first(); }

	HashFn&& get_hasher() &&
	{ return std::move(std::move(as_pair()).first()); }

	const HashFn& get_hasher() const &
	{ return as_pair().first(); }

	const HashFn&& get_hasher() const &&
	{ return std::move(std::move(as_pair()).first()); }

	KeyEqual& get_key_equal() &
	{ return as_pair().second(); }

	KeyEqual&& get_key_equal() &&
	{ return std::move(std::move(as_pair()).second()); }

	const KeyEqual& get_key_equal() const &
	{ return as_pair().second(); }

	const KeyEqual&& get_key_equal() const &&
	{ return std::move(std::move(as_pair()).second()); }

	list_type list_;
	vector_type table_;
	float max_load_factor_{1.0};
};

/**
 * @brief Helper function for creating an AnySet instance from a heterogeneous 
 *        sequence of objects.
 *
 * @param elements - Parameter pack of values to initialize the set's contents with.
 * @param hash_fn  - The hash function for the set.
 * @param key_eq   - The equality comparison function for the set.
 * @param alloc    - The allocator for the set.
 *
 * @return A newly-constructed AnySet instance that contains the provided elements.
 *
 * @relates AnySet
 */
template <
	class HashFn = te::AnyHash,
	class KeyEqual = std::equal_to<>,
	class Allocator = std::allocator<te::AnyValue<HashFn, KeyEqual>>,
	class ... Elements
>
te::AnySet<HashFn, KeyEqual, Allocator> make_anyset(
	Elements&& ... elements,
	const HashFn& hash_fn = HashFn(),
	const KeyEqual& key_eq = KeyEqual(),
	const Allocator& alloc = HashFn()
)
{
	return te::AnySet<HashFn, KeyEqual, Allocator>(
		std::forward_as_tuple(std::forward<Elements>(elements)...),
		hash_fn, key_eq, alloc
	);
}


} /* namespace te */	

#endif /* ANY_SET_H */

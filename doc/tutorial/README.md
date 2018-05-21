# AnySet Tutorials

## [AnySet Documentation]()

## Contents
1. [Constructing an AnySet object](#constructing-an-anyset-object)
    * [Initializing with Homogeneous Elements](#initializing-with-homogeneous-elements)
    * [Initializing with Heterogeneous Elements](#initializing-with-heterogeneous-elements)
2. [Adding and Removing Elements](#adding-and-removing-elements)
    * [Adding Non-Copy-Constructible Values](#adding-non-copy-constructible-values)
    * [Adding Non-Copy-Constructible Values](#adding-non-copy-constructible-values)
    * [Adding Non-Movable Values](#adding-non-movable-values)
    * [Adding Several Values at Once](#adding-several-values-at-once)
    * [Removing Elements](#removing-elements)
    * [Interset Operations](#interset-operations)
3. [Element Lookup and Similar Operations](#element-lookup-and-similar-operations)
    * [Find an Element](#find-an-element)
    * [Check if an Object Exists](#check-if-an-object-exists)
4. [Core Set-Theoretic Operations](#core-set-theoretic-operations)
5. [AnyValue (AnySet's `value_type`)](#anyvalue-anysets-value_type)
    * [Accessing the Contained Value Statically](#accessing-the-contained-value-statically)
    * [Accessing the Contained Value Dynamically](#accessing-the-contained-value-dynamically)
    * [Higher-Level Accessors](#higher-level-accessors)
6. [Customizing AnyHash](Customizing AnyHash)


# Constructing an AnySet Object

## Initializing with Homogeneous Elements
Construct from an initializer-list:
```c++
te::AnySet<> set({1, 2, 3, 4, 5});
// set == {1, 2, 3, 4, 5}

// assign a new list of values
set = {"One"s, "Two"s, "Three"s, "Four"s, "Five"s};
// set == {"One"s, "Two"s, "Three"s, "Four"s, "Five"s}
```

Construct from an iterator range.
```c++
using std::literals;
std::vector<std::string> names {
	"Albert"s, "Becky"s, "Thomas"s, "Noel"s, "Alexis"s
};
te::AnySet<> set(names.begin(), names.end());
// set = {"Albert"s, "Becky"s, "Thomas"s, "Noel"s, "Alexis"s}
```

## Initializing with Heterogeneous Elements
Construct from a `std::tuple`.
```c++
using std::literals;
te::AnySet<> set(
	std::forward_as_tuple("Some String"s, 3.141592, 100, std::bitset<3>(0b010u))
);
// set = {"Some String"s, 3.141592, 100, std::bitset<3>(0b010u)}
```

Constructing from a parameter pack:
```c++
te::AnySet<> set(te::make_anyset(std::forward<Args>(args)...));
// set = {args...};
```

# Adding and Removing Elements

## Adding Non-Copy-Constructible Values
Use `AnySet::emplace<T>()` to add objects of non-copy-constructible types.
```c++
te::AnySet<> set;
// add unique_ptr<int> to the set via move-construction
auto [pos, emplaced] = set.emplace<std::unique_ptr<int>>(std::make_unique<int>(1));
// set == {<std::unique_ptr<int> to int(1)>}

// `emplaced` is a bool indicating whether the emplacement succeeded.
assert(emplaced);
// `pos` iterator points to the emplaced element
assert(**pos == 1);
```

Movable types can be `insert()`'d:
```c++
te::AnySet<> set;
std::unique_ptr<double> p(std::make_unique<double>(3.1415));
auto [pos, inserted] = set.insert(std::move(p));
// set == {<std::unique_ptr<double> to double(3.1415)>}

// `inserted` is a bool indicating whether the insertion succeeded.
assert(inserted);
// `pos` iterator points to the inserted element
assert(**pos = 3.1415);

set.insert(std::make_unique<int>(2));
// set == {<std::unique_ptr<double> to double(3.1415)>, <std::unique_ptr<int> to int(2)>}

assert(inserted);
assert(**pos == 2);
```

## Adding Non-Movable Values
Any constructible type that is also hashable and equality-comparable can be added to an AnySet instance.
```c++
namespace custom {

// Custom `LockGuard` type so that we can overload operator==() and
// hash_value() without poisoning namespace std.
struct LockGuard: public std::lock_guard<std::mutex> {
        using std::lock_guard<std::mutex>::lock_guard;
};

// needs to be equality-comparable
bool operator==(const LockGuard& l, const LockGuard& r)
{ return &l == &r; }

// needs to be hashable
std::size_t hash_value(const LockGuard& lg)
{ return te::hash_value(&lg); }

} /* namespace custom */

static std::mutex some_mutex;
 ...
te::AnySet<> set;
// LockGuard is neither copy-constructible nor move-constructible
set.emplace<custom::LockGuard>(some_mutex);
```

## Adding Several Values at Once

### Variadic Insertion
Several values can be added simultaneously:
```c++
te::AnySet<> set;
// use parameter-pack insertion
auto result = set.insert("Some String"s, 3.141592, 100, std::bitset<3>(0b010u));
// set == {"Some String"s, 3.141592, 100, std::bitset<3>(0b010u)}

// `result` is a bitset of size <number of values passed to `.insert()`> (4 in this case)
// where the nth bit in the bitset indicates whether the nth argument was successfully inserted.
assert(result.all());

// do it again, this time with a few arguments that exist in `set` already
auto inserted = set.insert("Some String"s, 0.0, 100);

assert(inserted.count() == 2); // 2 values successfully inserted
assert(not inserted[0]);       // "Some String"s already in the set; not inserted
assert(inserted[1]);           // 0.0 not already in the set; inserted
assert(not inserted[2]);       // 100 already in the set; not inserted
```

### Homogeneous Sequence
Sequences of same-type values can be inserted using both iterator range insertion and `initializer_list` insertion.
```c++ 
std::vector<int> v{1, 2, 3, 4, 5};

// use iterator range insertion
auto count = set.insert(v.begin(), v.end());
// set == {1, 2, 3, 4, 5}

// `count` is the number of values successfully inserted
assert(count == 5);

v = {4, 5, 6, 7};

// more iterator range insertion
count = set.insert(v.begin(), v.end());
// set == {1, 2, 3, 4, 5, 6, 7}

assert(count == 2);

// initializer_list insertion
count = set.insert({5, 6, 7, 8, 9, 10});
// set == {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}

assert(count == 2);
```

## Removing Elements
The `.erase()` method can be used to remove elements from the set:

```c++
auto set = te::make_anyset("Some String"s, 3.1415, 100, std::bitset<4>(0b0101u));
auto pos = set.find("Some String"s);

// iterator overload - returns iterator to element after the erased element
pos = set.erase(pos);
// set == {3.141592, 100, std::bitset<3>(0b010u)}

if(pos != set.end())
	std::cout << "Element after 'Some String': " << *pos << std::endl;
else
	std::cout << "'Some String' was at the end of the set" << std::endl;

// range erasure - returns iterator to the element after the last erased element
// erase all values but 3.1415:
pos = set.find(3.1415);

// erase elements before 3.1415
pos = set.erase(set.begin(), pos);
// set = {3.1415, ...}

// erase elements after 3.1415
set.erase(std::next(pos), set.end());
// set = {3.1415}

assert(set.size() == 1);
assert(*pos == 3.1415);

// finally, by-value erasure: give the value of the element to be erased.
auto count = set.erase(100);

// `count` is the number of elements erased, either 0 or 1
assert(count == 0u); // == 0u because 100 was not in the set

count = set.erase(3.1415);
assert(count == 1u); // == 1u because 3.1415 was the last element in the set
assert(set.empty());
```

To completely clear the set, use the `.clear()` method:
```c++
auto set = te::make_anyset("Some String"s, 3.1415, 100, std::bitset<4>(0b0101u));
set.clear();
// set == {}
```

## Interset Operations

### Update (copy)
To add copies of elements from one AnySet instance to another (effectively computing an inplace set union), use the `.update()` method:
```c++
te::AnySet<> dest = {1, 2, 3, 4, 5};
te::AnySet<> src = {4, 5, 6, 7, 8};
dest.update(src);
// dest == {1, 2, 3, 4, 5, 6, 7, 8}
// src == {4, 5, 6, 7, 8} (unmodified)
```
**Note**: Attempting to copy non-copy-constructible values from `src` to `dest` via `.update()` will result in an exception being thrown.  See the [FIXME documentation]() for details.

### Update (move)
To move elements from one AnySet instance to another (effectively computing an inplace set union), use the `.update()` method with an rvalue:
```c++
te::AnySet<> dest = {1, 2, 3, 4, 5};
te::AnySet<> src = {4, 5, 6, 7, 8};
dest.update(std::move(src));
// dest == {1, 2, 3, 4, 5, 6, 7, 8}
// src == {4, 5}
```
**Note**: This overload does not call copy or move constructors of the contained element types, instead this operation splices nodes from `src` to `dest` directly.

### Splicing
To move specific elements from one set to another in an efficient manner, use the `splice` methods.

In the below examples, assume `set1` and `set2` have been declared like so:
```c++
auto set1 = te::make_anyset("Some String"s, 3.1415, 100, std::bitset<4>(0b0101u));
auto set2 = te::make_anyset("Some String"s, 3.1415, 25);
```

**Example: Splice an element from `set2` into `set1` that doesn't already exist in `set1`**
```c++
auto [inserted_pos, next, succeeded] = set1.splice(set2, set2.find(25));
// set1 == {"Some String"s, 3.1415, 100, std::bitset<4>(0b0101u), 25}
// set2 == {"Some String"s, 3.1415}

// splice succeeded in moving the element
assert(succeeded);
// the `inserted_pos` iterator points to element that we moved into `set1`
assert(*inserted_pos == 25);
// `next` is an iterator into `set2` to the element after `25`.
```

**Example: Splice an element from `set2` into `set1` that already exists in `set1`**
```c++
auto [inserted_pos, next, succeeded] = set1.splice(set2, set2.find("Some String"s));
// set1 == {"Some String"s, 3.1415, 100, std::bitset<4>(0b0101u)} (unmodified)
// set2 == {"Some String"s, 3.1415, 25} (unmodified)

// splice did not succeed in moving the element
assert(!succeeded);
// the `inserted_pos` iterator points to the element in `set1` that prevented the splice from succeeding
assert(*inserted_pos == "Some String"s);
// `next` is an iterator into `set2` to the element after `"Some String"s`.
```

**Example: Splice a range of elements from `set2` into `set1`**
```c++
auto [remain_first, remain_last] = set1.splice(set2, set2.begin(), set2.end());
// set1 == {"Some String"s, 3.1415, 100, std::bitset<4>(0b0101u), 25}
// set2 == {"Some String"s, 3.1415} 

// [remain_first, remain_last) is an iterator range to the elements that weren't successfully spliced.
assert(std::distance(remain_first, remain_last) == 2);
if(*remain_first == "Some String"s)
{
	assert(*remain_first == "Some String"s);
	assert(*std::next(remain_first) == 3.1415);
}
else
{
	assert(*remain_first == 3.1415);
	assert(*std::next(remain_first) == "Some String"s);
}
```

The `.splice_or_copy()` methods are equivalent to the `.splice()` methods, except that if the source set (`set2` in the above examples) is not a non-const rvalue, the elements from the source set are copied, rather than moved.  This means that when the copying overloads are called, the source set is not modified.  It also means that if an attempt is made to copy a non-copy-constructible element from the source set, an exception will be thrown (much like `.update()`, see the [FIXME relevant documentation]() for details).

### Node-Based Operations (.pop(), .dup(), and .push())
Node-based operations present a lower-level interface to modifying AnySet instances at the single-node level.

`.pop()` is semantically equivalent to the iterator overload of `.erase()`, except that it returns a `node_handle` instance (effectively a `std::unique_ptr<te::AnyValue<Hash, KeyEqual>>`) to the "erased" element (and an iterator to the following element).  As long as this node handle exists, pointers and references to the popped element are still valid (for the specifics of iterator invalidation, see the [FIXME relevant documentation]()).

`.push()` is the inverse operation to `.pop()`.  `.push()` takes a node handle and adds it to the AnySet instance if it doesn't already exist.  Pointers and references to the value contained in the `node_handle` are not invalidated by this operations so long as either:
1. The value was successfully added.
2. The value was not successfully added, but the returned `node_handle`'s lifetime has not ended.

`.dup()` is similar to `.pop()`, except that it does not remove the element pointed-to by the iterator argument and instead returns a `node_handle` containing a copy of said element.  If the value contained in the pointed-to element is not copy-constructible, an exeption is thrown (much like `.splice_or_copy()`, see the [FIXME relevant documentation]() for details). 

```c++
using node_type = typename te::AnySet<>::node_handle;

te::AnySet<> set1 = te::make_anyset("Some String"s, 3.1415, 100, std::bitset<4>(0b0101u));
te::AnySet<> set2;
auto pos = set.find("Some String"s);
std::string* p = std::addressof(*pos);

node_type node;

// pop "Some String" from `set1`
std::tie(node, pos) = set1.pop(pos);
// set1 == {3.1415, 100, std::bitset<4>(0b0101u)}
// set2 == {}

// okay, `node`'s lifetime has not ended.
std::cout << *p << std::endl;

// push "Some String" into `set2`
std::tie(pos, node) = set2.push(std::move(node));
// set1 == {3.1415, 100, std::bitset<4>(0b0101u)}
// set2 == {"Some String"s}

if(node) // takes the `else` branch in this example
{
	// If the node_handle return by `.push()` is non-empty/non-null, then the operation
	// did not succeed and the handle we passed in was returned back to us.
	assert(p == &te::unsafe_cast<const std::string&>(*node));
	assert(*p == *node);
	
	// In this case `pos` points to the element that prevented the `.push()` from succeeding
	assert(*p == *pos);
}
else
{
	// Otherwise if the returned node is empty/null the operation succeeded and the node 
	// handle we passed in was consumed and added to the set's (`set2`'s) contents.
	assert(p == &te::unsafe_cast<const std::string&>(*pos));

	// In this case `pos` points to the new position of the `.push()`'d node.
	assert(*p == *pos);
}

// Still okay; the object pointed to by `p` is guaranteed to still be alive and well 
// because we held on to the node handle returned by `.push()` ... although in this 
// specific example we didn't actually need to do that.
use(*p);

// We also know that `pos` is valid because it either points to `*p` or an object 
// equal to `*p` (in this case, it just points to `*p`

// duplicate the node at `pos` in `set2` and add it to `set1`
std::tie(node, pos) = set1.push(set2.dup(pos));
// set1 == {"Some String"s, 3.1415, 100, std::bitset<4>(0b0101u)}
// set2 == {"Some String"s}

assert(!node);
assert(*pos == "Some String"s);
assert(set1.contains("Some String"s));
assert(set2.contains("Some String"s));
assert(set1.contains_value(*pos));
assert(set2.contains_value(*pos));
```

# Element Lookup and Similar Operations

## Find an Element
The `.find()` member function can be used to obtain an iterator to an element with a specific value:
```c++
te::AnySet<> set = te::make_anyset("Some String"s, 3.1415, 100, std::bitset<4>(0b0101u));

// get an iterator to "Some String"s
auto pos = set.find("Some String"s);
assert(*pos == "Some String"s);

// get an iterator to "Some Other String"s
auto pos = set.find("Some Other String"s);
if(pos == set.end()) // evaluates to 'true' in this example
	std::cout << "'Some Other String' not in the set." << std::endl;
else
	std::cout << "Found 'Some Other String': " << *pos << std::endl;
```
Outputs:
```
'Some Other String' not in the set.
```

## Check if an Object Exists
The `.contains()`, `.contains_eq()`, `.contains_value()`, `.contains_value_eq()`, and `.count()` methods can be used to query whether a set contains a given value.

### `.contains()` and `.contains_eq()`
Use the `.contains()` method to check if an element in the set compares equal to the given value using the `te::AnySet<HashFn, KeyEqual, Allocator>` instance's `KeyEqual` comparison function object.  The `.contains_eq()` method overrides this behavior and uses `operator==()` to compare the objects (by default, the comparison object is `std::equal_to<>`; in this case, both methods are semantically equivalent).

The `.count()` method is equivalent to the `.countains()` method, except that its return type is `size_type` (returns either 0 or 1).

**Example: Default behavior with std::equal_to<>**
```c++
te::AnySet<> set = te::make_anyset("Some String"s, 3.1415, 100, std::bitset<4>(0b0101u));

// With default template parameters, `.contains()` and `.contains_eq()` are identical
assert(set.contains("Some String"s));
assert(set.contains_eq("Some String"s));
assert(not set.contains(2.5));
assert(not set.contains_eq(2.5));
```

**Example: Non-standard comparison function object**
```c++
// KeyEqual type that only checks if the values return the same value for `.size()`.
struct SizeEquals {
	template <class T, class U>
	bool operator()(const T& l, const U& r) const
	{ return l.size() == r.size(); }
};

// Hash function that just hashes the result of `.size()` 
struct SizeHash {
	template <class T>
	std::size_t operator()(const T& v) const
	{ return te::hash_value(v.size()); }
};

 ...

te::AnySet<SizeHash, SizeEquals> set({
	"a"s, "ab"s, "abc"s, "abcd"s, "abcde"s
});

// Works as expected:
assert(set.contains("abc"s));
assert(set.contains_eq("abc"s));

// The subtlety is here:
assert(set.contains("123"s));        // only the sizes are compared: "abc"s.size() == "123"s.size()
assert(not set.contains_eq("123"s)); // the actual strings, themselves, are compared: "abc"s != "123"s

// Type safety still matters though:
assert(not set.contains(std::bitset<3>()));    // No object of type `std::bitset<3>` is in the set
assert(not set.contains_eq(std::bitset<3>())); // Same
```

### `.contains_value()` and `.contains_value_eq()`
`.contains_value()` and `.contains_value_eq()` are restricted versions of `.contains()` and `.contains_eq()` that only accept a parameter of type `te::AnyValue<HashFn, KeyEqual>` (possibly obtained from another `te::AnySet<HashFn, KeyEqual>` instance).  These functions return true if the set contains an element that compares equal to, and has has the same type as, the object contained in the provided AnyValue instance.  `.contains_value()` uses the AnySet's `KeyEqual` comparison function object to check for equality, while `.contains_value_eq()` uses `operator==()`.  

```c++
// KeyEqual type that only checks if the values return the same value for `.size()`.
struct SizeEquals {
	template <class T, class U>
	bool operator()(const T& l, const U& r) const
	{ return l.size() == r.size(); }
};

// Hash function that just hashes the result of `.size()` 
struct SizeHash {
	template <class T>
	std::size_t operator()(const T& v) const
	{ return te::hash_value(v.size()); }
};

 ...

te::AnySet<SizeHash, SizeEquals> set({
	"a"s, "ab"s, "abc"s, "abcd"s, "abcde"s
});

te::AnySet<SizeHash, SizeEquals> other({
	"1"s, "12"s, "123"s, "1234"s, "12345"s
});

const auto& any_v = *set.find("abc"s);

assert(set.contains_value(any_v));   // only the sizes are compared: "abc"s.size() == "abc"s.size()
assert(other.contains_value(any_v)); // only the sizes are compared: "abc"s.size() == "123"s.size()

assert(set.contains_value_eq("abc"s));       // the actual values of the strings are compared: "abc"s == "abc"s
assert(not other.contains_value_eq("abc"s)); // the actual values of the strings are compared: "abc"s != "123"s
```

**Note**: Presently `.contains()` and `.contains_eq()` also behave this way when given an AnyValue instance.  This may be changed in the future in the name of not having a special case, so users should not rely on this.

# Core Set-Theoretic Operations
To enable [core set theoretic operations](https://en.wikipedia.org/wiki/Set_%28abstract_data_type%29#Core_set-theoretical_operations) and their corresponding operator overloads, include the header `SetOperations.h`:
```c++
#include "anyset/SetOperations.h"
```
This header includes these operations:

| Operation                   | Free Function                        | Operators                              |
|:----------------------------|:-------------------------------------|:---------------------------------------|
| Set Union                   | `union_of(a, b, ...)`                | `a + b`, `a += b`, `a \| b`, `a \|= b` |
| Set Intersection            | `intersection_of(a, b, ...)`         | `a & b`, `a &= b`                      |
| Set Symmetric Difference    | `symmetric_difference_of(a, b, ...)` | `a ^ b`, `a ^= b`                      |
| Set (Asymmetric) Difference | `difference_of(a, b, ...)`           | `a - b`, `a -= b`                      |
| Is Subset                   | `is_subset_of(a, b)`                 | `a <= b`                               |
| Is Subset (strict)          |                                      | `a < b`                                |
| Is Superset                 | `is_superset_of(a, b)`               | `a >= b`                               |
| Is Superset (strict)        |                                      | `a > b`                                |

**Note**: `difference_of(a, b, ...)` is equivalent to `difference_of(a, union_of(b, ...))`.

The header addionally includes an overload of the `<<` stream operator for writing the contents of the set to an instance of `std::ostream`:
```c++
auto set = te::make_anyset("Some String"s, std::bitset<4>{0b0101u}, 100, 3.1415);
std::cout << set << std::endl;
```
Possible Output (particular order not guaranteed):
```
{Some String, 0101, 100, 3.1415}
```

# AnyValue (AnySet's `value_type`)
The class template `te::AnyValue<HashFn, Compare>` is a `std::any`-like type that is capable of storing `const` objects of any constructible type.  AnyValue can only store *values*, object references cannot be stored in AnyValue instances (but pointers can).  AnyValue objects have no public constructors and are neither copyable nor movable.  AnyValue is *not* a proxy class (like `std::vector<bool>::refernce`): AnySets store actual instances of AnyValue, and expose them in its public interface.  

One can make use of AnySet without ever interacting directly with AnyValue's interface, but it provides some powerful methods of accessing the contained value that are worth knowing.

## Accessing the Contained Value Statically

### `exact_cast<T>()`
`exact_cast<T>()` obtains a reference or pointer to (or copy of) the value contained in an AnyValue instance with a specified type.  `exact_cast<T>()` succeeds only when the contained object's type matches the casted-to type exactly (modulo references and `const`/`volatile` qualifiers).  A reference to AnyValue can be `exact_cast()`'d to a reference to `const T`, while a pointer to AnyValue can be casted to pointer to `const T`.  

```c++
auto set = te::make_anyset("Some String"s, 3.1415, 100, std::bitset<4>(0b0101u));
// get a reference to the AnyValue instance containing "Some String"s
const auto& any_v = *set.find("Some String"s);

// use exact_cast() to access the contained string.
const std::string& str = te::exact_cast<const std::string&>(any_v);

// throws std::bad_cast: any_v doesn't contain an 'int'
// const int& i = te::exact_cast<const int&>(any_v);

// get a pointer to the contained string
const std::string* p_str = te::exact_cast<const std::string*>(&any_v);
assert(p_str);
assert(p_str == &str);

// throws std::bad_cast: any_v doesn't contain a 'std::string*'
// const std::string* p = te::exact_cast<const std::string*>(any_v);

// returns null
const int* p_int = te::exact_cast<const int*>(&any_v);
assert(not p_int);
```

### `unsafe_cast<T>()`
`unsafe_cast<T>()` obtains a reference to (or copy of) the value contained in an AnyValue instance with a specified type.  `unsafe_cast<T>()` does no type checking; an `unsafe_cast()` that casts to a type other than the exact type of contained object (modulo references and `const`/`volatile` qualifiers) invokes undefined behavior.  A reference to AnyValue can be `exact_cast()`'d to a reference to `const T`.  Unlike `exact_cast()`, `unsafe_cast()` has no overloads for a pointers to AnyValue instances.

```c++
auto set = te::make_anyset("Some String"s, 3.1415, 100, std::bitset<4>(0b0101u));
// get a reference to the AnyValue instance containing "Some String"s
const auto& any_v = *set.find("Some String"s);

// use unsafe_cast() to access the contained string
const std::string& str = te::unsafe_cast<const std::string&>(any_v);

// undefined behavior: any_v doesn't contain an 'int'
// const int& i = te::exact_cast<const int&>(any_v);

// fails to compile
// const std::string* p_str = te::exact_cast<const std::string*>(&any_v);

// undefined behavior: any_v doesn't contain a 'std::string*'
// const std::string* p = te::exact_cast<const std::string*>(any_v);
```

## Accessing the Contained Value Dynamically

### `polymorphic_cast<T>()`
`polymorphic_cast<T>()` is the only cast function capable of casting through the inheritence heirarchy of an AnyValue's contained object's type.  `polymorphic_cast<T>()` cannot cast to final or non-class type (such a cast fails to compile), and will fail at runtime if the contained object is of final or non-class type, even if the target type is an accessible base of the contained type.  The contained object does not need to be of polymorphic type for `polymorphic_cast<T>()` to successfully cast through its inheritence heirarchy.

```c++
namespace silly {
// silly inheritence heirarchy; non-polymorphic
struct MyString:
	public std::string,
	public std::bitset<8>
{
	MyString(const std::string& s): std::string(s) { }
};

// sillier yet; introduce a polymorphic type into the heirarchy
struct MyStringError:
	public MyString,
	public std::runtime_error
{
	MyStringError(const std::string& s):
		MyString(s), std::runtime_error("Error: " + s)
	{
		
	}
};

// overloads so ADL can find hash_value() in AnyHash::operator()
std::size_t hash_value(const MyString& s)
{ return te::hash_value(std::string_view(s)); }

std::size_t hash_value(const MyStringError& s)
{ return te::hash_value(std::string_view(s)); }

} /* namespace silly */

 ...

using H = te::AnyHash;
using E = std::equal_to<>;
using namespace std::literals;
using namespace silly;
using any_t = te::AnyValue<H, E>;
{
	std::unique_ptr<any_t> my_string = 
		te::make_any_value<MyString, H, E>(te::AnyHash{}, "Some string."s);
	const auto& as_my_string = te::polymorphic_cast<const MyString&>(*my_string);
	const auto& as_string = te::polymorphic_cast<const std::string&>(*my_string);
	const auto& as_bitset = te::polymorphic_cast<const std::bitset<8>&>(*my_string);
	std::cout << "std:string: " << as_string << '\n';
	std::cout << "bitset<8>:  " << as_bitset << '\n' << std::endl;
}
{
	std::unique_ptr<any_t> my_string = 
		te::make_any_value<MyStringError, H, E>(te::AnyHash{}, "Some string error message."s);
	const auto* as_my_string = te::polymorphic_cast<const MyString*>(my_string.get());
	assert(static_cast<bool>(as_my_string));
	const auto* as_string = te::polymorphic_cast<const std::string*>(my_string.get());
	const auto* as_bitset = te::polymorphic_cast<const std::bitset<8>*>(my_string.get());
	const auto* as_exception = te::polymorphic_cast<const std::exception*>(my_string.get());
	std::cout << "std:string: " << *as_string << '\n';
	std::cout << "bitset<8>:  " << *as_bitset << '\n';
	std::cout << ".what():    " << as_exception->what() << std::endl;
}
```

Outputs:
```
std:string: Some string.
bitset<8>:  00000000

std:string: Some string error message.
bitset<8>:  00000000
.what():    Error: Some string error message.
```

## Higher-Level Accessors

### `get<T>()`, `as<T>()` and `try_as<T>()`
`as<T>()` attempts to access the contained value in an `AnyValue` instance by first attempting an `exact_cast<T>()` followed by a `polymorphic_cast<T>()`.  If both casts fail, then a `std::bad_cast` is thrown.  If either cast succeeds, a reference to the contained object is returned.  The `polymorphic_cast<T>()` is only attempted if the target type of the cast is a non-final class type.  `as<T>()` can only be called on a reference to AnyValue and returns a const reference to the target type, `T`.  The target type `T`, must be absent of any top-level `const/volatile` qualifiers or reference decorations (i.e. it should be "just the type").

`get<T>()` is equivalent to `as<T>()`.

```c++
auto set = te::make_anyset("Some String"s, 3.1415, 100, std::bitset<4>(0b0101u));
// get a reference to the AnyValue instance containing "Some String"s
const auto& any_v = *set.find("Some String"s);

// use as<std::string>() to access the contained string
const std::string& str = te::as<std::string>(any_v);

// fails to compile 
// const std::string& str = te::as<const std::string&>(any_v);

// throws std::bad_cast: any_v doesn't contain an 'int'
// const int& i = te::as<int>(any_v);
```

`try_as<T>()` is nearly identical to `as<T>()` except that it returns a pointer to the contained value instead of a reference.  In the event that the cast fails, no exceptions are thrown, instead the returned pointer is null.

```c++
auto set = te::make_anyset("Some String"s, 3.1415, 100, std::bitset<4>(0b0101u));
// get a reference to the AnyValue instance containing "Some String"s
const auto& any_v = *set.find("Some String"s);

// use try_as<std::string>() to access the contained string
const std::string* str = te::try_as<std::string>(any_v);
assert(str);

// returns null
const int* i = te::try_as<int>(any_v);
assert(not i);

// fails to compile 
// const std::string& str = te::try_as<const std::string&>(any_v);

// fails to compile: return type of the cast would be `const std::string**` 
//                   but `str` has type `const std::string*`
// const std::string* str = te::try_as<const std::string*>(any_v);
```


### `get_default_ref<T>()` and `get_default_val<T>()`
`get_default_ref<T>()` is nearly identical to `as<T>()` except that instead of throwing a `std::bad_cast` on failure, returns a given 'default' value, by reference.  The default value must be an lvalue reference to an object of the same type as the target type.

```c++
auto set = te::make_anyset("Some String"s, 3.1415, 100, std::bitset<4>(0b0101u));
// get a reference to the AnyValue instance containing "Some String"s
const auto& any_v = *set.find("Some String"s);

// use as<std::string>() to access the contained string
const std::string empty_str = "";
const std::string& str = te::get_default_ref<std::string>(any_v, empty_str);
assert(str == "Some String"s);
assert(str != empty_str);


// try to access 'any_v' as an 'int'
int default_int = -1;
const int& i = te::get_default_ref<int>(any_v, default_int);
assert(&i == &default_int); // any_v doesn't contain an 'int'; default_int returned

// fails to compile: default argument is not of type 'double'
// const double& dbl = te::get_default_ref<double>(any_v, -1);

// fails to compile: default argument is an rvalue (must be an lvalue)
// const double& dbl = te::get_default_ref<double>(any_v, -1.0);
```

`get_default_val<T>()` is similar to `get_default_ref<T>()` except that it returns by value.  The default value must be convertible to the target type (the conversion is done via an explicit static_cast<T>()).

```c++
auto set = te::make_anyset("Some String"s, 3.1415, 100, std::bitset<4>(0b0101u));
// get a reference to the AnyValue instance containing "Some String"s
const auto& any_v = *set.find("Some String"s);

// use get_default_val<std::string>() to access the contained string
std::string str = te::get_default_ref<std::string>(any_v, empty_str);
assert(str == "Some String"s);

// try to access 'any_v' as an 'int'
const int& i = te::get_default_val<int>(any_v, -1);
assert(i == -1); // any_v doesn't contain an 'int'; default '-1' is returned 

// compiles fine: default argument is convertible to 'double'
const double& dbl = te::get_default_val<double>(any_v, -1);

assert(dbl == -1.0); // any_v doesn't contain an 'double'; default '-1.0' is returned 
```


# Customizing AnyHash
This method of customizing AnyHash's behavior should be preferred when the access to the namespace of the to-be-hashed type is prohibitted (such as providing a hash function definition for `std::vector`).

The default hash function object type for `te::AnySet<>` is `te::AnyHash`.  This type is designed to make adding your own hash functions as painless as possible.  `te::AnyHash<>` is compatible with [Boost.ContainerHash](https://www.boost.org/doc/libs/1_67_0/doc/html/hash.html) out of the box (no boost headers are included; strictly an ADL-based approach) and it automatically absorbs all specializations of `std::hash<>`.

The header `extra-hash.h` provides a minimal set of building blocks (again, compatible with Boost.ContainerHash) for writing hash functions for any type, plus a few "no brainer" specializations of `te::Hash` for standard types like `std::pair` (see the [FIXME relevant documentation]() for details).

The primary methods of customizing `te::AnyHash`'s behavior follow:

## Template Specialization: Specializing te::Hash
`te::Hash<>` is a class template in the style of `std::hash<>`.  In fact, here is `te::Hash<>`'s implementation:
```c++
namespace te {

template <class T = void>
struct Hash: public std::hash<T> {}

// void specialization in the style of <functional> header function objects
template <>
struct Hash<void> {
	template <class T>
	std::size_t operator()(const T& v) const 
	{ return std::invoke(te::Hash<T>{}, v); }
};

} /* namespace te */
```

Users may specialize `te::Hash<>` for any type other than `void`.  For example, here's one way of defining a hash function for std::vector:
```c++
// for te::Hash
#include "anyset/AnyHash.h"
// for te::hash_combine
#include "anyset/extra-hash.h"
// for std::vector
#include <vector>
// for std::accumulate
#include <algorithm>

template <class T, class A>
struct te::Hash<std::vector<T, A>> {
	std::size_t operator()(const std::vector<T, A>& v) const
	{
		return std::accumulate(begin(v), end(v), std::size_t(0), 
			[](const auto& l, const auto& r) {
				using te::hash_value;
				return te::hash_combine(hash_value(l), hash_value(r));
			}
		);
	}
};
```

## ADL: Overloading `hash_value()` at Namespace Scope
When users are blessed with access to the enclosing namespace of the class being specialized, this method should be preferred.  This is also the method used by Boost.ContainerHash.

Users can provide an overload of the free function `hash_value()` in the same namespace as the to-be-hashed type.  This overload will be found by ADL when calling `te::AnyHash::operator()`.

For example:
```c++
namespace company {

// The type we want to provide a hash function for
struct Employee {
	const std::size_t department_number;
	const std::string first_name;
	const std::string last_name;
};

// This overload makes `struct Employee` hashable by `te::AnyHash`
std::size_t hash_value(const Employee& emp)
{
	using te::hash_value;
	return te::hash_combine(
		hash_value(emp.department_number),
		hash_value(emp.first_name),
		hash_value(emp.last_name)
	);
}

} /* namespace company */
```



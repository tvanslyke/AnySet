# AnySet Tutorials

## Quick Start
To use `te::AnySet` include the header `AnySet.h`:
```c++
#include "anyset/AnySet.h"
```

This header exposes `te::AnySet`, `te::AnyValue`, `te::AnyHash`, `te::Hash`


## Constructing an AnySet Object
Shown here are some of the ways that one can initialize an AnySet object.

### Initializing with Homogeneous Elements
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

### Initializing with Heterogeneous Elements
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

## Modifying Sets: Adding and Erasing Elements

### Adding Non-Copy-Constructible Values
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

### Adding Non-Movable Values
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

### Adding Several Values at Once

#### Variadic Insertion
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

#### Homogeneous Sequence
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

### Adding Values from Another AnySet Instance

#### Update (copy)
To add copies of elements from one AnySet instance to another (effectively computing an inplace set union), use the `.update()` method:
```c++
te::AnySet<> dest = {1, 2, 3, 4, 5};
te::AnySet<> src = {4, 5, 6, 7, 8};
dest.update(src);
// dest == {1, 2, 3, 4, 5, 6, 7, 8}
// src == {4, 5, 6, 7, 8} (unmodified)
```
**Note**: Attempting to copy non-copy-constructible values from `src` to `dest` via `.update()` will result in an exception being thrown.  See the [FIXME documentation]() for details.

#### Update (move)
To move elements from one AnySet instance to another (effectively computing an inplace set union), use the `.update()` method with an rvalue:
```c++
te::AnySet<> dest = {1, 2, 3, 4, 5};
te::AnySet<> src = {4, 5, 6, 7, 8};
dest.update(std::move(src));
// dest == {1, 2, 3, 4, 5, 6, 7, 8}
// src == {4, 5}
```
**Note**: This overload does not call copy or move constructors of the contained element types, instead this operation splices nodes from `src` to `dest` directly.

#### Splicing
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

#### Node-Based Operations (.pop(), .dup(), and .push())
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


### Core Set-Threoretic Operations
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



# AnySet Tutorial


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

## Adding Elements

### Adding Non-Copy-Constructible Values
Use `AnySet::emplace<T>()` to add objects of non-copy-constructible types.
```c++
te::AnySet<> set;
// add unique_ptr<int> to the set via move-construction
set.emplace<std::unique_ptr<int>>(std::make_unique<int>(1));
```

Movable types can be `insert()`'d:
```c++
te::AnySet<> set;
std::unique_ptr<double> p(std::make_unique<double>(3.1415));
set.insert(std::move(p));
set.insert(std::make_unique<int>(2));
```

#### Adding Non-Movable Values
Any constructible type that is also hashable and equality-comparable can be added to an AnySet instance.
```c++
namespace custom {

struct LockGuard: public std::lock_guard<std::mutex> {
        using std::lock_guard<std::mutex>::lock_guard;
};

// needs to be equality-comparable
bool operator==(const LockGuard& l, const LockGuard& r)
{ return &l == &r; }

// needs to be hashable
std::size_t hash_value(const LockGuard& lg)
{
	using te::hash_value;
	return hash_value(&lg);
}

} /* namespace custom */

static std::mutex some_mutex;
 ...
te::AnySet<> set;
// LockGuard is neither copy-constructible nor move-constructible
set.emplace<custom::LockGuard>(some_mutex);
```

### Adding Several Values at Once
Several values can be added simultaneously:
```c++
te::AnySet<> set;
set.insert("Some String"s, 3.141592, 100, std::bitset<3>(0b010u));
// set == {"Some String"s, 3.141592, 100, std::bitset<3>(0b010u)}
```






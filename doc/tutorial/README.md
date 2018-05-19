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
	std::forward_as_tuple("Some String"s, 3.141592, 100, std::bitset<3>{0b010})
);
```

Constructing from a parameter pack:
```c++
te::AnySet<> set(te::make_anyset(std::forward<Args>(args)...));
```

## Adding Elements

### Adding Non-Copy-Constructible Values
Use `AnySet::emplace<T>()` to add objects of non-copy-constructible types.
```c++
te::AnySet<> set;
```


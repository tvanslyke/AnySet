#include <iostream>
#include <string>
#include <bitset>
#include <vector>
#include <numeric>
#include <algorithm>

// for te::AnySet<>
#include "anyset/AnySet.h"
// for te::hash_combine
#include "anyset/extra-hash.h"
// So we can stream AnySet objects with operator<<()
#include "anyset/SetOperations.h"


// Specialize te::Hash<> for std::vector.
template <class T, class Alloc>
struct te::Hash<std::vector<T, Alloc>>
{
	std::size_t operator()(const std::vector<T, Alloc>& vec) const
	{
		auto accumulate_op = [](const auto& l, const auto& r) {
			using te::hash_value;
			return te::hash_combine(hash_value(l), hash_value(r));
		};
		return std::accumulate(begin(vec), end(vec), std::size_t(0), accumulate_op);
	}
};

namespace example {

// A silly example class.
struct NameList:
	public std::vector<std::string>
{
	using std::vector<std::string>::vector;
};

} /* namespace example */

// Specialization for NameList.
template <>
struct te::Hash<example::NameList>: 
	public te::Hash<std::vector<std::string>> // just inherit operator()
{

};

namespace example {

// Let's make NameList objects streamable.
std::ostream& operator<<(std::ostream& os, const NameList& s)
{
	os << "NameList([";
	if(s.size() > 0u)
	{
		os << '"' << s.front() << '"';
		for(auto pos = std::next(s.begin()); pos != s.end(); ++pos)
			os << ", " <<  '"' << *pos << '"';
	}
	return os << "])";
}


// Another silly example class.
struct Sentence:
	public std::vector<std::string>
{
	using std::vector<std::string>::vector;
};

// Provide an overload of hash_value() in the style of boost::container_hash
std::size_t hash_value(const Sentence& s)
{
	using te::hash_value;
	return hash_value(static_cast<const std::vector<std::string>&>(s));
}

// Let's make Sentence objects streamable as well.
std::ostream& operator<<(std::ostream& os, const Sentence& s)
{
	os << "Sentence('";
	if(s.size() > 0u)
	{
		os << s.front();
		for(auto pos = std::next(s.begin()); pos != s.end(); ++pos)
			os << ' ' << *pos;
		os << '.';
	}
	return os << "')";
}

} /* namespace example */


// While we're at it, why not make std::vector<std::string> streamable.
std::ostream& operator<<(std::ostream& os, const std::vector<std::string>& s)
{
	os << "std::vector([";
	if(s.size() > 0u)
	{
		os << s.front();
		for(auto pos = std::next(s.begin()); pos != s.end(); ++pos)
			os << ", " << *pos;
	}
	return os << "])";
}

int main()
{
	using namespace std::literals;
	using example::NameList;
	using example::Sentence;

	// A list of names.
	const NameList name_list {
		"Herb"s, "Bjarne"s, "Kate"s, "Howard"s, "Sean"s
	};
	
	// A sentence.
	const Sentence sentence{"This"s, "is"s, "a"s, "sentence"s};

	te::AnySet<> set;
	set.insert(name_list);
	set.insert(sentence);
	
	// Erm, not really a great example of a sentence.
	const Sentence bad_sentence{"Herb"s, "Bjarne"s, "Kate"s, "Howard"s, "Sean"s};

	using te::hash_value;
	// Technically true.
	assert(bad_sentence == name_list);
	assert(hash_value(bad_sentence) == hash_value(name_list));

	// So 'bad_sentence' and 'name_list' have the same hash and compare equal...
	assert(not set.contains(bad_sentence)); // ... but that doesn't mean 'bad_sentence' is in set.
	
	{
		// So let's put it in the set!
		std::cout << "Before inserting 'bad_sentence': " << set << '\n';
		auto [pos, inserted] = set.insert(bad_sentence);
		std::cout << "After inserting 'bad_sentence':  " << set << '\n';
		// Just to prove it to you!
		assert(inserted);
		assert(pos->typeinfo() == typeid(bad_sentence));
		assert(*pos == bad_sentence);
		assert(*pos != name_list);

		// And then erase it again.  After all, this is an example.
		set.erase(bad_sentence);
		std::cout << "After erasing 'bad_sentence':    " << set << '\n';
		assert(not set.contains(bad_sentence));
	}

	// Let's go crazy 
	set.insert(
		"Some string."s,
		std::vector<int>{0, 1, 2, 3},
		3.14159265,
		std::bitset<8>{0b01010101}
	);

	std::cout << "After inserting a bunch of crazy stuff:    " << set << '\n';
	
	// Now let's print all instances of std::vector<std::string>
	std::cout << "Printing all instances of 'std::vector<std::string>' in the set:" << std::endl;
	for(const auto& value: set)
	{
		auto vec_p= te::polymorphic_cast<const std::vector<std::string>*>(&value);
		if(static_cast<bool>(vec_p))
		{
			std::cout << '\t' << "Vector of strings: " << *vec_p << ' ';
			if(te::is<Sentence>(value))
				std::cout << "(really it's a 'Sentence')" << '\n';
			else if(te::is<NameList>(value))
				std::cout << "(really it's a 'NameList')" << '\n';
			else
				assert(false);
		}
	}
	return 0;
}

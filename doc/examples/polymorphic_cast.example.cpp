#include "AnySet.h"
#include <iostream>
#include <string>
#include <stdexcept>


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

// overloads so ADL can find compute_hash() in AnyHash::operator()
std::size_t compute_hash(const MyString& s)
{ return te::compute_hash(std::string_view(s)); }

std::size_t compute_hash(const MyStringError& s)
{ return te::compute_hash(std::string_view(s)); }

} /* namespace silly */

int main() {
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
	// Outputs:
	// std:string: Some string.
	// bitset<8>:  00000000
	// 
	// std:string: Some string error message.
	// bitset<8>:  00000000
	// .what():    Error: Some string error message.
}


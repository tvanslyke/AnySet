cmake_minimum_required(VERSION 3.8)
project(test-any-set)

set(CMAKE_CXX_STANDARD 17)
include_directories("./../")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0")
add_executable(test-any-set
	tests/test-main.cpp
	tests/bucket_size.cpp
	tests/bucket_count.cpp 
	tests/bucket.cpp 
	tests/clear.cpp
	tests/count.cpp
	tests/emplace.cpp
	tests/empty.cpp
	tests/eq.cpp
	tests/equal_range_const.cpp
	tests/equal_range_nonconst.cpp
	tests/erase.cpp
	tests/find.cpp
	tests/insert.cpp
	tests/iterators.cpp
	tests/load_factor.cpp
	tests/local_iterators.cpp
	tests/max_bucket_count.cpp
	tests/max_load_factor.cpp
	tests/rehash.cpp
	tests/reserve.cpp
	tests/size.cpp
	tests/swap_member.cpp
	tests/constructor.cpp
)



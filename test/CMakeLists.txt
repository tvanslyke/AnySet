cmake_minimum_required(VERSION 3.8)
project(test-any-set)

set(CMAKE_CXX_STANDARD 17)
include_directories("./../include/")

add_executable(test-anyset
	tests/test-main.cpp
	tests/assign.cpp
	tests/bucket_size.cpp
	tests/bucket_count.cpp 
	tests/bucket.cpp 
	tests/clear.cpp
	tests/constructor.cpp
	tests/contains.cpp
	tests/count.cpp
	tests/dup.cpp
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
	tests/push-pop.cpp
	tests/rehash.cpp
	tests/reserve.cpp
	tests/size.cpp
	tests/splice.cpp
	tests/splice_or_copy.cpp
	tests/swap_member.cpp
	tests/update.cpp
	tests/set-operations/union_of.cpp
	tests/set-operations/intersection_of.cpp
	tests/set-operations/difference_of.cpp
	tests/set-operations/symmetric_difference_of.cpp
	tests/set-operations/subset_superset.cpp
	tests/value-operations/polymorphic_cast.cpp
	tests/value-operations/exact_cast.cpp
	tests/value-operations/unsafe_cast.cpp
	tests/value-operations/as.cpp
)



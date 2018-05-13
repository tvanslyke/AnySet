cmake_minimum_required(VERSION 3.8)
project(any-set-docs)

find_package(Doxygen)

if(DOXYGEN_FOUND)
	doxygen_add_docs(docs ./../include/AnySet.h ./../include/AnyNode.h)
endif(DOXYGEN_FOUND)

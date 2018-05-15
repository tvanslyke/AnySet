cmake_minimum_required(VERSION 3.8)
project(any-set-docs)

find_package(Doxygen)

if(DOXYGEN_FOUND)
	set(DOXYGEN_LAYOUT_FILE "DoxygenLayout.xml")
	doxygen_add_docs(docs ./../include/SetOperations.h ./../include/extra-hash.h ./../include/AnyHash.h ./../include/AnySet.h ./../include/AnyNode.h)
endif(DOXYGEN_FOUND)

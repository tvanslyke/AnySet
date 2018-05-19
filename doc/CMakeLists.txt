cmake_minimum_required(VERSION 3.8)
project("AnySet Documentation")
find_package(Doxygen)

if(DOXYGEN_FOUND)
	set(DOXYGEN_LAYOUT_FILE "DoxygenLayout.xml")
	set(DOXYGEN_EXAMPLE_PATH "examples/")
	doxygen_add_docs(
		docs
		./../include/anyset/SetOperations.h
		./../include/anyset/extra-hash.h
		./../include/anyset/AnyHash.h
		./../include/anyset/AnyList.h
		./../include/anyset/AnySet.h
		./../include/anyset/AnyNode.h
		./../include/anyset/CompressedPair.h 
		./../include/anyset/ValueHolder.h 
	)
endif(DOXYGEN_FOUND)

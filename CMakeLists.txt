cmake_minimum_required(VERSION 3.8)

project(anyset VERSION 0.1.0 LANGUAGES CXX)


add_library(AnySet INTERFACE)
target_include_directories(AnySet
	INTERFACE $<INSTALL_INTERFACE:include>
)

target_compile_features(AnySet INTERFACE cxx_std_17)
add_subdirectory(test)
add_subdirectory(doc)

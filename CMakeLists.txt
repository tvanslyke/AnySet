cmake_minimum_required(VERSION 3.8)

project(anyset VERSION 0.1.0 LANGUAGES CXX)


add_library(AnySet INTERFACE)
target_include_directories(AnySet INTERFACE 
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include/anyset>
    $<INSTALL_INTERFACE:include/anyset>
)

target_compile_features(AnySet INTERFACE cxx_std_17)

add_subdirectory(test EXCLUDE_FROM_ALL)
add_subdirectory(doc EXCLUDE_FROM_ALL)

install(
	DIRECTORY include/anyset 
	INCLUDE DESTINATION include 
	FILES_MATCHING PATTERN "*.h"
)

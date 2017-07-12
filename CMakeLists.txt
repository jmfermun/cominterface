#============================================================================
# Name        : CMakeLists.txt
# Author      : Juan Manuel Fernández Muñoz
# Date        : July, 2017
# Description : CMake configuration file for ComInterface library
#============================================================================

# CMake minimum required version
cmake_minimum_required(VERSION 2.8)

# Project name
project(cominterface)

# Compile options
if(NOT DEFINED BUILD_SHARED_LIBS)
	set(BUILD_SHARED_LIBS ON CACHE BOOL
		"Compile the library as a shared library." FORCE)
endif(NOT DEFINED BUILD_SHARED_LIBS)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Release" CACHE STRING
		"Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif(NOT CMAKE_BUILD_TYPE)

# Set the output folder where the library will be created
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)

# Boost libraries
set(BOOST_LIBS system)
find_package(Boost COMPONENTS ${BOOST_LIBS} REQUIRED)

# Headers
include_directories(${CMAKE_SOURCE_DIR}/include ${Boost_INCLUDE_DIR})

# ComInterface sources
set(LIBRARY_SRC src/comserial.cpp src/comsocket.cpp)

# ComInterface library
add_library(${PROJECT_NAME} ${LIBRARY_SRC})

# Linker libraries
target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES})

if(WIN32)
  target_link_libraries(${PROJECT_NAME} ws2_32 wsock32)
endif()

# Installation
install(DIRECTORY include/cominterface/
		DESTINATION include/cominterface
		FILES_MATCHING PATTERN "*.hpp")

install(TARGETS ${PROJECT_NAME}
		RUNTIME DESTINATION bin
		LIBRARY DESTINATION lib
		ARCHIVE DESTINATION lib)

# Example
add_subdirectory(example)

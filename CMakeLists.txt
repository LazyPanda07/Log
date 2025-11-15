cmake_minimum_required(VERSION 3.27.0)

set(CMAKE_CXX_STANDARD 20)
option(BUILD_SHARED_LIBS "" OFF)

project(Log VERSION	1.9.1)

if (UNIX)
	add_definitions(-D__LINUX__)

	if (${CMAKE_SYSTEM_NAME} STREQUAL "Android")
		add_definitions(-D__ANDROID__)
	endif()
endif (UNIX)

add_library(
	${PROJECT_NAME} SHARED
	src/Log.cpp
)

target_include_directories(
	${PROJECT_NAME} PUBLIC
	include
)

if (DEFINED ENV{MARCH} AND NOT "$ENV{MARCH}" STREQUAL "")
	target_compile_options(${PROJECT_NAME} PRIVATE -march=$ENV{MARCH})
endif()

install(
	TARGETS ${PROJECT_NAME} 
	ARCHIVE DESTINATION lib
	LIBRARY DESTINATION lib
	RUNTIME DESTINATION dll
)

install(DIRECTORY include DESTINATION .)

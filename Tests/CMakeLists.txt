cmake_minimum_required(VERSION 3.27.0)

set(CMAKE_CXX_STANDARD 20)
set(DLL ${CMAKE_SOURCE_DIR}/../Log)

if (UNIX)
	set(DLL ${DLL}/lib/libLog.so)

	add_definitions(-D__LINUX__)
else ()
	set(DLL ${DLL}/dll/Log.dll)
endif (UNIX)

project(Tests)

add_executable(
	${PROJECT_NAME}
	main.cpp
)

target_include_directories(
	${PROJECT_NAME} PUBLIC
	${CMAKE_SOURCE_DIR}/../src
	gtest/include
)

target_link_directories(
	${PROJECT_NAME} PUBLIC
	${CMAKE_SOURCE_DIR}/../Log/lib
	gtest/lib
)

target_link_libraries(
	${PROJECT_NAME} PUBLIC
	Log
	gtest
	gtest_main
)

install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_BINARY_DIR}/bin)

install(FILES ${DLL} DESTINATION ${CMAKE_BINARY_DIR}/bin)

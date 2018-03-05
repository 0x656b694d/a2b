cmake_minimum_required(VERSION 2.6)
 
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS}, include)

find_package(Boost REQUIRED)

add_executable(runTests test/uttranslator.cpp)
target_link_libraries(runTests ${GTEST_LIBRARIES} pthread)

cmake_minimum_required(VERSION 2.6)
 
macro(use_cxx11)
  if (CMAKE_VERSION VERSION_LESS "3.1")
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
      set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
    endif ()
  else ()
    set (CMAKE_CXX_STANDARD 11)
  endif ()
endmacro(use_cxx11)

use_cxx11()

find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS}, include)

find_package(Boost REQUIRED)

add_executable(runTests test/uttranslator.cpp)
target_link_libraries(runTests ${GTEST_LIBRARIES} pthread)

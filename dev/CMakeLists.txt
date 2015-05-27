cmake_minimum_required(VERSION 2.8)

option(BUILD_ALL      "Enable building examples and tests [default: OFF]" OFF)
option(BUILD_EXAMPLES "Enable building examples [default: OFF]"           OFF)
option(BUILD_TESTS    "Enable building tests    [default: OFF]"           OFF)

set(SO_5_VERSION 5.5.5)

if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU" 
     OR "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang"
     OR "${CMAKE_CXX_COMPILER_ID}" MATCHES "Intel"
)
	add_definitions(-std=c++11 -pthread -mtune=native -march=native -Wall)
	set(CMAKE_EXE_LINKER_FLAGS "-pthread")
	set(CMAKE_SHARED_LINKER_FLAGS "-pthread")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
	# message("Using MSVC compiler")
else ()
    message(FATAL_ERROR "Your C++ compiler not supported.\nPlease mail me on san@masterspline.net Your compiler ID '${CMAKE_CXX_COMPILER_ID}' and this error message.")
endif ()

include_directories( ${CMAKE_CURRENT_LIST_DIR} )

add_subdirectory(so_5)
if(BUILD_ALL OR BUILD_EXAMPLES)
  add_subdirectory(sample/so_5)
endif()
if(BUILD_ALL OR BUILD_TESTS)
  enable_testing()
  add_subdirectory(test/so_5)
  if(WIN32)
    configure_file(${CMAKE_SOURCE_DIR}/cmake/run_tests.bat ${CMAKE_BINARY_DIR} NEWLINE_STYLE WIN32)
  endif()
endif()

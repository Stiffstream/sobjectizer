if(NOT UNITTEST)
    message(FATAL_ERROR "UNITTEST is not defined!")
endif()

if(NOT UNITTEST_SRCFILES)
    set(UNITTEST_SRCFILES main.cpp)
endif()

include(${CMAKE_SOURCE_DIR}/so_5/cmake/target.cmake)

unset(SO_5_TEST_LAUNCHER)
if (ANDROID AND DEFINED ADBRUNNER)
    set(SO_5_TEST_LAUNCHER ${ADBRUNNER})
    list(APPEND SO_5_TEST_LAUNCHER -L$<TARGET_FILE_DIR:${SO_5_SHARED_LIB}>)
endif()

add_executable(${UNITTEST} ${UNITTEST_SRCFILES})
target_link_libraries(${UNITTEST} ${SO_5_SHARED_LIB})
add_test(NAME ${UNITTEST} COMMAND ${SO_5_TEST_LAUNCHER} ${UNITTEST})

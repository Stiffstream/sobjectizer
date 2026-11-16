add_subdirectory(top_level_task)

if(NOT( (CMAKE_CXX_COMPILER_ID STREQUAL "GNU") AND
		(SOBJECTIZER_SANITIZE STREQUAL "address") ) )

	# GCC with address sanitizer doesn't support symmetric transfer
	# for C++20 coroutines.
	add_subdirectory(task_1m)
endif()

add_subdirectory(mchain)

add_subdirectory(agents)

add_subdirectory(testing)

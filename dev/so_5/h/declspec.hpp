/*
	SObjectizer 5.
*/

#if !defined( _SO_5__DECLSPEC_HPP_ )
#define _SO_5__DECLSPEC_HPP_

#include <cpp_util_2/h/detect_compiler.hpp>

#if defined( SO_5_PRJ )
	#define SO_5_TYPE\
	CPP_UTIL_2_EXPORT

	#define SO_5_EXPORT_FUNC_SPEC(ret_type)\
	CPP_UTIL_2_EXPORT_FUNC_SPEC(ret_type)

#else
	#define SO_5_TYPE\
	CPP_UTIL_2_IMPORT

	#define SO_5_EXPORT_FUNC_SPEC(ret_type)\
	CPP_UTIL_2_IMPORT_FUNC_SPEC(ret_type)

#endif

#endif

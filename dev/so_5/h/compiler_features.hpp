/*
	SObjectizer 5.
*/

/*!
 * \since v.5.3.0
 * \file 
 * \brief Detection of compiler version and absence of various features.
 */

#pragma once

#if defined( _MSC_VER ) && ( _MSC_VER <= 1700 )
	// Visual C++ 2012 and earlier doesn't support variadic templates.
	#define SO_5_NO_VARIADIC_TEMPLATES

	#if _MSC_VER == 1700
		// Please see details at:
		// http://connect.microsoft.com/VisualStudio/feedback/details/761829/c-11-unexpected-behavior-for-std-future-wait-for-and-std-packaged-task
		//
		// Because of it is dangerous to use wait_for() with
		// synchronous service calls.
		#define SO_5_STD_FUTURE_WAIT_FOR_ALWAYS_DEFFERED
	#endif
#endif 

#if defined( _MSC_VER )
	// Visual C++ 2012 and 2013 have very slow std::this_thread::get_id()
	// implementation.
	#define SO_5_MSVC_NEEDS_OWN_CURRENT_THREAD_ID
#endif

#if __cplusplus > 201103L
	#define SO_5_DEPRECATED_ATTR(msg) [[deprecated(msg)]]
#else
	#define SO_5_DEPRECATED_ATTR(msg)
#endif


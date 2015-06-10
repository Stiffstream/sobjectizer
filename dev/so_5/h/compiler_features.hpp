/*
	SObjectizer 5.
*/

/*!
 * \since v.5.3.0
 * \file 
 * \brief Detection of compiler version and absence of various features.
 */

#pragma once

#if defined( _MSC_VER )
	#define SO_5_MSVC

	// Visual C++ 2012 and 2013 have very slow std::this_thread::get_id()
	// implementation.
	#define SO_5_MSVC_NEEDS_OWN_CURRENT_THREAD_ID

	#if _MSC_VER == 1700
		// Please see details at:
		// http://connect.microsoft.com/VisualStudio/feedback/details/761829/c-11-unexpected-behavior-for-std-future-wait-for-and-std-packaged-task
		//
		// Because of it is dangerous to use wait_for() with
		// synchronous service calls.
		#define SO_5_STD_FUTURE_WAIT_FOR_ALWAYS_DEFFERED
	#endif

	// Visual C++ 2013 doesn't support defaults for move-constructors
	// and move-operators.
	#define SO_5_NO_DEFAULTS_FOR_MOVE_CONSTRUCTOR

#endif

#if !(__cplusplus >= 201103L)
	// There is no noexcept keyword
	#define SO_5_NOEXCEPT
#else
	#define SO_5_NOEXCEPT noexcept
	#define SO_5_HAVE_NOEXCEPT 1
#endif


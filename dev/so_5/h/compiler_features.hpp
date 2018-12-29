/*
	SObjectizer 5.
*/

/*!
 * \since
 * v.5.3.0
 *
 * \file 
 * \brief Detection of compiler version and absence of various features.
 */

#pragma once

#if defined( _MSC_VER )
	#define SO_5_MSVC

	// Visual C++ 2012 and 2013 have very slow std::this_thread::get_id()
	// implementation.
	#if _MSC_VER < 1900
		#define SO_5_MSVC_NEEDS_OWN_CURRENT_THREAD_ID

		// Visual C++ 2013 doesn't support methods overloading based on
		// reference or rvalue reference.
		#define SO_5_NO_SUPPORT_FOR_RVALUE_REFERENCE_OVERLOADING

		#if _MSC_VER == 1800
			// Visual C++2013 has problems with inheriting deleted
			// copy constructors and copy operators.
			#define SO_5_MSVC_CANT_INHERIT_DELETED_COPY_CONSTRUCTOR

			// Visual C++2013 can't handle =default for
			// move-constructor.
			#define SO_5_MSVC_CANT_DEFAULT_MOVE_CONSTRUCTOR
		#endif

		// Visual C++ 2012 and 2013 have no constexpr
		#define SO_5_CONSTEXPR

		// Visual C++ 2013 have no normal initializing constructor
		// for std::atomic.
		#define SO_5_STD_ATOMIC_HAS_ONLY_DEFAULT_CTOR

		// Visual C++ 2013 doesn't understand move constructor as a
		// special function and can't defaulted such constructors.
		#define SO_5_NEED_MSVC_12_WORKAROUNDS

		// Visual C++ 2013 doesn't support inline namespace.
		#define SO_5_NO_SUPPORT_FOR_INLINE_NAMESPACE
	#endif

	#if _MSC_VER == 1700
		// Please see details at:
		// http://connect.microsoft.com/VisualStudio/feedback/details/761829/c-11-unexpected-behavior-for-std-future-wait-for-and-std-packaged-task
		//
		// Because of it is dangerous to use wait_for() with
		// synchronous service calls.
		#define SO_5_STD_FUTURE_WAIT_FOR_ALWAYS_DEFFERED
	#endif

#endif

#if defined( __clang__ )
	#define SO_5_CLANG
#endif

#if defined( __GNUC__ )
	#if (4 == __GNUC__) && (8 == __GNUC_MINOR__)
		#define SO_5_NEED_GNU_4_8_WORKAROUNDS
	#endif
#endif

#if (__cplusplus >= 201103L) || (defined(_MSC_VER) && (_MSC_VER >= 1900)) 
	#define SO_5_NOEXCEPT noexcept
	#define SO_5_HAVE_NOEXCEPT 1
#else
	// There is no noexcept keyword
	#define SO_5_NOEXCEPT
#endif

#if defined( SO_5_NO_SUPPORT_FOR_RVALUE_REFERENCE_OVERLOADING )
	#define SO_5_OVERLOAD_FOR_REF
	#define SO_5_OVERLOAD_FOR_RVALUE_REF
#else
	#define SO_5_OVERLOAD_FOR_REF &
	#define SO_5_OVERLOAD_FOR_RVALUE_REF &&
#endif

#if !defined(SO_5_CONSTEXPR)
	#define SO_5_CONSTEXPR constexpr
#endif

// Try to use __has_cpp_attribute if it is supported.
#if defined(__has_cpp_attribute)
	// clang-4 and clang-5 produce warnings when [[nodiscard]]
	// is used with -std=c++11 and -std=c++14.
	#if __has_cpp_attribute(nodiscard) && \
			!(defined(SO_5_CLANG) && __cplusplus < 201703L)
		#define SO_5_NODISCARD [[nodiscard]]
	#endif
#endif

// Handle the result of __has_cpp_attribute.
#if !defined( SO_5_NODISCARD )
	#define SO_5_NODISCARD
#endif

#if defined( SO_5_NO_SUPPORT_FOR_INLINE_NAMESPACE )
	#define SO_5_INLINE_NS
#else
	#define SO_5_INLINE_NS inline
#endif	


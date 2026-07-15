/*
 * SObjectizert-5
 */

/// @file
/// @brief Checking presence of thread sanitizer.
///
/// @since v.5.8.6

#if !defined( SO_5_DETAILS_HAS_THREAD_SANITIZER_HPP )
#define SO_5_DETAILS_HAS_THREAD_SANITIZER_HPP

#if defined( __SANITIZE_THREAD__ )
	#define SO_5_HAS_THREAD_SANITIZER 1
#elif defined( __has_feature )
	#if __has_feature(thread_sanitizer)
		#define SO_5_HAS_THREAD_SANITIZER 1
	#endif
#endif

#if !defined( SO_5_HAS_THREAD_SANITIZER )
	#define SO_5_HAS_THREAD_SANITIZER 0
#endif

#endif


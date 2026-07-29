/*
 * SObjectizer-5
 */

/// @file
/// @brief A special wrapper around std::lock_guard to make TSan happy.
///
/// @since v.5.8.6

#if !defined( SO_5_DETAILS_TSAN_FRIENDLY_LOCK_GUARD_HPP )
#define SO_5_DETAILS_TSAN_FRIENDLY_LOCK_GUARD_HPP

#include <so_5/details/has_thread_sanitizer.hpp>

#include <mutex>

#if defined( SO_5_HAS_THREAD_SANITIZER )
	#include <thread>
#endif

namespace so_5::details
{

#if defined( SO_5_HAS_THREAD_SANITIZER )

/// Workaround for a strange behaviour in thread sanitizer.
///
/// Should be used instead of `std::lock_guard` in places where
/// thread sanitizer report a possible deadlock because of lock
/// order inversion.
///
/// Uses a loop with mutex::try_lock and this_thread::yield instead of
/// plain mutex::lock method. In such a case TSan doesn't report this
/// warning.
///
/// @sa https://github.com/llvm/llvm-project/issues/57955
template< typename M >
class tsan_friendly_lock_guard_t
	{
		M & m_mutex;

	public:
		static
		void
		do_lock( M & mutex )
			{
				// Repeat attempts until we successed.
				while( !mutex.try_lock() )
					{
						std::this_thread::yield();
					}
			}

		explicit tsan_friendly_lock_guard_t( M & mutex )
			: m_mutex{ mutex }
			{
				do_lock( m_mutex );
			}

		~tsan_friendly_lock_guard_t()
			{
				m_mutex.unlock();
			}
	};

#else

/// Actual lock guard for normal cases.
///
/// It's just a thin wrapper around std::lock_guard.
///
/// @since v.5.8.6
template< typename M >
class tsan_friendly_lock_guard_t
	{
		std::lock_guard< M > m_guard;

	public:
		static
		void
		do_lock( M & mutex )
			{
				mutex.lock();
			}

		explicit tsan_friendly_lock_guard_t( M & mutex )
			: m_guard{ mutex }
			{}
	};

#endif

} /* namespace so_5::details */

#endif


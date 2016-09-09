/*
 * SObjectizer-5
 */

/*!
 * \since
 * v.5.4.0
 *
 * \file
 * \brief A workaround for very slow implementation of
 * std::this_thread::get_id() under some compilers.
 */

#include <so_5/h/current_thread_id.hpp>

#if defined( SO_5_MSVC_NEEDS_OWN_CURRENT_THREAD_ID )

// For Visual C++ we need to define our own id facilities.

#include <windows.h>

namespace so_5
{
	SO_5_FUNC current_thread_id_t
	query_current_thread_id()
		{
			return current_thread_id_t(
					static_cast< current_thread_id_t::id_t >(
							GetCurrentThreadId() ) );
		}

} /* namespace so_5 */

#endif


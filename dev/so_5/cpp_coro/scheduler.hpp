/*
 * SObjectizer-5
 */

/// @file
/// @brief Declaration of scheduler interface for C++20 coroutines.
///
/// @since v.5.8.6

#if !defined(SO_5_CPP_CORO_SCHEDULER_HPP)
#define SO_5_CPP_CORO_SCHEDULER_HPP

#include <so_5/declspec.hpp>

#include <coroutine>

namespace so_5::cpp_coro
{

//
// resumable_item_t
//
//FIXME: document this!
struct resumable_item_t
	{
		/// What need to be resumed.
		std::coroutine_handle<> m_to_be_resumed;

		/// The next item in the waiting queue.
		///
		/// @note
		/// This value has to be managed by a scheduler.
		resumable_item_t * m_next{ nullptr };
	};

//
// scheduler_t
//
//FIXME: document this!
class SO_5_TYPE scheduler_t
	{
	public:
		scheduler_t();
		virtual ~scheduler_t();

		//FIXME: document this!
		/// @attention
		/// The @a what_to_resume has to be valid reference and it should
		/// remain valid until the coroutine will be scheduled.
		virtual
		void
		schedule(
			/// What need to be resumed.
			resumable_item_t & what_to_resume ) = 0;
	};

} /* namespace so_5::cpp_coro */

#endif


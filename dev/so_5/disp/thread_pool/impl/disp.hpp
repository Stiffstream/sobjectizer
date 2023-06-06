/*
 * SObjectizer-5
 */

/*!
 * \file
 * \brief An implementation of thread pool dispatcher.
 *
 * \since v.5.4.0
 */

#pragma once

#include <so_5/disp/thread_pool/impl/work_thread_template.hpp>

namespace so_5
{

namespace disp
{

namespace thread_pool
{

namespace impl
{

//
// adaptation_t
//
/*!
 * \brief Adaptation of common implementation of thread-pool-like dispatcher
 * to the specific of this thread-pool dispatcher.
 *
 * \since v.5.5.4
 */
struct adaptation_t
	{
		[[nodiscard]]
		static constexpr std::string_view
		dispatcher_type_name() noexcept
			{
				return { "tp" }; // thread_pool.
			}

		[[nodiscard]]
		static bool
		is_individual_fifo( const bind_params_t & params ) noexcept
			{
				return fifo_t::individual == params.query_fifo();
			}

		static void
		wait_for_queue_emptyness( agent_queue_t & queue ) noexcept
			{
				queue.wait_for_emptyness();
			}
	};

//
// dispatcher_template_t
//
/*!
 * \brief Template for dispatcher.
 *
 * This template depends on work_thread type (with or without activity
 * tracking).
 *
 * \since v.5.5.18
 */
template< typename Work_Thread >
using dispatcher_template_t =
		common_implementation::dispatcher_t<
				Work_Thread,
				dispatcher_queue_t,
				agent_queue_t,
				bind_params_t,
				adaptation_t >;

} /* namespace impl */

} /* namespace thread_pool */

} /* namespace disp */

} /* namespace so_5 */


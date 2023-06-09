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

#include <so_5/disp/thread_pool/impl/basic_event_queue.hpp>
#include <so_5/disp/thread_pool/impl/work_thread_template.hpp>

#include <so_5/disp/thread_pool/pub.hpp>

namespace so_5
{

namespace disp
{

namespace thread_pool
{

namespace impl
{

class agent_queue_t;

//
// dispatcher_queue_t
//
using dispatcher_queue_t = so_5::disp::reuse::queue_of_queues_t< agent_queue_t >;

//
// agent_queue_t
//
/*!
 * \brief Event queue for the agent (or cooperation).
 *
 * \note
 * This class isn't final since v.5.8.0
 *
 * \since v.5.4.0
 */
class agent_queue_t
	:	public basic_event_queue_t
	,	private so_5::atomic_refcounted_t
	{
		friend class so_5::intrusive_ptr_t< agent_queue_t >;

	public :
		//! Constructor.
		agent_queue_t(
			//FIXME: outliving_reference_t has to be used!
			//! Dispatcher queue to work with.
			dispatcher_queue_t & disp_queue,
			//! Parameters for the queue.
			const bind_params_t & params )
			:	basic_event_queue_t{
					params.query_max_demands_at_once()
				}
			,	m_disp_queue{ disp_queue }
			{}

		/*!
		 * \brief Give away a pointer to the next agent_queue.
		 *
		 * \note
		 * This method is a part of interface required by
		 * so_5::disp::reuse::queue_of_queues_t.
		 *
		 * \since v.5.8.0
		 */
		[[nodiscard]]
		agent_queue_t *
		intrusive_queue_giveout_next() noexcept
			{
				auto * r = m_intrusive_queue_next;
				m_intrusive_queue_next = nullptr;
				return r;
			}

		/*!
		 * \brief Set a pointer to the next agent_queue.
		 *
		 * \note
		 * This method is a part of interface required by
		 * so_5::disp::reuse::queue_of_queues_t.
		 *
		 * \since v.5.8.0
		 */
		void
		intrusive_queue_set_next( agent_queue_t * next ) noexcept
			{
				m_intrusive_queue_next = next;
			}

	protected:
		//FIXME: document this
		void
		schedule_on_disp_queue() noexcept override
			{
				m_disp_queue.schedule( this );
			}

	private :
		//FIXME: document this!
		dispatcher_queue_t & m_disp_queue;

		/*!
		 * \brief The next item in intrusive queue of agent_queues.
		 *
		 * This field is necessary to implement interface required by
		 * so_5::disp::reuse::queue_of_queues_t.
		 *
		 * \since v.5.8.0
		 */
		agent_queue_t * m_intrusive_queue_next{ nullptr };
	};

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
				bind_params_t,
				adaptation_t >;

} /* namespace impl */

} /* namespace thread_pool */

} /* namespace disp */

} /* namespace so_5 */


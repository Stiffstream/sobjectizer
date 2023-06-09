/*
 * SObjectizer-5
 */

/*!
 * \file
 * \brief A reusable implementation of work_thread_template that can be used by
 * various thread-pool dispatchers.
 *
 * \since v.5.8.0
 */

#pragma once

#include <so_5/stats/impl/activity_tracking.hpp>

#include <so_5/disp/abstract_work_thread.hpp>

#include <so_5/disp/thread_pool/impl/common_implementation.hpp>

#include <so_5/impl/thread_join_stuff.hpp>

namespace so_5
{

namespace disp
{

namespace thread_pool
{

namespace impl
{

namespace work_thread_details
{

/*!
 * \brief Part of %common_data_t that depends on a template parameter.
 *
 * Intended to be used as a mixin for common_data_t type.
 */
template< typename Disp_Queue >
struct common_data_template_dependent_t
	{
		//! Dispatcher's queue.
		Disp_Queue * m_disp_queue;

		common_data_template_dependent_t(
			outliving_reference_t< Disp_Queue > disp_queue )
			:	m_disp_queue{ std::addressof(disp_queue.get()) }
			{}
	};

/*!
 * \brief Part of %common_data_t that doesn't depend on a template parameter.
 *
 * Intended to be used as a mixin for common_data_t type.
 */
struct common_data_template_independent_t
	{
		//! ID of thread.
		/*!
		 * Receives actual value inside body().
		 */
		so_5::current_thread_id_t m_thread_id;

		//! Actual thread.
		work_thread_holder_t m_thread_holder;

		//! Waiting object for long wait.
		so_5::disp::mpmc_queue_traits::condition_unique_ptr_t m_condition;

		common_data_template_independent_t(
			work_thread_holder_t thread_holder,
			so_5::disp::mpmc_queue_traits::condition_unique_ptr_t condition )
			:	m_thread_holder{ std::move(thread_holder) }
			,	m_condition{ std::move(condition) }
			{}
	};

/*!
 * \brief Main data for work_thread.
 *
 * \since v.5.5.18
 */
template< typename Disp_Queue >
struct common_data_t
	:	public common_data_template_dependent_t< Disp_Queue >
	,	public common_data_template_independent_t
	{
		common_data_t(
			outliving_reference_t< Disp_Queue > queue,
			work_thread_holder_t thread_holder )
			:	common_data_template_dependent_t< Disp_Queue >{ queue }
			,	common_data_template_independent_t{
					std::move(thread_holder),
					queue.get().allocate_condition()
				}
			{}
	};

/*!
 * \brief Part of implementation of work thread without activity tracing.
 *
 * \since v.5.5.18
 */
template< typename Disp_Queue >
class no_activity_tracking_impl_t : protected common_data_t< Disp_Queue >
	{
	public :
		/*!
		 * \brief Mandatory alias for Disp_Queue.
		 *
		 * This alias is then be used by the work_thread_template_t template.
		 */
		using disp_queue_t = Disp_Queue;

		//! Initializing constructor.
		no_activity_tracking_impl_t(
			outliving_reference_t< Disp_Queue > queue,
			work_thread_holder_t thread_holder )
			:	common_data_t< Disp_Queue >( queue, std::move(thread_holder) )
			{}

		template< typename L >
		void
		take_activity_stats( L ) { /* Nothing to do */ }

	protected :
		void
		work_started() {}

		void
		work_finished() {}

		void
		wait_started() {}

		void
		wait_finished() {}
	};

/*!
 * \brief Part of implementation of work thread with activity tracing.
 *
 * \since v.5.5.18
 */
template< typename Disp_Queue >
class with_activity_tracking_impl_t : protected common_data_t< Disp_Queue >
	{
		using activity_tracking_traits = so_5::stats::activity_tracking_stuff::traits;

	public :
		/*!
		 * \brief Mandatory alias for Disp_Queue.
		 *
		 * This alias is then be used by the work_thread_template_t template.
		 */
		using disp_queue_t = Disp_Queue;

		//! Initializing constructor.
		with_activity_tracking_impl_t(
			outliving_reference_t< Disp_Queue > queue,
			work_thread_holder_t thread_holder )
			:	common_data_t< Disp_Queue >( queue, std::move(thread_holder) )
			{}

		template< typename L >
		void
		take_activity_stats( L lambda )
			{
				so_5::stats::work_thread_activity_stats_t result;

				result.m_working_stats = m_work_activity_collector.take_stats();
				result.m_waiting_stats = m_waiting_stats_collector.take_stats();

				lambda( result );
			}

	protected :
		//! Lock for activity statistics.
		activity_tracking_traits::lock_t m_stats_lock;

		//! A collector for work activity.
		so_5::stats::activity_tracking_stuff::stats_collector_t<
					so_5::stats::activity_tracking_stuff::external_lock<> >
				m_work_activity_collector{ m_stats_lock };

		//! A collector for waiting stats.
		so_5::stats::activity_tracking_stuff::stats_collector_t<
					so_5::stats::activity_tracking_stuff::external_lock<> >
				m_waiting_stats_collector{ m_stats_lock };

		void
		work_started()
			{
				m_work_activity_collector.start();
			}

		void
		work_finished()
			{
				m_work_activity_collector.stop();
			}

		void
		wait_started()
			{
				m_waiting_stats_collector.start();
			}

		void
		wait_finished()
			{
				m_waiting_stats_collector.stop();
			}
	};

//
// work_thread_template_t
//
/*!
 * \brief Implementation of work_thread in form of template class.
 *
 * \since v.5.5.18
 */
template< typename Impl >
class work_thread_template_t final : public Impl
	{
	public :
		using disp_queue_t = typename Impl::disp_queue_t;

	private:
		using agent_queue_t = typename disp_queue_t::item_t;

	public:
		//! Initializing constructor.
		work_thread_template_t(
			outliving_reference_t< disp_queue_t > queue,
			work_thread_holder_t thread_holder )
			:	Impl( queue, std::move(thread_holder) )
			{}

		void
		join()
			{
				so_5::impl::ensure_join_from_different_thread( this->m_thread_id );
				this->m_thread_holder.unchecked_get().join();
			}

		//! Launch work thread.
		void
		start()
			{
				this->m_thread_holder.unchecked_get().start( [this]() { body(); } );
			}

		/*!
		 * \brief Get ID of work thread.
		 *
		 * \note This method returns correct value only after start
		 * of the thread.
		 *
		 * \since v.5.5.18
		 */
		so_5::current_thread_id_t
		thread_id() const
			{
				return this->m_thread_id;
			}

	private :
		//! Thread body method.
		void
		body()
			{
				this->m_thread_id = so_5::query_current_thread_id();

				agent_queue_t * agent_queue;
				while( nullptr != (agent_queue = this->pop_agent_queue()) )
					{
						this->do_queue_processing( agent_queue );
					}
			}

		/*!
		 * \brief An attempt of extraction of non-empty agent queue.
		 *
		 * \note This is noexcept method because its logic can't survive
		 * an exception from m_disp_queue->pop.
		 *
		 * \since v.5.5.18
		 */
		[[nodiscard]]
		agent_queue_t *
		pop_agent_queue() noexcept
			{
				agent_queue_t * result = nullptr;

				this->wait_started();

				result = this->m_disp_queue->pop( *(this->m_condition) );

				this->wait_finished();

				return result;
			}

		/*!
		 * \brief Starts processing of demands from the queue specified.
		 *
		 * Starts from \a current_queue. Processes up to enabled number
		 * of events from that queue. Then if the queue is not empty
		 * tries to find another non-empty queue. If there is no such queue
		 * then continue processing of \a current_queue.
		 *
		 * \since v.5.5.15.1
		 */
		void
		do_queue_processing( agent_queue_t * current_queue )
			{
				do
					{
						const auto e = this->process_queue( *current_queue );

						if( agent_queue_t::emptyness_t::not_empty == e )
							{
								// We can continue processing of that queue if
								// there is no more non-empty queues waiting.
								current_queue =
									this->m_disp_queue->try_switch_to_another(
										current_queue );
							}
						else
							// Handling of the current queue should be stopped.
							current_queue = nullptr;
					}
				while( current_queue != nullptr );
			}


		//! Processing of demands from agent queue.
		[[nodiscard]]
		typename agent_queue_t::emptyness_t
		process_queue( agent_queue_t & queue )
			{
				std::size_t demands_processed = 0;
				typename agent_queue_t::pop_result_t pop_result;

				do
					{
						auto & d = queue.front();

						this->work_started();

						d.call_handler( this->m_thread_id );

						this->work_finished();

						++demands_processed;
						pop_result = queue.pop( demands_processed );
					}
				while( agent_queue_t::processing_continuation_t::enabled ==
						pop_result.m_continuation );

				return pop_result.m_emptyness;
			}
	};

} /* namespace work_thread_details */

//
// work_thread_no_activity_tracking_t
//
/*!
 * \brief Type of work thread without activity tracking.
 *
 * \since v.5.5.18
 */
template< typename Disp_Queue >
using work_thread_no_activity_tracking_t =
		work_thread_details::work_thread_template_t<
				work_thread_details::no_activity_tracking_impl_t< Disp_Queue > >;

//
// work_thread_with_activity_tracking_t
//
/*!
 * \brief Type of work thread without activity tracking.
 *
 * \since v.5.5.18
 */
template< typename Disp_Queue >
using work_thread_with_activity_tracking_t =
		work_thread_details::work_thread_template_t<
				work_thread_details::with_activity_tracking_impl_t< Disp_Queue > >;

} /* namespace impl */

} /* namespace thread_pool */

} /* namespace disp */

} /* namespace so_5 */


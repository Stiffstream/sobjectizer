/*
 * SObjectizer-5
 */

/*!
 * \since v.5.5.4
 * \file
 * \brief Reusable tools for run-time monitoring of thread-pool-like dispatchers.
 */

#pragma once

#include <so_5/h/atomic_refcounted.hpp>

#include <so_5/rt/h/send_functions.hpp>

#include <so_5/rt/stats/h/repository.hpp>
#include <so_5/rt/stats/h/messages.hpp>
#include <so_5/rt/stats/h/std_names.hpp>

#include <so_5/disp/reuse/h/data_source_prefix_helpers.hpp>

namespace so_5 {

namespace disp {

namespace reuse {

namespace thread_pool_stats {

namespace stats = so_5::stats;

/*!
 * \since v.5.5.4
 * \brief Description of one event queue.
 */
struct queue_description_t
	{
		//! Prefix for data-sources related to that queue.
		stats::prefix_t m_prefix;

		//! Count of agents bound to that queue.
		std::size_t m_agent_count;

		//! Current queue size.
		std::size_t m_queue_size;
	};

/*!
 * \since v.5.5.4
 * \brief A holder of one event queue information block.
 *
 * This holder must be allocated as dynamic object.
 *
 * There may be two references to it:
 * - one (main reference) is in coresponding event queue object inside
 *   dispatcher. This reference will be destroyed with the event queue.
 *   It means that while the event queue exists there will be at least
 *   one reference to the holder;
 * - second (temporary reference) is created only for data source update
 *   operation. This reference ensures that holder will not be destroyed
 *   while data source update operation is in progress (but the event queue
 *   could be destroyed).
 *
 * The most important thing about the holder is: actual value for
 * m_next atribute must be set only during data source update operation,
 * and must be dropped to nullptr after it.
 */
struct queue_description_holder_t : private atomic_refcounted_t
	{
		friend class intrusive_ptr_t< queue_description_holder_t >;

		//! Actual description for the event queue.
		queue_description_t m_desc;

		//! Next item in the chain of queues descriptions.
		/*!
		 * Receives actual value only for small amount of time during
		 * data source update operation.
		 */
		intrusive_ptr_t< queue_description_holder_t > m_next;
	};

/*!
 * \since v.5.5.4
 * \brief Typedef for smart pointer to queue_description.
 */
using queue_description_holder_ref_t =
		intrusive_ptr_t< queue_description_holder_t >;

/*!
 * \since v.5.5.4
 * \brief Helper function for creating queue_description_holder object.
 */
inline queue_description_holder_ref_t
make_queue_desc_holder(
	const stats::prefix_t & prefix,
	const std::string & coop_name,
	std::size_t agent_count )
	{
		queue_description_holder_ref_t result( new queue_description_holder_t() );

		std::ostringstream ss;
		ss << prefix.c_str() << "/cq/"
				<< so_5::disp::reuse::ios_helpers::length_limited_string{
						coop_name, 16 };

		result->m_desc.m_prefix = stats::prefix_t{ ss.str() };
		result->m_desc.m_agent_count = agent_count;
		result->m_desc.m_queue_size = 0;

		return result;
	}

/*!
 * \since v.5.5.4
 * \brief Helper function for creating queue_description_holder object.
 *
 * Must be used for the case when agent uses individual FIFO.
 */
inline queue_description_holder_ref_t
make_queue_desc_holder(
	const stats::prefix_t & prefix,
	const void * agent )
	{
		queue_description_holder_ref_t result( new queue_description_holder_t() );

		std::ostringstream ss;
		ss << prefix.c_str() << "/aq/"
				<< so_5::disp::reuse::ios_helpers::pointer{ agent };

		result->m_desc.m_prefix = stats::prefix_t{ ss.str() };
		result->m_desc.m_agent_count = 1;
		result->m_desc.m_queue_size = 0;

		return result;
	}

/*!
 * \since v.5.5.4
 * \brief An interface of collector of information about thread-pool-like
 * dispatcher state.
 */
class stats_consumer_t
	{
	protected :
		~stats_consumer_t()
			{}

	public :
		//! Informs consumer about actual actual thread count.
		virtual void
		set_thread_count( std::size_t value ) = 0;

		//! Informs counsumer about yet another event queue.
		virtual void
		add_queue(
				const intrusive_ptr_t< queue_description_holder_t > & queue_desc ) = 0;
	};

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnon-virtual-dtor"
#endif

/*!
 * \since v.5.5.4
 * \brief An interface of supplier of information about thread-pool-like
 * dispatcher state.
 *
 * It is intended to be used as mixin for a dispatcher class.
 */
class stats_supplier_t
	{
	public :
		virtual void
		supply( stats_consumer_t & consumer ) = 0;
	};

/*!
 * \since v.5.5.4
 * \brief Type of data source for thread-pool-like dispatchers.
 */
class data_source_t : public stats::manually_registered_source_t
	{
	public :
		//! Initializing constructor.
		data_source_t(
			//! Supplier of statistical information.
			stats_supplier_t & supplier )
			:	m_supplier( supplier )
			{}

		//! Setting of data-source basic name.
		void
		set_data_sources_name_base(
			//! Type of the dispatcher.
			//! Like 'tp' for thread-pool-dispatcher or
			//! 'atp' for adv-thread-pool-dispatcher.
			const char * disp_type,
			//! Optional name to be used as part of data-source prefix.
			const std::string & name_basic,
			//! Pointer to the dispatcher object.
			//! Will be used if \a name_basic is empty.
			const void * disp_pointer )
			{
				m_prefix = make_disp_prefix( disp_type, name_basic, disp_pointer ); 
			}

		//! Distribution of statistical information.
		virtual void
		distribute(
			const mbox_t & mbox ) override
			{
				// Collecting...
				collector_t collector;
				m_supplier.supply( collector );

				// Distributing...
				so_5::send< stats::messages::quantity< std::size_t > >(
						mbox,
						m_prefix,
						stats::suffixes::disp_thread_count(),
						collector.thread_count() );

				so_5::send< stats::messages::quantity< std::size_t > >(
						mbox,
						m_prefix,
						stats::suffixes::agent_count(),
						collector.agent_count() );

				collector.for_each_queue(
					[this, &mbox]( const queue_description_t & queue ) {
						so_5::send< stats::messages::quantity< std::size_t > >(
								mbox,
								queue.m_prefix,
								stats::suffixes::agent_count(),
								queue.m_agent_count );

						so_5::send< stats::messages::quantity< std::size_t > >(
								mbox,
								queue.m_prefix,
								stats::suffixes::work_thread_queue_size(),
								queue.m_queue_size );
					} );
			}

		//! Basic prefix for data source names.
		const stats::prefix_t &
		prefix() const
			{
				return m_prefix;
			}

	private :
		//! Statistical information supplier.
		stats_supplier_t & m_supplier;

		//! Prefix for data-source names.
		stats::prefix_t m_prefix;

		//! Actual type of statical information collector.
		class collector_t : public stats_consumer_t
			{
			public :
				~collector_t()
					{
						// Chain of queue data sources must be cleaned up.
						auto holder = m_queue_desc_head;
						m_queue_desc_head.reset();

						while( holder )
							{
								auto current = holder;
								holder = holder->m_next;
								current->m_next.reset();
							}

						m_queue_desc_tail.reset();
					}

				virtual void
				set_thread_count(
					std::size_t thread_count ) override
					{
						m_thread_count = thread_count;
					}

				virtual void
				add_queue(
					const intrusive_ptr_t< queue_description_holder_t > & info ) override
					{
						m_agent_count += info->m_desc.m_agent_count;

						if( m_queue_desc_tail )
							{
								m_queue_desc_tail->m_next = info;
								m_queue_desc_tail = info;
							}
						else
							{
								m_queue_desc_head = info;
								m_queue_desc_tail = info;
							}
					}

				std::size_t
				thread_count() const
					{
						return m_thread_count;
					}

				std::size_t
				agent_count() const
					{
						return m_agent_count;
					}

				template< typename LAMBDA >
				void
				for_each_queue( LAMBDA lambda ) const
					{
						const queue_description_holder_t * p =
								m_queue_desc_head.get();

						while( p )
							{
								lambda( p->m_desc );
								p = p->m_next.get();
							}
					}

			private :
				std::size_t m_thread_count = { 0 };
				std::size_t m_agent_count = { 0 };

				intrusive_ptr_t< queue_description_holder_t > m_queue_desc_head;
				intrusive_ptr_t< queue_description_holder_t > m_queue_desc_tail;
			};

	};

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

} /* namespace thread_pool_stats */

} /* namespace reuse */

} /* namespace disp */

} /* namespace so_5 */


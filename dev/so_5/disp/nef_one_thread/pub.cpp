/*
	SObjectizer 5.
*/

/*!
 * \file
 * \brief Functions for creating and binding of the single thread dispatcher
 * that provides noexcept guarantee for scheduling evt_finish demand.
 *
 * \since v.5.5.8
 */

#include <so_5/disp/nef_one_thread/pub.hpp>

#include <so_5/disp/reuse/actual_work_thread_factory_to_use.hpp>
#include <so_5/disp/reuse/data_source_prefix_helpers.hpp>
#include <so_5/disp/reuse/make_actual_dispatcher.hpp>

#include <so_5/impl/thread_join_stuff.hpp>

#include <so_5/stats/repository.hpp>
#include <so_5/stats/messages.hpp>
#include <so_5/stats/std_names.hpp>

#include <so_5/send_functions.hpp>

namespace so_5 {

namespace disp {

namespace nef_one_thread {

namespace impl {

namespace queue_traits = so_5::disp::mpsc_queue_traits;

namespace stats = so_5::stats;

//
// demand_t
//
/*!
 * \brief A single execution demand.
 *
 * \since v.5.8.0
 */
struct demand_t
	{
		//! Execution demand to be used.
		/*!
		 * \note
		 * It may be empty (if the default constructor was used for
		 * preallocated evt_start/evt_finish demands).
		 */
		execution_demand_t m_execution_demand;

		//! Next demand in the queue.
		/*!
		 * \note
		 * It's a dynamically allocated object that has to be deallocated
		 * manually during the destruction of the queue.
		 */
		demand_t * m_next = nullptr;

		/*!
		 * \brief Default constructor.
		 *
		 * It's necessary for preallocation of evt_start and evt_finish
		 * demands.
		 */
		demand_t() = default;

		//! Initializing constructor.
		demand_t( execution_demand_t && source )
			:	m_execution_demand{ std::move( source ) }
			{}
	};

//
// demand_unique_ptr_t
//
/*!
 * \brief An alias for unique_ptr to demand.
 *
 * \since v.5.8.0
 */
using demand_unique_ptr_t = std::unique_ptr< demand_t >;

//
// demand_queue_t
//
class demand_queue_t
	{
		//! Queue lock.
		queue_traits::lock_unique_ptr_t m_lock;

		//! Shutdown flag.
		bool m_shutdown = false;

		//! Head of the queue.
		/*! Null if queue is empty. */
		demand_t * m_head = nullptr;
		//! Tail of the queue.
		/*! Null if queue is empty. */
		demand_t * m_tail = nullptr;

		//! Current size of the queue.
		std::atomic< std::size_t > m_size = { 0 };

	public:
		//! Initializing constructor.
		demand_queue_t(
			//! Lock to be used for queue protection.
			queue_traits::lock_unique_ptr_t lock )
			:	m_lock{ std::move(lock) }
			{}

		~demand_queue_t() noexcept
			{
				while( m_head )
					{
						remove_head();
					}
			}

		//! Set the shutdown signal.
		void
		stop()
			{
				queue_traits::lock_guard_t lock{ *m_lock };

				m_shutdown = true;

				// If the queue is empty then someone can wait a notification.
				if( !m_head )
					{
						lock.notify_one();
					}
			}

		void
		push( demand_unique_ptr_t tail_demand )
			{
				queue_traits::lock_guard_t lock{ *m_lock };

				++m_size;

				if( nullptr == m_head )
					{
						m_head = tail_demand.release();
						m_tail = m_head;

						// Someone might wait for the first demand.
						lock.notify_one();
					}
				else
					{
						m_tail->m_next = tail_demand.release();
						m_tail = m_tail->m_next;
					}
			}

		// Returns nullptr if shutdown flag is set.
		demand_unique_ptr_t
		pop()
			{
				demand_unique_ptr_t result;

				queue_traits::unique_lock_t lock{ *m_lock };

				while( !m_shutdown && (nullptr == m_head) )
					{
						lock.wait_for_notify();
					}

				if( !m_shutdown )
					{
						// Assume that m_head != nullptr.
						result = remove_head();
					}

				return result;
			}

		//! Get the current size of the queue.
		std::size_t
		size() const
			{
				return m_size.load( std::memory_order_acquire );
			}

	private:
		//! Helper method for deleting queue's head object.
		demand_unique_ptr_t
		remove_head() noexcept
			{
				demand_unique_ptr_t to_be_deleted{ m_head };
				m_head = m_head->m_next;
				m_head->m_next = nullptr;

				--m_size;

				return to_be_deleted;
			}
	};

//
// agent_queue_t
//
/*!
 *
 * \note
 * This class is Moveable, but not Copyable.
 */
class agent_queue_t final : public event_queue_t
	{
		std::reference_wrapper< demand_queue_t > m_dest_queue;

		demand_unique_ptr_t m_evt_start_demand;
		demand_unique_ptr_t m_evt_finish_demand;

	public:
		agent_queue_t(
			demand_queue_t & dest_queue,
			demand_unique_ptr_t evt_start_demand,
			demand_unique_ptr_t evt_finish_demand )
			:	m_dest_queue{ dest_queue }
			,	m_evt_start_demand{ std::move(evt_start_demand) }
			,	m_evt_finish_demand{ std::move(evt_finish_demand) }
			{}

		agent_queue_t( const agent_queue_t & ) = delete;
		agent_queue_t &
		operator=( const agent_queue_t & ) = delete;

		agent_queue_t( agent_queue_t && ) = default;
		agent_queue_t &
		operator=( agent_queue_t && ) = default;

		void
		push( execution_demand_t demand ) override
			{
				m_dest_queue.get().push(
						std::make_unique< demand_t >( std::move(demand) ) );
			}

		void
		push_evt_start( execution_demand_t demand ) override
			{
				// ATTENTION: assume that m_evt_start_demand is valid.
				// It's UB is that isn't the true.
				m_evt_start_demand->m_execution_demand = std::move(demand);
				m_dest_queue.get().push( std::move(m_evt_start_demand) );
			}

		void
		push_evt_finish( execution_demand_t demand ) noexcept override
			{
				// ATTENTION: assume that m_evt_start_demand is valid.
				// It's UB is that isn't the true.
				//
				// Don't expect exceptions here.
				m_evt_finish_demand->m_execution_demand = std::move(demand);
				m_dest_queue.get().push( std::move(m_evt_finish_demand) );
			}
	};

namespace work_thread_details {

//
// common_data_t
//
/*!
 * \brief A common data for all work thread implementations.
 *
 * \since v.5.8.0
 */
struct common_data_t
	{
		//! Demands queue to work for.
		demand_queue_t m_queue;

		//! Thread object.
		work_thread_holder_t m_thread_holder;

		//! ID of the work thread.
		/*!
		 * \note Receives actual value only after successful start
		 * of the thread.
		 */
		so_5::current_thread_id_t m_thread_id;

		//! Initializing constructor.
		common_data_t(
			//! Lock to be used for queue protection.
			queue_traits::lock_unique_ptr_t lock,
			//! Worker thread to be used.
			work_thread_holder_t thread_holder )
			:	m_queue{ std::move(lock) }
			,	m_thread_holder{ std::move(thread_holder) }
			{}
	};

//
// no_activity_tracking_impl_t
//
/*!
 * \brief A part of implementation of work thread without activity tracking.
 *
 * \since v.5.8.0
 */
class no_activity_tracking_impl_t : protected common_data_t
	{
	public :
		no_activity_tracking_impl_t(
			//! Lock to be used for queue protection.
			queue_traits::lock_unique_ptr_t lock,
			//! Worker thread to be used.
			work_thread_holder_t thread_holder )
			:	common_data_t{
					std::move(lock),
					std::move(thread_holder)
				}
			{}

	protected :
		void
		work_started() { /* Nothing to do. */ }

		void
		work_finished() { /* Nothing to do. */ }

		void
		wait_started() { /* Nothing to do. */ }

		void
		wait_finished() { /* Nothing to do. */ }
	};

//
// with_activity_tracking_impl_t
//
/*!
 * \brief A part of implementation of work thread with activity tracking.
 *
 * \since v.5.5.18
 */
class with_activity_tracking_impl_t : protected common_data_t
	{
	public :
		with_activity_tracking_impl_t(
			//! Lock to be used for queue protection.
			queue_traits::lock_unique_ptr_t lock,
			//! Worker thread to be used.
			work_thread_holder_t thread_holder )
			:	common_data_t{
					std::move(lock),
					std::move(thread_holder)
				}
			{}

		so_5::stats::work_thread_activity_stats_t
		take_activity_stats()
			{
				so_5::stats::work_thread_activity_stats_t result;

				result.m_working_stats = m_working_stats.take_stats();
				result.m_waiting_stats = m_waiting_stats.take_stats();

				return result;
			}

	protected :
		//! Statictics for work activity.
		so_5::stats::activity_tracking_stuff::stats_collector_t<
				so_5::stats::activity_tracking_stuff::internal_lock >
			m_working_stats;

		//! Statictics for wait activity.
		so_5::stats::activity_tracking_stuff::stats_collector_t<
				so_5::stats::activity_tracking_stuff::internal_lock >
			m_waiting_stats;

		void
		work_started() { m_working_stats.start(); }

		void
		work_finished() { m_working_stats.stop(); }

		void
		wait_started() { m_waiting_stats.start(); }

		void
		wait_finished() { m_waiting_stats.stop(); }
	};

} /* namespace work_thread_details */

//
// work_thread_template_t
//
/*!
 * \brief A worker thread for nef_one_thread dispatcher.
 *
 * \since v.5.8.0
 */
template< typename Work_Thread >
class work_thread_template_t : public Work_Thread
	{
		using base_type_t = Work_Thread;

	public :
		//! Initializing constructor.
		work_thread_template_t(
			//! Lock to be used for queue protection.
			queue_traits::lock_unique_ptr_t lock,
			//! Worker thread to be used.
			work_thread_holder_t thread_holder )
			:	base_type_t{
					std::move(lock),
					std::move(thread_holder)
				}
			{}

		void
		start()
			{
				this->m_thread_holder.unchecked_get().start( [this]() { body(); } );
			}

		void
		stop()
			{
				this->m_queue.stop();
			}

		void
		join()
			{
				so_5::impl::ensure_join_from_different_thread( this->m_thread_id );
				this->m_thread_holder.unchecked_get().join();
			}

		[[nodiscard]]
		so_5::current_thread_id_t
		thread_id() const
			{
				return this->m_thread_id;
			}

		[[nodiscard]]
		demand_queue_t &
		demand_queue() noexcept
			{
				return this->m_queue;
			}

	private :
		void
		body()
			{
				this->m_thread_id = so_5::query_current_thread_id();

				bool demand_extracted = true;
				do
					{
						auto d = this->pop_demand();
						if( d )
							{
								this->call_handler( d->m_execution_demand );
							}
						else
							// No more demands, it's time to break the loop.
							demand_extracted = false;
					}
				while( demand_extracted );
			}

		[[nodiscard]]
		demand_unique_ptr_t
		pop_demand()
			{
				this->wait_started();
				auto wait_meter_stopper = so_5::details::at_scope_exit(
						[this] { this->wait_finished(); } );

				return this->m_queue.pop();
			}

		void
		call_handler( so_5::execution_demand_t & demand )
			{
				this->work_started();
				auto work_meter_stopper = so_5::details::at_scope_exit(
						[this] { this->work_finished(); } );

				demand.call_handler( this->m_thread_id );
			}
	};

//
// work_thread_no_activity_tracking_t
//
using work_thread_no_activity_tracking_t =
	work_thread_template_t< work_thread_details::no_activity_tracking_impl_t >;

//
// work_thread_with_activity_tracking_t
//
using work_thread_with_activity_tracking_t =
	work_thread_template_t< work_thread_details::with_activity_tracking_impl_t >;

//
// send_thread_activity_stats
//
void
send_thread_activity_stats(
	const so_5::mbox_t &,
	const stats::prefix_t &,
	work_thread_no_activity_tracking_t & )
	{
		/* Nothing to do */
	}

void
send_thread_activity_stats(
	const so_5::mbox_t & mbox,
	const stats::prefix_t & prefix,
	work_thread_with_activity_tracking_t & wt )
	{
		so_5::send< stats::messages::work_thread_activity >(
				mbox,
				prefix,
				stats::suffixes::work_thread_activity(),
				wt.thread_id(),
				wt.take_activity_stats() );
	}

//
// dispatcher_template_t
//
/*!
 * \brief An implementation of dispatcher with one working
 * thread and guarantee that evt_finish demands will be added to
 * the queue without exceptions.
 *
 * \since v.5.8.0
 */
template< typename Work_Thread >
class dispatcher_template_t final : public disp_binder_t
	{
		friend class disp_data_source_t;

	public:
		dispatcher_template_t(
			outliving_reference_t< environment_t > env,
			const std::string_view name_base,
			disp_params_t params )
			:	m_work_thread{
					params.queue_params().lock_factory()(),
					so_5::disp::reuse::acquire_work_thread( params, env.get() ),
				}
			,	m_data_source{
					outliving_mutable(env.get().stats_repository()),
					name_base,
					outliving_mutable(*this)
				}
			{
				m_work_thread.start();
			}

		~dispatcher_template_t() noexcept override
			{
				m_work_thread.stop();
				m_work_thread.join();
			}

		void
		preallocate_resources(
			agent_t & agent ) override
			{
				// Assume that there is no pointer to the agent in the map yet.
				auto evt_start_demand = std::make_unique< demand_t >();
				auto evt_finish_demand = std::make_unique< demand_t >();

				// All further operattions have to be performed under the lock.
				std::lock_guard< std::mutex > lock{ m_agent_map_lock };

				m_agents.emplace(
						std::addressof(agent),
						agent_queue_t{
								m_work_thread.demand_queue(),
								std::move(evt_start_demand),
								std::move(evt_finish_demand)
							} );
			}

		void
		undo_preallocation(
			agent_t & agent ) noexcept override
			{
				std::lock_guard< std::mutex > lock{ m_agent_map_lock };

				m_agents.erase( std::addressof(agent) );
			}

		void
		bind(
			agent_t & agent ) noexcept override
			{
				event_queue_t & queue = [&]() -> event_queue_t &
					{
						std::lock_guard< std::mutex > lock{ m_agent_map_lock };

						auto it = m_agents.find( std::addressof(agent) );
						// Just in case, to simplify debugging if something
						// went very, very wrong.
						// This will lead to the termination of the application,
						// but it is better than accessing a random pointer.
						if( it == m_agents.end() )
							SO_5_THROW_EXCEPTION(
									rc_no_preallocated_resources_for_agent,
									"nef_one_thread dispatcher has no info about an agent "
									"in bind() method" );

						return it->second;
					}();

				agent.so_bind_to_dispatcher( queue );
			}

		void
		unbind(
			agent_t & agent ) noexcept override
			{
				// Just reuse existing implementation.
				this->undo_preallocation( agent );
			}

	private:
		/*!
		 * \brief Data source for run-time monitoring of whole dispatcher.
		 *
		 * \since v.5.8.0
		 */
		class disp_data_source_t : public stats::source_t
			{
				//! Dispatcher to work with.
				outliving_reference_t< dispatcher_template_t > m_dispatcher;

				//! Basic prefix for data sources.
				stats::prefix_t m_base_prefix;

			public :
				disp_data_source_t(
					const std::string_view name_base,
					outliving_reference_t< dispatcher_template_t > disp )
					:	m_dispatcher{ disp }
					,	m_base_prefix{ so_5::disp::reuse::make_disp_prefix(
								"nef-ot",
								name_base,
								&(disp.get()) )
						}
					{}

				void
				distribute( const mbox_t & mbox ) override
					{
						auto & disp = m_dispatcher.get();

						const std::size_t agents_count = [&disp]() {
								std::lock_guard< std::mutex > lock{ disp.m_agent_map_lock };
								return disp.m_agents.size();
							}();

						so_5::send< stats::messages::quantity< std::size_t > >(
								mbox,
								m_base_prefix,
								stats::suffixes::agent_count(),
								agents_count );

						//FIXME: send demands_count value!

						send_thread_activity_stats(
								mbox,
								m_base_prefix,
								disp.m_work_thread );
					}
			};

		//! Type of map from agent pointer to an individual event_queue.
		using agent_map_t = std::map< agent_t *, agent_queue_t >;

		//! Worker thread for the dispatcher.
		Work_Thread m_work_thread;

		//! Data source for run-time monitoring.
		stats::auto_registered_source_holder_t< disp_data_source_t >
				m_data_source;

		//! Lock for agent_map protection.
		std::mutex m_agent_map_lock;

		//! Agents for those resources are allocated by the dispatcher.
		agent_map_t m_agents;
	};

//
// dispatcher_handle_maker_t
//
class dispatcher_handle_maker_t
	{
	public :
		static dispatcher_handle_t
		make( disp_binder_shptr_t binder ) noexcept
			{
				return { std::move( binder ) };
			}
	};

} /* namespace impl */

//
// make_dispatcher
//
SO_5_FUNC dispatcher_handle_t
make_dispatcher(
	environment_t & env,
	const std::string_view data_sources_name_base,
	disp_params_t params )
	{
		using namespace so_5::disp::reuse;

		using dispatcher_no_activity_tracking_t =
				impl::dispatcher_template_t<
						impl::work_thread_no_activity_tracking_t >;

		using dispatcher_with_activity_tracking_t =
				impl::dispatcher_template_t<
						impl::work_thread_with_activity_tracking_t >;

		disp_binder_shptr_t binder = so_5::disp::reuse::make_actual_dispatcher<
						disp_binder_t,
						dispatcher_no_activity_tracking_t,
						dispatcher_with_activity_tracking_t >(
				outliving_mutable(env),
				data_sources_name_base,
				std::move(params) );

		return impl::dispatcher_handle_maker_t::make( std::move(binder) );
	}

} /* namespace nef_one_thread */

} /* namespace disp */

} /* namespace so_5 */



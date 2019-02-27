/*
	SObjectizer 5
*/

#include <so_5/disp/active_obj/pub.hpp>

#include <so_5/event_queue.hpp>
#include <so_5/send_functions.hpp>

#include <so_5/details/rollback_on_exception.hpp>

#include <so_5/disp/reuse/data_source_prefix_helpers.hpp>
#include <so_5/disp/reuse/make_actual_dispatcher.hpp>

#include <so_5/disp/reuse/work_thread/work_thread.hpp>

#include <so_5/stats/repository.hpp>
#include <so_5/stats/messages.hpp>
#include <so_5/stats/std_names.hpp>

#include <map>
#include <mutex>
#include <algorithm>

namespace so_5
{

namespace disp
{

namespace active_obj
{

namespace impl
{

namespace work_thread = so_5::disp::reuse::work_thread;
namespace stats = so_5::stats;

namespace
{

/*!
 * \brief Just a helper function for consequetive call to shutdown and wait.
 * \since
 * v.5.5.4
 */
template< class T >
void
shutdown_and_wait( T & w )
	{
		w.shutdown();
		w.wait();
	}

template< class Work_Thread >
void
send_demands_count_stats(
	const so_5::mbox_t & mbox,
	const stats::prefix_t & prefix,
	Work_Thread & wt )
	{
		so_5::send< stats::messages::quantity< std::size_t > >(
				mbox,
				prefix,
				stats::suffixes::work_thread_queue_size(),
				wt.demands_count() );
	}

void
send_thread_activity_stats(
	const so_5::mbox_t &,
	const stats::prefix_t &,
	work_thread::work_thread_no_activity_tracking_t & )
	{
		/* Nothing to do */
	}

void
send_thread_activity_stats(
	const so_5::mbox_t & mbox,
	const stats::prefix_t & prefix,
	work_thread::work_thread_with_activity_tracking_t & wt )
	{
		so_5::send< stats::messages::work_thread_activity >(
				mbox,
				prefix,
				stats::suffixes::work_thread_activity(),
				wt.thread_id(),
				wt.take_activity_stats() );
	}

} /* anonymous */

//
// dispatcher_template_t
//

/*!
 * \brief Implementation of active object dispatcher in form of template class.
 */
template< typename Work_Thread >
class dispatcher_template_t : public disp_binder_t
	{
	public:
		dispatcher_template_t(
			//! SObjectizer Environment to work in.
			outliving_reference_t< environment_t > env,
			//! Base part of data sources names.
			const std::string_view name_base,
			//! Dispatcher's parameters.
			disp_params_t params )
			:	m_params{ std::move(params) }
			,	m_data_source{
					env,
					name_base,
					outliving_mutable( *this )
				}
			{}

		~dispatcher_template_t() noexcept override
			{
				// All working threads should receive stop signal.
				for( auto & p: m_agent_threads )
					p.second->shutdown();

				// All working threads should be joined.
				for( auto & p: m_agent_threads )
					p.second->wait();
			}

		void
		preallocate_resources(
			agent_t & agent ) override
			{
				std::lock_guard lock{ m_lock };

				if( m_agent_threads.end() != m_agent_threads.find( &agent ) )
					SO_5_THROW_EXCEPTION(
							rc_disp_create_failed,
							"thread for the agent is already exists" );

				auto lock_factory = m_params.queue_params().lock_factory();
				auto thread = std::make_shared< Work_Thread >(
						std::move(lock_factory) );

				thread->start();
				so_5::details::do_with_rollback_on_exception(
						[&] { m_agent_threads[ &agent ] = thread; },
						[&thread] { shutdown_and_wait( *thread ); } );
			}

		void
		undo_preallocation(
			agent_t & agent ) noexcept override
			{
				const auto eject_thread = [&] {
					std::lock_guard lock{ m_lock };

					auto it = m_agent_threads.find( &agent );
					auto thread = it->second;
					m_agent_threads.erase( it );

					return thread;
				};

				shutdown_and_wait( *eject_thread() );
			}

		void
		bind(
			agent_t & agent ) noexcept override
			{
				const auto get_queue = [&] {
					std::lock_guard lock{ m_lock };
					return m_agent_threads.find( &agent )->second->get_agent_binding();
				};

				agent.so_bind_to_dispatcher( *get_queue() );
			}

		void
		unbind(
			agent_t & agent ) noexcept override
			{
				// We should perform the same actions as for undo_preallocation.
				undo_preallocation( agent );
			}

	private:
		friend class disp_data_source_t;

		//! An alias for shared pointer to work thread object.
		using work_thread_shptr_t = std::shared_ptr< Work_Thread >;

		//! Typedef for mapping from agents to their working threads.
		using agent_thread_map_t =
				std::map< const agent_t *, work_thread_shptr_t >;

		/*!
		 * \brief Data source for run-time monitoring of whole dispatcher.
		 * \since
		 * v.5.5.4
		 */
		class disp_data_source_t : public stats::auto_registered_source_t
			{
				//! Dispatcher to work with.
				outliving_reference_t< dispatcher_template_t > m_dispatcher;

				//! Basic prefix for data source names.
				stats::prefix_t m_base_prefix;

			public :
				disp_data_source_t(
					outliving_reference_t< environment_t > env,
					const std::string_view name_base,
					outliving_reference_t< dispatcher_template_t > disp )
					:	stats::auto_registered_source_t{
							outliving_mutable( env.get().stats_repository() )
						}
					,	m_dispatcher{ disp }
					{
						using namespace so_5::disp::reuse;

						m_base_prefix = make_disp_prefix(
								"ao", // ao -- active_objects
								name_base,
								&(m_dispatcher.get()) );
					}

				void
				distribute( const mbox_t & mbox ) override
					{
						auto & disp = m_dispatcher.get();

						std::lock_guard lock{ disp.m_lock };

						so_5::send< stats::messages::quantity< std::size_t > >(
								mbox,
								m_base_prefix,
								stats::suffixes::agent_count(),
								disp.m_agent_threads.size() );

						for( const auto & [a, wt] : disp.m_agent_threads )
							distribute_value_for_work_thread( mbox, a, *wt );
					}

			private:
				void
				distribute_value_for_work_thread(
					const mbox_t & mbox,
					const agent_t * agent,
					Work_Thread & wt )
					{
						std::ostringstream ss;
						ss << m_base_prefix.c_str() << "/wt-"
								<< so_5::disp::reuse::ios_helpers::pointer{ agent };

						const stats::prefix_t wt_prefix{ ss.str() };

						send_demands_count_stats( mbox, wt_prefix, wt );
						send_thread_activity_stats( mbox, wt_prefix, wt );
					}
			};

		/*!
		 * \brief Parameters for the dispatcher.
		 *
		 * \since
		 * v.5.5.10
		 */
		const disp_params_t m_params;

		//! This object lock.
		std::mutex m_lock;

		//! A map from agents to single thread dispatchers.
		agent_thread_map_t m_agent_threads;

		/*!
		 * \brief Data source for run-time monitoring.
		 *
		 * \since
		 * v.5.5.4
		 */
		disp_data_source_t m_data_source;
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
						work_thread::work_thread_no_activity_tracking_t >;

		using dispatcher_with_activity_tracking_t =
				impl::dispatcher_template_t<
						work_thread::work_thread_with_activity_tracking_t >;

		disp_binder_shptr_t binder = so_5::disp::reuse::make_actual_dispatcher<
						disp_binder_t,
						dispatcher_no_activity_tracking_t,
						dispatcher_with_activity_tracking_t >(
				outliving_reference_t(env),
				data_sources_name_base,
				std::move(params) );

		return impl::dispatcher_handle_maker_t::make( std::move(binder) );
	}

} /* namespace active_obj */

} /* namespace disp */

} /* namespace so_5 */


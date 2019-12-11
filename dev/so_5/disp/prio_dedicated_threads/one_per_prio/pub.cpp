/*
	SObjectizer 5.
*/

/*!
 * \file
 * \brief Functions for creating and binding of the dispatcher with
 * dedicated threads per priority.
 *
 * \since
 * v.5.5.8
 */

#include <so_5/disp/prio_dedicated_threads/one_per_prio/pub.hpp>

#include <so_5/disp/reuse/work_thread/work_thread.hpp>

#include <so_5/disp/reuse/data_source_prefix_helpers.hpp>
#include <so_5/disp/reuse/make_actual_dispatcher.hpp>

#include <so_5/stats/repository.hpp>
#include <so_5/stats/messages.hpp>
#include <so_5/stats/std_names.hpp>

#include <so_5/send_functions.hpp>

#include <so_5/details/invoke_noexcept_code.hpp>

#include <algorithm>

namespace so_5 {

namespace disp {

namespace prio_dedicated_threads {

namespace one_per_prio {

namespace impl {

namespace stats = so_5::stats;

namespace {

void
send_thread_activity_stats(
	const so_5::mbox_t &,
	const stats::prefix_t &,
	so_5::disp::reuse::work_thread::work_thread_no_activity_tracking_t & )
	{
		/* Nothing to do */
	}

void
send_thread_activity_stats(
	const so_5::mbox_t & mbox,
	const stats::prefix_t & prefix,
	so_5::disp::reuse::work_thread::work_thread_with_activity_tracking_t & wt )
	{
		so_5::send< stats::messages::work_thread_activity >(
				mbox,
				prefix,
				stats::suffixes::work_thread_activity(),
				wt.thread_id(),
				wt.take_activity_stats() );
	}

} /* namespace anonymous */

//
// dispatcher_template_t
//
/*!
 * \brief An actual implementation of dispatcher with dedicated thread
 * for every priority in form of a template class.
 *
 * \since
 * v.5.5.8, v.5.5.18, v.5.6.0
 */
template< typename Work_Thread >
class dispatcher_template_t final : public disp_binder_t
	{
	public:
		dispatcher_template_t(
			outliving_reference_t< environment_t > env,
			const std::string_view name_base,
			disp_params_t params )
			:	m_data_source{
					outliving_mutable(env.get().stats_repository()),
					name_base,
					outliving_mutable(*this)
				}
			{
				allocate_work_threads( params );
				launch_work_threads();
			}

		~dispatcher_template_t() noexcept override
			{
				for( auto & t : m_threads )
					t->shutdown();

				for( auto & t : m_threads )
					t->wait();
			}

		void
		preallocate_resources(
			agent_t & /*agent*/ ) override
			{
				// Nothing to do.
			}

		void
		undo_preallocation(
			agent_t & /*agent*/ ) noexcept override
			{
				// Nothing to do.
			}

		void
		bind(
			agent_t & agent ) noexcept override
			{
				const auto priority = agent.so_priority();

				agent.so_bind_to_dispatcher(
						*(m_threads[ to_size_t(priority) ]->get_agent_binding()) );

				m_agents_per_priority[ to_size_t(priority) ] += 1;
			}

		void
		unbind(
			agent_t & agent ) noexcept override
			{
				const auto priority = agent.so_priority();

				m_agents_per_priority[ to_size_t(priority) ] -= 1;
			}

	private:
		friend class disp_data_source_t;

		/*!
		 * \brief Data source for run-time monitoring of whole dispatcher.
		 *
		 * \since
		 * v.5.5.8
		 */
		class disp_data_source_t final : public stats::source_t
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
								"pdt-opp",
								name_base,
								&(disp.get()) )
						}
					{}

				void
				distribute( const mbox_t & mbox ) override
					{
						std::size_t agents_count = 0;

						auto & disp = m_dispatcher.get();

						so_5::prio::for_each_priority( [&]( priority_t p ) {
								auto agents = disp.m_agents_per_priority[
										to_size_t(p) ].load( std::memory_order_acquire );

								agents_count += agents;

								distribute_value_for_work_thread(
										mbox,
										p,
										agents,
										*(disp.m_threads[ to_size_t(p) ]) );
							} );

						so_5::send< stats::messages::quantity< std::size_t > >(
								mbox,
								m_base_prefix,
								stats::suffixes::agent_count(),
								agents_count );
					}

			private:
				void
				distribute_value_for_work_thread(
					const mbox_t & mbox,
					priority_t priority,
					std::size_t agents_count,
					Work_Thread & wt )
					{
						std::ostringstream ss;
						ss << m_base_prefix.c_str() << "/wt-p" << to_size_t(priority);

						const stats::prefix_t prefix{ ss.str() };

						so_5::send< stats::messages::quantity< std::size_t > >(
								mbox,
								prefix,
								stats::suffixes::work_thread_queue_size(),
								wt.demands_count() );

						so_5::send< stats::messages::quantity< std::size_t > >(
								mbox,
								prefix,
								stats::suffixes::agent_count(),
								agents_count );

						send_thread_activity_stats( mbox, prefix, wt );
					}
			};

		//! Data source for run-time monitoring.
		stats::auto_registered_source_holder_t< disp_data_source_t >
				m_data_source;

		//! Working threads for every priority.
		std::vector< std::unique_ptr< Work_Thread > > m_threads;

		//! Counters for agent count for every priority.
		std::atomic< std::size_t > m_agents_per_priority[ so_5::prio::total_priorities_count ];

		//! Allocate work threads for dispatcher.
		void
		allocate_work_threads( const disp_params_t & params )
			{
				m_threads.reserve( so_5::prio::total_priorities_count );
				so_5::prio::for_each_priority( [&]( so_5::priority_t ) {
						auto lock_factory = params.queue_params().lock_factory();

						auto t = std::make_unique< Work_Thread >(
								std::move(lock_factory) );

						m_threads.push_back( std::move(t) );
					} );
			}

		//! Start all working threads.
		void
		launch_work_threads()
			{
				using namespace std;
				using namespace so_5::details;
				using namespace so_5::prio;

				// This helper vector will be used for shutdown of
				// started threads in the case of an exception.
				Work_Thread * started_threads[ total_priorities_count ];
				// Initially all items must be NULL.
				fill( begin(started_threads), end(started_threads), nullptr );

				do_with_rollback_on_exception( [&] {
						for( std::size_t i = 0; i != total_priorities_count; ++i )
							{
								m_agents_per_priority[ i ].store( 0,
										std::memory_order_release );

								m_threads[ i ]->start();

								// Thread successfully started. Pointer to it
								// must be used on rollback.
								started_threads[ i ] = m_threads[ i ].get();
							}
					},
					[&] {
						invoke_noexcept_code( [&] {
							// Shutdown all started threads.
							for( auto t : started_threads )
								if( t )
									{
										t->shutdown();
										t->wait();
									}
								else
									// No more started threads.
									break;
						} );
					} );
			}
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
				outliving_mutable(env),
				data_sources_name_base,
				std::move(params) );

		return impl::dispatcher_handle_maker_t::make( std::move(binder) );
	}

} /* namespace one_per_prio */

} /* namespace prio_dedicated_threads */

} /* namespace disp */

} /* namespace so_5 */


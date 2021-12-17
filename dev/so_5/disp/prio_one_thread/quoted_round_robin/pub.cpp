/*
	SObjectizer 5.
*/

/*!
 * \file
 * \brief Functions for creating and binding of the single thread dispatcher
 * with priority support (quoted round robin policy).
 *
 * \since
 * v.5.5.8
 */

#include <so_5/disp/prio_one_thread/quoted_round_robin/pub.hpp>

#include <so_5/disp/prio_one_thread/quoted_round_robin/impl/demand_queue.hpp>
#include <so_5/disp/prio_one_thread/reuse/work_thread.hpp>

#include <so_5/disp/reuse/actual_work_thread_factory_to_use.hpp>
#include <so_5/disp/reuse/data_source_prefix_helpers.hpp>
#include <so_5/disp/reuse/make_actual_dispatcher.hpp>

#include <so_5/stats/repository.hpp>
#include <so_5/stats/messages.hpp>
#include <so_5/stats/std_names.hpp>

#include <so_5/send_functions.hpp>

namespace so_5 {

namespace disp {

namespace prio_one_thread {

namespace quoted_round_robin {

namespace impl {

namespace stats = so_5::stats;

namespace {

void
send_thread_activity_stats(
	const so_5::mbox_t &,
	const stats::prefix_t &,
	so_5::disp::prio_one_thread::reuse::work_thread_no_activity_tracking_t<
			demand_queue_t > & )
	{
		/* Nothing to do */
	}

void
send_thread_activity_stats(
	const so_5::mbox_t & mbox,
	const stats::prefix_t & prefix,
	so_5::disp::prio_one_thread::reuse::work_thread_with_activity_tracking_t<
			demand_queue_t > & wt )
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
 * \brief An implementation of dispatcher with one working thread and support
 * of demand priorities (quoted round robin policy) in form of template class.
 *
 * \since
 * v.5.5.8, v.5.5.18, v.5.6.0
 */
template< typename Work_Thread >
class dispatcher_template_t final : public disp_binder_t
	{
		friend class disp_data_source_t;

	public:
		dispatcher_template_t(
			outliving_reference_t< environment_t > env,
			const std::string_view name_base,
			disp_params_t params,
			const quotes_t & quotes )
			:	m_demand_queue{
					params.queue_params().lock_factory()(),
					quotes }
			,	m_work_thread{
					so_5::disp::reuse::acquire_work_thread( params, env.get() ),
					m_demand_queue }
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
				m_demand_queue.stop();
				m_work_thread.join();
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
						m_demand_queue.event_queue_by_priority( priority ) );

				m_demand_queue.agent_bound( priority );
			}

		void
		unbind(
			agent_t & agent ) noexcept override
			{
				const auto priority = agent.so_priority();

				m_demand_queue.agent_unbound( priority );
			}

	private:
		/*!
		 * \brief Data source for run-time monitoring of whole dispatcher.
		 *
		 * \since
		 * v.5.5.8
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
								"pot-qrr",
								name_base,
								&(disp.get()) )
						}
					{}

				void
				distribute( const mbox_t & mbox ) override
					{
						auto & disp = m_dispatcher.get();

						std::size_t agents_count = 0;

						disp.m_demand_queue.handle_stats_for_each_prio(
							[&]( const demand_queue_t::queue_stats_t & stat ) {
								distribute_value_for_priority(
									mbox,
									stat.m_priority,
									stat.m_quote,
									stat.m_agents_count,
									stat.m_demands_count );

								agents_count += stat.m_agents_count;
							} );

						so_5::send< stats::messages::quantity< std::size_t > >(
								mbox,
								m_base_prefix,
								stats::suffixes::agent_count(),
								agents_count );

						send_thread_activity_stats(
								mbox,
								m_base_prefix,
								disp.m_work_thread );
					}

			private:
				void
				distribute_value_for_priority(
					const mbox_t & mbox,
					priority_t priority,
					std::size_t quote,
					std::size_t agents_count,
					std::size_t demands_count )
					{
						std::ostringstream ss;
						ss << m_base_prefix.c_str() << "/p" << to_size_t(priority);

						const stats::prefix_t prefix{ ss.str() };

						so_5::send< stats::messages::quantity< std::size_t > >(
								mbox,
								prefix,
								stats::suffixes::demand_quote(),
								quote );

						so_5::send< stats::messages::quantity< std::size_t > >(
								mbox,
								prefix,
								stats::suffixes::agent_count(),
								agents_count );

						so_5::send< stats::messages::quantity< std::size_t > >(
								mbox,
								prefix,
								stats::suffixes::work_thread_queue_size(),
								demands_count );
					}
			};

		//! Demand queue for the dispatcher.
		demand_queue_t m_demand_queue;

		//! Working thread for the dispatcher.
		Work_Thread m_work_thread;

		//! Data source for run-time monitoring.
		stats::auto_registered_source_holder_t< disp_data_source_t >
				m_data_source;
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
	const quotes_t & quotes,
	disp_params_t params )
	{
		using namespace so_5::disp::reuse;
		using namespace so_5::disp::prio_one_thread::reuse;

		using dispatcher_no_activity_tracking_t =
				impl::dispatcher_template_t<
						work_thread_no_activity_tracking_t< impl::demand_queue_t > >;

		using dispatcher_with_activity_tracking_t =
				impl::dispatcher_template_t<
						work_thread_with_activity_tracking_t< impl::demand_queue_t > >;

		disp_binder_shptr_t binder = so_5::disp::reuse::make_actual_dispatcher<
						disp_binder_t,
						dispatcher_no_activity_tracking_t,
						dispatcher_with_activity_tracking_t >(
				outliving_mutable(env),
				data_sources_name_base,
				std::move(params),
				quotes );

		return impl::dispatcher_handle_maker_t::make( std::move(binder) );
	}

} /* namespace quoted_round_robin */

} /* namespace prio_one_thread */

} /* namespace disp */

} /* namespace so_5 */


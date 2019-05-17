/*
	SObjectizer 5.
*/

#include <so_5/disp/active_group/pub.hpp>

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

namespace active_group
{

namespace impl
{

namespace work_thread = so_5::disp::reuse::work_thread;
namespace stats = so_5::stats;

namespace
{

/*!
 * \brief Just a helper function for consequetive call to shutdown and wait.
 *
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
// actual_dispatcher_iface_t
//
/*!
 * \brief An actual interface of active group dispatcher.
 *
 * \since
 * v.5.6.0
 */
class actual_dispatcher_iface_t : public basic_dispatcher_iface_t
	{
	public :
		/*!
		 * \brief Create a new thread for a group if it necessary.
		 *
		 * If name \a group_name is unknown then a new work
		 * thread is started. This thread is marked as it has one
		 * working agent on it.
		 *
		 * If there already is a thread for \a group_name then the
		 * counter of working agents is incremented.
		 */
		virtual void
		allocate_thread_for_group( const std::string & group_name ) = 0;

		/*!
		 * \brief Get the event_queue for the specified active group.
		 *
		 * It is expected that thread for the group is already
		 * created by calling allocate_thread_for_group() method.
		 */
		virtual so_5::event_queue_t *
		query_thread_for_group( const std::string & group_name ) noexcept = 0;

		/*!
		 * \brief Release the thread for the specified active group.
		 *
		 * Method decrements the working agent count for the thread of
		 * \a group_name. If there no more working agents left then
		 * the event_queue and working thread for that group will be
		 * destroyed.
		 */
		virtual void
		release_thread_for_group( const std::string & group_name ) noexcept = 0;
	};

//
// actual_dispatcher_iface_shptr_t
//
using actual_dispatcher_iface_shptr_t =
		std::shared_ptr< actual_dispatcher_iface_t >;

//
// actual_binder_t
//
/*!
 * \brief Implementation of binder interface for %active_group dispatcher.
 *
 * \since
 * v.5.6.0
 */
class actual_binder_t final : public disp_binder_t
	{
		//! Dispatcher to be used.
		actual_dispatcher_iface_shptr_t m_disp;
		//! Name of group for new agents.
		const std::string m_group_name;

	public :
		actual_binder_t(
			actual_dispatcher_iface_shptr_t disp,
			nonempty_name_t group_name ) noexcept
			:	m_disp{ std::move(disp) }
			,	m_group_name{ group_name.giveout_value() }
			{}

		void
		preallocate_resources(
			agent_t & /*agent*/ ) override
			{
				m_disp->allocate_thread_for_group( m_group_name );
			}

		void
		undo_preallocation(
			agent_t & /*agent*/ ) noexcept override
			{
				m_disp->release_thread_for_group( m_group_name );
			}

		void
		bind(
			agent_t & agent ) noexcept override
			{
				auto queue = m_disp->query_thread_for_group( m_group_name );
				agent.so_bind_to_dispatcher( *queue );
			}

		void
		unbind(
			agent_t & /*agent*/ ) noexcept override
			{
				m_disp->release_thread_for_group( m_group_name );
			}
	};

//
// dispatcher_template_t
//

/*!
 * \brief Implementation of active object dispatcher in form of template class.
 */
template< typename Work_Thread >
class dispatcher_template_t final : public actual_dispatcher_iface_t
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
					outliving_mutable(env.get().stats_repository()),
					name_base,
					outliving_mutable( *this )
				}
			{}

		~dispatcher_template_t() noexcept override
			{
				// All working threads should receive stop signal.
				for( auto & p: m_groups )
					p.second.m_thread->shutdown();

				// All working threads should be joined.
				for( auto & p: m_groups )
					p.second.m_thread->wait();
			}

		disp_binder_shptr_t
		binder( nonempty_name_t group_name ) override
			{
				return std::make_shared< actual_binder_t >(
						this->shared_from_this(),
						std::move(group_name) );
			}

		void
		allocate_thread_for_group( const std::string & group_name ) override
			{
				std::lock_guard lock{ m_lock };

				auto it = m_groups.find( group_name );

				if( m_groups.end() == it )
					{
						// New thread should be created.
						auto thread = std::make_shared< Work_Thread >(
								m_params.queue_params().lock_factory() );

						thread->start();

						so_5::details::do_with_rollback_on_exception(
								[&] {
									m_groups.emplace(
											group_name,
											thread_with_refcounter_t{ thread, 1u } );
								},
								[&thread] { shutdown_and_wait( *thread ); } );
					}
			}

		so_5::event_queue_t *
		query_thread_for_group( const std::string & group_name ) noexcept override
			{
				std::lock_guard lock{ m_lock };

				return m_groups.find( group_name )->second.m_thread->
						get_agent_binding();
			}

		void
		release_thread_for_group( const std::string & group_name ) noexcept override
			{
				auto thread = search_and_try_remove_group_from_map( group_name );
				if( thread )
					shutdown_and_wait( *thread );
			}

	private:
		friend class disp_data_source_t;

		//! An alias for shared pointer to work thread.
		using work_thread_shptr_t = std::shared_ptr< Work_Thread >;

		//! Auxiliary class for the working agent counting.
		struct thread_with_refcounter_t
			{
				work_thread_shptr_t m_thread;
				std::size_t m_user_agent;
			};

		//! Typedef for mapping from group names to a single thread
		//! dispatcher.
		using active_group_map_t =
				std::map< std::string, thread_with_refcounter_t >;

		/*!
		 * \brief Data source for run-time monitoring of whole dispatcher.
		 *
		 * \since
		 * v.5.5.4
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
					{
						using namespace so_5::disp::reuse;

						m_base_prefix = make_disp_prefix(
								"ag", // ao -- active_groups
								name_base,
								&m_dispatcher );
					}

				void
				distribute( const so_5::mbox_t & mbox ) override
					{
						auto & disp = m_dispatcher.get();

						std::lock_guard lock{ disp.m_lock };

						so_5::send< stats::messages::quantity< std::size_t > >(
								mbox,
								m_base_prefix,
								stats::suffixes::disp_active_group_count(),
								disp.m_groups.size() );

						std::size_t agent_count = 0;
						for( const auto & p : disp.m_groups )
							{
								distribute_value_for_work_thread(
										mbox,
										p.first,
										p.second );

								agent_count += p.second.m_user_agent;
							}

						so_5::send< stats::messages::quantity< std::size_t > >(
								mbox,
								m_base_prefix,
								stats::suffixes::agent_count(),
								agent_count );
					}

			private:
				void
				distribute_value_for_work_thread(
					const so_5::mbox_t & mbox,
					const std::string & group_name,
					const thread_with_refcounter_t & wt )
					{
						std::ostringstream ss;
						ss << m_base_prefix.c_str() << "/wt-" << group_name;

						const stats::prefix_t prefix{ ss.str() };

						so_5::send< stats::messages::quantity< std::size_t > >(
								mbox,
								prefix,
								stats::suffixes::agent_count(),
								wt.m_user_agent );

						so_5::send< stats::messages::quantity< std::size_t > >(
								mbox,
								prefix,
								stats::suffixes::work_thread_queue_size(),
								wt.m_thread->demands_count() );

						send_thread_activity_stats(
								mbox,
								prefix,
								*(wt.m_thread) );
					}
			};

		//! Parameters for the dispatcher.
		const disp_params_t m_params;

		//! A map of dispatchers for active groups.
		active_group_map_t m_groups;

		//! This object lock.
		std::mutex m_lock;

		/*!
		 * \brief Data source for run-time monitoring.
		 *
		 * \since
		 * v.5.5.4, v.5.6.0
		 */
		stats::auto_registered_source_holder_t< disp_data_source_t >
				m_data_source;

		/*!
		 * \brief Helper function for searching and erasing agent's
		 * thread from map of active threads.
		 *
		 * \since
		 * v.5.5.4
		 *
		 * \note Does all actions on locked object.
		 *
		 * \return nullptr if thread for the group is not found
		 * or there are still some agents on it.
		 */
		work_thread_shptr_t
		search_and_try_remove_group_from_map(
			const std::string & group_name ) noexcept
			{
				work_thread_shptr_t result;

				std::lock_guard lock{ m_lock };

				auto it = m_groups.find( group_name );

				if( m_groups.end() != it && 0u == --(it->second.m_user_agent) )
					{
						result = it->second.m_thread;
						m_groups.erase( it );
					}

				return result;
			}
};

//
// dispatcher_handle_maker_t
//
class dispatcher_handle_maker_t
	{
	public :
		static dispatcher_handle_t
		make( actual_dispatcher_iface_shptr_t disp ) noexcept
			{
				return { std::move( disp ) };
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

		auto binder = so_5::disp::reuse::make_actual_dispatcher<
						impl::actual_dispatcher_iface_t,
						dispatcher_no_activity_tracking_t,
						dispatcher_with_activity_tracking_t >(
				outliving_reference_t(env),
				data_sources_name_base,
				std::move(params) );

		return impl::dispatcher_handle_maker_t::make( std::move(binder) );
	}

} /* namespace active_group */

} /* namespace disp */

} /* namespace so_5 */


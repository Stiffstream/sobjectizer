/*
	SObjectizer 5.
*/

#include <so_5/disp/one_thread/pub.hpp>

#include <so_5/environment.hpp>
#include <so_5/send_functions.hpp>

#include <so_5/stats/repository.hpp>
#include <so_5/stats/messages.hpp>
#include <so_5/stats/std_names.hpp>

#include <so_5/stats/impl/activity_tracking.hpp>

#include <so_5/disp/reuse/work_thread/work_thread.hpp>

#include <so_5/disp/reuse/data_source_prefix_helpers.hpp>
#include <so_5/disp/reuse/make_actual_dispatcher.hpp>

#include <so_5/details/rollback_on_exception.hpp>

#include <atomic>

namespace so_5
{

namespace disp
{

namespace one_thread
{

namespace impl
{

namespace work_thread = so_5::disp::reuse::work_thread;
namespace stats = so_5::stats;

namespace data_source_details
{

template< typename Work_Thread >
struct common_data_t
	{
		//! Prefix for dispatcher-related data.
		stats::prefix_t m_base_prefix;
		//! Prefix for working thread-related data.
		stats::prefix_t m_work_thread_prefix;

		//! Working thread of the dispatcher.
		Work_Thread & m_work_thread;

		//! Count of agents bound to the dispatcher.
		std::atomic< std::size_t > & m_agents_bound;

		common_data_t(
			Work_Thread & work_thread,
			std::atomic< std::size_t > & agents_bound )
			:	m_work_thread( work_thread )
			,	m_agents_bound( agents_bound )
		{}
	};

inline void
track_activity(
	const mbox_t &,
	const common_data_t< work_thread::work_thread_no_activity_tracking_t > & )
	{}

inline void
track_activity(
	const mbox_t & mbox,
	const common_data_t< work_thread::work_thread_with_activity_tracking_t > & data )
	{
		so_5::send< stats::messages::work_thread_activity >(
				mbox,
				data.m_base_prefix,
				stats::suffixes::work_thread_activity(),
				data.m_work_thread.thread_id(),
				data.m_work_thread.take_activity_stats() );
	}

} /* namespace data_source_details */

/*!
 * \brief Data source for one-thread dispatcher.
 */
template< typename Work_Thread >
class data_source_t final
	:	public stats::source_t
	,	protected data_source_details::common_data_t< Work_Thread >
	{
	public :
		using actual_work_thread_type_t = Work_Thread;

		data_source_t(
			actual_work_thread_type_t & work_thread,
			std::atomic< std::size_t > & agents_bound,
			const std::string_view name_base,
			const void * pointer_to_disp )
			:	data_source_details::common_data_t< Work_Thread >{
					work_thread, agents_bound }
			{
				// Names of data sources must be formed.
				using namespace so_5::disp::reuse;

				this->m_base_prefix = make_disp_prefix(
						"ot", // ot -- one_thread
						name_base,
						pointer_to_disp );

				this->m_work_thread_prefix = make_disp_working_thread_prefix(
						this->m_base_prefix,
						0 );
			}

		void
		distribute( const mbox_t & mbox ) override
			{
				so_5::send< stats::messages::quantity< std::size_t > >(
						mbox,
						this->m_base_prefix,
						stats::suffixes::agent_count(),
						this->m_agents_bound.load( std::memory_order_acquire ) );

				so_5::send< stats::messages::quantity< std::size_t > >(
						mbox,
						this->m_work_thread_prefix,
						stats::suffixes::work_thread_queue_size(),
						this->m_work_thread.demands_count() );

				data_source_details::track_activity( mbox, *this );
			}
	};

//
// actual_dispatcher_t
//

/*!
 * \brief A dispatcher with the single working thread and an event queue.
 *
 * \note
 * This class implements disp_binder_t. It makes possible to use instance
 * of that class as dispatcher binder (this avoids creation of small
 * binder objects).
 *
 * \since
 * v.5.6.0
 */
template< typename Work_Thread >
class actual_dispatcher_t final : public disp_binder_t
	{
	public:
		actual_dispatcher_t(
			outliving_reference_t< environment_t > env,
			const std::string_view name_base,
			disp_params_t params )
			:	m_work_thread{ params.queue_params().lock_factory() }
			,	m_data_source{
					outliving_mutable(env.get().stats_repository()),
					m_work_thread,
					m_agents_bound,
					name_base,
					this }
			{
				m_work_thread.start();
			}

		~actual_dispatcher_t() noexcept override
			{
				m_work_thread.shutdown();
				m_work_thread.wait();
			}

		// Implementation of methods, inherited from disp_binder.
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

		virtual void
		bind(
			agent_t & agent ) noexcept override
			{
				agent.so_bind_to_dispatcher( *(m_work_thread.get_agent_binding()) );
				++m_agents_bound;
			}

		virtual void
		unbind(
			agent_t & /*agent*/ ) noexcept override
			{
				--m_agents_bound;
			}

	private:
		//! Working thread for the dispatcher.
		Work_Thread m_work_thread;

		/*!
		 * \since
		 * v.5.5.4
		 *
		 * \brief Count of agents bound to this dispatcher.
		 */
		std::atomic< std::size_t > m_agents_bound = { 0 };

		/*!
		 * \brief Data source for run-time monitoring.
		 *
		 * \since
		 * v.5.5.4, v.5.6.0
		 */
		stats::auto_registered_source_holder_t< data_source_t< Work_Thread > >
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
	disp_params_t params )
	{
		using namespace so_5::disp::reuse;

		using dispatcher_no_activity_tracking_t =
				impl::actual_dispatcher_t<
						work_thread::work_thread_no_activity_tracking_t >;

		using dispatcher_with_activity_tracking_t =
				impl::actual_dispatcher_t<
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

} /* namespace one_thread */

} /* namespace disp */

} /* namespace so_5 */


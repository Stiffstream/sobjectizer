/*
 * SObjectizer-5
 */

/*!
 * \file
 * \brief Public interface of thread pool dispatcher that
 * provides noexcept guarantee for scheduling evt_finish demand.
 *
 * \since v.5.8.0
 */

#include <so_5/disp/nef_thread_pool/pub.hpp>

#include <so_5/disp/thread_pool/impl/work_thread_template.hpp>
#include <so_5/disp/thread_pool/impl/basic_event_queue.hpp>

#include <so_5/disp/reuse/make_actual_dispatcher.hpp>

#include <so_5/ret_code.hpp>

#include <so_5/disp_binder.hpp>
#include <so_5/environment.hpp>

namespace so_5
{

namespace disp
{

namespace nef_thread_pool
{

namespace impl
{

using so_5::disp::thread_pool::impl::work_thread_no_activity_tracking_t;
using so_5::disp::thread_pool::impl::work_thread_with_activity_tracking_t;

class agent_queue_with_preallocated_finish_demand_t;

//
// dispatcher_queue_t
//
using dispatcher_queue_t = so_5::disp::reuse::queue_of_queues_t<
		agent_queue_with_preallocated_finish_demand_t >;

//
// agent_queue_with_preallocated_finish_demand_t
//
//FIXME: document this!
class agent_queue_with_preallocated_finish_demand_t final
	:	public so_5::disp::thread_pool::impl::basic_event_queue_t
	,	private so_5::atomic_refcounted_t
	{
		friend class so_5::intrusive_ptr_t< agent_queue_with_preallocated_finish_demand_t >;

		using base_type_t = so_5::disp::thread_pool::impl::basic_event_queue_t;

	public:
		agent_queue_with_preallocated_finish_demand_t(
			//! Dispatcher queue to work with.
			outliving_reference_t< dispatcher_queue_t > disp_queue,
			//! Parameters for the queue.
			const bind_params_t & params )
			:	base_type_t{ params.query_max_demands_at_once() }
			,	m_disp_queue{ disp_queue.get() }
			,	m_finish_demand{ std::make_unique< base_type_t::demand_t >() }
			{}

		void
		push_evt_finish( execution_demand_t demand ) noexcept override
			{
				// It's assumed that m_finish_demand isn't empty.
				*(m_finish_demand) = std::move(demand);

				// Just delegate the work.
				this->push_preallocated( std::move(m_finish_demand) );
			}

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
		agent_queue_with_preallocated_finish_demand_t *
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
		intrusive_queue_set_next(
			agent_queue_with_preallocated_finish_demand_t * next ) noexcept
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

		//FIXME: document this!
		std::unique_ptr< base_type_t::demand_t > m_finish_demand;

		/*!
		 * \brief The next item in intrusive queue of agent_queues.
		 *
		 * This field is necessary to implement interface required by
		 * so_5::disp::reuse::queue_of_queues_t
		 *
		 * \since v.5.8.0
		 */
		agent_queue_with_preallocated_finish_demand_t * m_intrusive_queue_next{ nullptr };
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
				return { "nef_tp" }; // nef_thread_pool.
			}

		[[nodiscard]]
		static bool
		is_individual_fifo( const bind_params_t & /*params*/ ) noexcept
			{
				// NOTE: all agents use individual fifo.
				return true;
			}

		static void
		wait_for_queue_emptyness(
			agent_queue_with_preallocated_finish_demand_t & queue ) noexcept
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
		so_5::disp::thread_pool::common_implementation::dispatcher_t<
				Work_Thread,
				dispatcher_queue_t,
				bind_params_t,
				adaptation_t >;

//
// actual_dispatcher_iface_t
//
/*!
 * \brief An actual interface of nef-thread-pool dispatcher.
 *
 * This interface defines a set of methods necessary for binder.
 *
 * \since v.5.8.0
 */
class actual_dispatcher_iface_t : public basic_dispatcher_iface_t
	{
	public :
		//! Preallocate all necessary resources for a new agent.
		virtual void
		preallocate_resources_for_agent(
			agent_t & agent,
			const bind_params_t & params ) = 0;

		//! Undo preallocation of resources for a new agent.
		virtual void
		undo_preallocation_for_agent(
			agent_t & agent ) noexcept = 0;

		//! Get resources allocated for an agent.
		[[nodiscard]]
		virtual event_queue_t *
		query_resources_for_agent( agent_t & agent ) noexcept = 0;

		//! Unbind agent from the dispatcher.
		virtual void
		unbind_agent( agent_t & agent ) noexcept = 0;
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
 * \brief Actual implementation of dispatcher binder for %nef_thread_pool dispatcher.
 *
 * \since v.5.8.0
 */
class actual_binder_t final : public disp_binder_t
	{
		//! Dispatcher to be used.
		actual_dispatcher_iface_shptr_t m_disp;
		//! Binding parameters.
		const bind_params_t m_params;

	public :
		actual_binder_t(
			actual_dispatcher_iface_shptr_t disp,
			bind_params_t params ) noexcept
			:	m_disp{ std::move(disp) }
			,	m_params{ params }
			{}

		void
		preallocate_resources(
			agent_t & agent ) override
			{
				m_disp->preallocate_resources_for_agent( agent, m_params );
			}

		void
		undo_preallocation(
			agent_t & agent ) noexcept override
			{
				m_disp->undo_preallocation_for_agent( agent );
			}

		void
		bind(
			agent_t & agent ) noexcept override
			{
				auto queue = m_disp->query_resources_for_agent( agent );
				agent.so_bind_to_dispatcher( *queue );
			}

		void
		unbind(
			agent_t & agent ) noexcept override
			{
				m_disp->unbind_agent( agent );
			}
	};

//
// actual_dispatcher_implementation_t
//
/*!
 * \brief Actual implementation of binder for %nef_thread_pool dispatcher.
 *
 * \since v.5.8.0
 */
template< typename Work_Thread >
class actual_dispatcher_implementation_t final
	:	public actual_dispatcher_iface_t
	{
		//! Real dispatcher.
		dispatcher_template_t< Work_Thread > m_impl;

	public :
		actual_dispatcher_implementation_t(
			//! SObjectizer Environment to work in.
			outliving_reference_t< environment_t > env,
			//! Base part of data sources names.
			const std::string_view name_base,
			//! Dispatcher's parameters.
			disp_params_t params )
			:	m_impl{
					env.get(),
					params,
					name_base,
					params.thread_count(),
					params.queue_params()
				}
			{
				m_impl.start( env.get() );
			}

		~actual_dispatcher_implementation_t() noexcept override
			{
				m_impl.shutdown_then_wait();
			}

		[[nodiscard]]
		disp_binder_shptr_t
		binder( bind_params_t params ) override
			{
				return std::make_shared< actual_binder_t >(
						this->shared_from_this(),
						params );
			}

		void
		preallocate_resources_for_agent(
			agent_t & agent,
			const bind_params_t & params ) override
			{
				m_impl.preallocate_resources_for_agent( agent, params );
			}

		void
		undo_preallocation_for_agent(
			agent_t & agent ) noexcept override
			{
				m_impl.undo_preallocation_for_agent( agent );
			}

		event_queue_t *
		query_resources_for_agent( agent_t & agent ) noexcept override
			{
				return m_impl.query_resources_for_agent( agent );
			}

		void
		unbind_agent( agent_t & agent ) noexcept override
			{
				m_impl.unbind_agent( agent );
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

namespace
{

using namespace so_5::disp::nef_thread_pool::impl;

/*!
 * \brief Sets the thread count to default value if used do not
 * specify actual thread count.
 *
 * \since v.5.8.0
 */
inline void
adjust_thread_count( disp_params_t & params )
	{
		if( !params.thread_count() )
			params.thread_count( default_thread_pool_size() );
	}

} /* namespace anonymous */

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

		adjust_thread_count( params );

		using dispatcher_no_activity_tracking_t =
				impl::actual_dispatcher_implementation_t<
						impl::work_thread_no_activity_tracking_t<
								impl::dispatcher_queue_t
						>
				>;

		using dispatcher_with_activity_tracking_t =
				impl::actual_dispatcher_implementation_t<
						impl::work_thread_with_activity_tracking_t<
								impl::dispatcher_queue_t
						>
				>;

		auto binder = so_5::disp::reuse::make_actual_dispatcher<
						impl::actual_dispatcher_iface_t,
						dispatcher_no_activity_tracking_t,
						dispatcher_with_activity_tracking_t >(
				outliving_mutable(env),
				data_sources_name_base,
				std::move(params) );

		return impl::dispatcher_handle_maker_t::make( std::move(binder) );
	}

} /* namespace nef_thread_pool */

} /* namespace disp */

} /* namespace so_5 */


/*
 * SObjectizer-5
 */

/*!
 * \since
 * v.5.4.0
 *
 * \file
 * \brief Public interface of thread pool dispatcher.
 */

#include <so_5/disp/thread_pool/pub.hpp>

#include <so_5/disp/thread_pool/impl/disp.hpp>

#include <so_5/disp/reuse/make_actual_dispatcher.hpp>

#include <so_5/ret_code.hpp>

#include <so_5/disp_binder.hpp>
#include <so_5/environment.hpp>

namespace so_5
{

namespace disp
{

namespace thread_pool
{

namespace impl
{

//
// actual_dispatcher_iface_t
//
//FIXME: document this!
/*!
 * \brief An actual interface of thread-pool dispatcher.
 *
 * \since
 * v.5.6.0
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

//FIXME: document this!
//
// actual_binder_t
//
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
//FIXME: document this!
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
			:	m_impl{ name_base, params.thread_count(), params.queue_params() }
			{
				m_impl.start( env.get() );
			}

		~actual_dispatcher_implementation_t() noexcept override
			{
				m_impl.shutdown_then_wait();
			}

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

using namespace so_5::disp::thread_pool::impl;

/*!
 * \brief Sets the thread count to default value if used do not
 * specify actual thread count.
 *
 * \since
 * v.5.5.11
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
						impl::work_thread_no_activity_tracking_t >;

		using dispatcher_with_activity_tracking_t =
				impl::actual_dispatcher_implementation_t<
						impl::work_thread_with_activity_tracking_t >;

		auto binder = so_5::disp::reuse::make_actual_dispatcher<
						impl::actual_dispatcher_iface_t,
						dispatcher_no_activity_tracking_t,
						dispatcher_with_activity_tracking_t >(
				outliving_reference_t(env),
				data_sources_name_base,
				std::move(params) );

		return impl::dispatcher_handle_maker_t::make( std::move(binder) );
	}

} /* namespace thread_pool */

} /* namespace disp */

} /* namespace so_5 */


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

#include <so_5/disp/thread_pool/h/pub.hpp>

#include <so_5/disp/thread_pool/impl/h/disp.hpp>

#include <so_5/h/ret_code.hpp>

#include <so_5/rt/h/disp_binder.hpp>
#include <so_5/rt/h/environment.hpp>

#include <so_5/disp/reuse/h/disp_binder_helpers.hpp>
#include <so_5/disp/reuse/h/proxy_dispatcher_template.hpp>

#include <so_5/h/stdcpp.hpp>

namespace so_5
{

namespace disp
{

namespace thread_pool
{

namespace
{

using namespace so_5::disp::thread_pool::impl;

using actual_disp_iface_t =
		common_implementation::ext_dispatcher_iface_t< bind_params_t >;

//
// proxy_dispatcher_t
//

using proxy_dispatcher_base_t =
		so_5::disp::reuse::proxy_dispatcher_template_t<
				actual_disp_iface_t,
				disp_params_t >;

/*!
 * \brief A proxy dispatcher which creates actual dispatcher at start.
 *
 * \since
 * v.5.5.18
 *
 * This proxy is necessary because named dispatchers which are created
 * by create_disp() functions do not have a reference to SObjectizer
 * Environment at creation time. That reference is available in start()
 * method. Because of that creation of actual dispatcher (with or without
 * activity tracking) is delayed and performed only in start() method.
 */
class proxy_dispatcher_t : public proxy_dispatcher_base_t
	{
	public:
		proxy_dispatcher_t( disp_params_t params )
			:	proxy_dispatcher_base_t( std::move(params) )
			{}

		virtual event_queue_t *
		bind_agent( agent_ref_t agent, const bind_params_t & params ) override
			{
				return m_disp->bind_agent( agent, params );
			}

		virtual void
		unbind_agent( agent_ref_t agent ) override
			{
				return m_disp->unbind_agent( agent );
			}

	protected :
		virtual void
		do_actual_start( environment_t & env ) override
			{
				using dispatcher_no_activity_tracking_t =
						dispatcher_template_t< work_thread_no_activity_tracking_t >;

				using dispatcher_with_activity_tracking_t =
						dispatcher_template_t<
								work_thread_with_activity_tracking_t >;

				make_actual_dispatcher<
							dispatcher_no_activity_tracking_t,
							dispatcher_with_activity_tracking_t >(
						env,
						m_disp_params.thread_count(),
						m_disp_params.queue_params() );
			}
	};

//
// binding_actions_t
//
/*!
 * \brief A mixin with implementation of main binding/unbinding actions.
 *
 * \since
 * v.5.5.4
 */
class binding_actions_t
	{
	protected :
		binding_actions_t( bind_params_t params )
			:	m_params( std::move( params ) )
			{}

		disp_binding_activator_t
		do_bind(
			actual_disp_iface_t & disp,
			agent_ref_t agent )
			{
				auto queue = disp.bind_agent( agent, m_params );

				return [queue, agent]() {
						agent->so_bind_to_dispatcher( *queue );
					};
			}

		void
		do_unbind(
			actual_disp_iface_t & disp,
			agent_ref_t agent )
			{
				disp.unbind_agent( std::move( agent ) );
			}

	private :
		const bind_params_t m_params;
	};

//
// disp_binder_t
//
/*!
 * \brief An actual dispatcher binder for thread pool dispatcher.
 * \since
 * v.5.4.0
 */
using disp_binder_t = so_5::disp::reuse::binder_for_public_disp_template_t<
		proxy_dispatcher_t, binding_actions_t >;

//
// private_dispatcher_binder_t
//

/*!
 * \brief A binder for the private %thread_pool dispatcher.
 * \since
 * v.5.5.4
 */
using private_dispatcher_binder_t =
	so_5::disp::reuse::binder_for_private_disp_template_t<
		private_dispatcher_handle_t,
		proxy_dispatcher_t,
		binding_actions_t >;

//
// real_private_dispatcher_t
//
/*!
 * \brief A real implementation of private_dispatcher interface.
 *
 * \since
 * v.5.5.4
 */
class real_private_dispatcher_t : public private_dispatcher_t
	{
	public :
		/*!
		 * Constructor creates a dispatcher instance and launces it.
		 */
		real_private_dispatcher_t(
			//! SObjectizer Environment to work in.
			environment_t & env,
			//! Parameters for the dispatcher.
			disp_params_t params,
			//! Value for creating names of data sources for
			//! run-time monitoring.
			const std::string & data_sources_name_base )
			:	m_disp( so_5::stdcpp::make_unique< proxy_dispatcher_t >(
					std::move( params ) ) )
			{
				m_disp->set_data_sources_name_base( data_sources_name_base );
				m_disp->start( env );
			}

		/*!
		 * Destructors shuts an instance down and waits for it.
		 */
		~real_private_dispatcher_t() override
			{
				m_disp->shutdown();
				m_disp->wait();
			}

		virtual disp_binder_unique_ptr_t
		binder( const bind_params_t & params ) override
			{
				return so_5::stdcpp::make_unique< private_dispatcher_binder_t >(
						private_dispatcher_handle_t( this ),
						*m_disp,
						params );
			}

	private :
		std::unique_ptr< proxy_dispatcher_t > m_disp;
	};

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
// create_disp
//
SO_5_FUNC dispatcher_unique_ptr_t
create_disp(
	disp_params_t params )
	{
		adjust_thread_count( params );

		return so_5::stdcpp::make_unique< proxy_dispatcher_t >(
				std::move(params) );
	}

//
// create_private_disp
//
SO_5_FUNC private_dispatcher_handle_t
create_private_disp(
	//! SObjectizer Environment to work in.
	environment_t & env,
	//! Parameters for the dispatcher.
	disp_params_t params,
	//! Value for creating names of data sources for
	//! run-time monitoring.
	const std::string & data_sources_name_base )
	{
		adjust_thread_count( params );

		return private_dispatcher_handle_t{
				new real_private_dispatcher_t{
						env,
						std::move(params),
						data_sources_name_base } };
	}

//
// create_disp_binder
//
SO_5_FUNC disp_binder_unique_ptr_t
create_disp_binder(
	std::string disp_name,
	const bind_params_t & params )
	{
		return so_5::stdcpp::make_unique< disp_binder_t >(
				std::move( disp_name ), params );
	}

} /* namespace thread_pool */

} /* namespace disp */

} /* namespace so_5 */


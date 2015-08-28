/*
 * SObjectizer-5
 */

/*!
 * \since v.5.4.0
 * \file
 * \brief Public interface of thread pool dispatcher.
 */

#include <so_5/disp/thread_pool/h/pub.hpp>

#include <so_5/disp/thread_pool/impl/h/disp.hpp>

#include <so_5/h/ret_code.hpp>

#include <so_5/rt/h/disp_binder.hpp>
#include <so_5/rt/h/environment.hpp>

#include <so_5/disp/reuse/h/disp_binder_helpers.hpp>

namespace so_5
{

namespace disp
{

namespace thread_pool
{

//
// params_t
//
params_t::params_t()
	:	m_fifo( fifo_t::cooperation )
	,	m_max_demands_at_once( 4 )
	{}

params_t &
params_t::fifo( fifo_t v )
	{
		m_fifo = v;
		return *this;
	}

fifo_t
params_t::query_fifo() const
	{
		return m_fifo;
	}

params_t &
params_t::max_demands_at_once( std::size_t v )
	{
		m_max_demands_at_once = v;
		return *this;
	}

std::size_t
params_t::query_max_demands_at_once() const
	{
		return m_max_demands_at_once;
	}

namespace
{

using namespace so_5::rt;
using namespace so_5::disp::thread_pool::impl;

//
// binding_actions_t
//
/*!
 * \since v.5.5.4
 * \brief A mixin with implementation of main binding/unbinding actions.
 */
class binding_actions_t
	{
	protected :
		binding_actions_t( params_t params )
			:	m_params( std::move( params ) )
			{}

		so_5::rt::disp_binding_activator_t
		do_bind(
			dispatcher_t & disp,
			so_5::rt::agent_ref_t agent )
			{
				auto queue = disp.bind_agent( agent, m_params );

				return [queue, agent]() {
						agent->so_bind_to_dispatcher( *queue );
					};
			}

		void
		do_unbind(
			dispatcher_t & disp,
			agent_ref_t agent )
			{
				disp.unbind_agent( std::move( agent ) );
			}

	private :
		const params_t m_params;
	};

//
// disp_binder_t
//
/*!
 * \since v.5.4.0
 * \brief An actual dispatcher binder for thread pool dispatcher.
 */
using disp_binder_t = so_5::disp::reuse::binder_for_public_disp_template_t<
		dispatcher_t, binding_actions_t >;

//
// private_dispatcher_binder_t
//

/*!
 * \since v.5.5.4
 * \brief A binder for the private %thread_pool dispatcher.
 */
using private_dispatcher_binder_t =
	so_5::disp::reuse::binder_for_private_disp_template_t<
		private_dispatcher_handle_t,
		dispatcher_t,
		binding_actions_t >;

//
// real_private_dispatcher_t
//
/*!
 * \since v.5.5.4
 * \brief A real implementation of private_dispatcher interface.
 */
class real_private_dispatcher_t : public private_dispatcher_t
	{
	public :
		/*!
		 * Constructor creates a dispatcher instance and launces it.
		 */
		real_private_dispatcher_t(
			//! SObjectizer Environment to work in.
			so_5::rt::environment_t & env,
			//! Count of working threads.
			std::size_t thread_count,
			//! Value for creating names of data sources for
			//! run-time monitoring.
			const std::string & data_sources_name_base )
			:	m_disp( new dispatcher_t( thread_count ) )
			{
				m_disp->set_data_sources_name_base( data_sources_name_base );
				m_disp->start( env );
			}

		/*!
		 * Destructors shuts an instance down and waits for it.
		 */
		~real_private_dispatcher_t()
			{
				m_disp->shutdown();
				m_disp->wait();
			}

		virtual so_5::rt::disp_binder_unique_ptr_t
		binder( const params_t & params ) override
			{
				return so_5::rt::disp_binder_unique_ptr_t(
						new private_dispatcher_binder_t(
								private_dispatcher_handle_t( this ),
								*m_disp,
								params ) );
			}

	private :
		std::unique_ptr< dispatcher_t > m_disp;
	};

} /* namespace anonymous */

//
// private_dispatcher_t
//
private_dispatcher_t::~private_dispatcher_t()
	{}

//
// create_disp
//
SO_5_FUNC so_5::rt::dispatcher_unique_ptr_t
create_disp(
	std::size_t thread_count )
	{
		if( !thread_count )
			thread_count = default_thread_pool_size();

		return so_5::rt::dispatcher_unique_ptr_t(
				new impl::dispatcher_t( thread_count ) );
	}

//
// create_private_disp
//
SO_5_FUNC private_dispatcher_handle_t
create_private_disp(
	//! SObjectizer Environment to work in.
	so_5::rt::environment_t & env,
	//! Count of working threads.
	std::size_t thread_count,
	//! Value for creating names of data sources for
	//! run-time monitoring.
	const std::string & data_sources_name_base )
	{
		if( !thread_count )
			thread_count = default_thread_pool_size();

		return private_dispatcher_handle_t(
				new real_private_dispatcher_t(
						env,
						thread_count,
						data_sources_name_base ) );
	}

//
// create_disp_binder
//
SO_5_FUNC so_5::rt::disp_binder_unique_ptr_t
create_disp_binder(
	std::string disp_name,
	const params_t & params )
	{
		return so_5::rt::disp_binder_unique_ptr_t(
				new disp_binder_t( std::move( disp_name ), params ) );
	}

} /* namespace thread_pool */

} /* namespace disp */

} /* namespace so_5 */


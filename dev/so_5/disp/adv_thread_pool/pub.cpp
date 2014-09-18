/*
 * SObjectizer-5
 */

/*!
 * \since v.5.4.0
 * \file
 * \brief Public interface of advanced thread pool dispatcher.
 */

#include <so_5/disp/adv_thread_pool/h/pub.hpp>

#include <so_5/disp/adv_thread_pool/impl/h/disp.hpp>

#include <so_5/h/ret_code.hpp>

#include <so_5/rt/h/disp_binder.hpp>
#include <so_5/rt/h/so_environment.hpp>

#include <so_5/disp/reuse/h/disp_binder_helpers.hpp>

namespace so_5
{

namespace disp
{

namespace adv_thread_pool
{

//
// params_t
//
params_t::params_t()
	:	m_fifo( fifo_t::cooperation )
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

//
// create_disp
//
SO_5_EXPORT_FUNC_SPEC(so_5::rt::dispatcher_unique_ptr_t)
create_disp(
	std::size_t thread_count )
	{
		if( !thread_count )
			thread_count = default_thread_pool_size();

		return so_5::rt::dispatcher_unique_ptr_t(
				new impl::dispatcher_t( thread_count ) );
	}

namespace
{

using namespace so_5::rt;
using namespace so_5::disp::adv_thread_pool::impl;

//
// disp_binder_t
//
/*!
 * \since v.5.4.0
 * \brief An actual dispatcher binder for thread pool dispatcher.
 */
class disp_binder_t : public so_5::rt::disp_binder_t
	{
	public :
		//! Constructor.
		disp_binder_t(
			std::string disp_name,
			params_t params )
			:	m_disp_name( std::move( disp_name ) )
			,	m_params( std::move( params ) )
			{}

		virtual disp_binding_activator_t
		bind_agent(
			so_environment_t & env,
			agent_ref_t agent )
			{
				using namespace so_5::disp::reuse;

				return do_with_dispatcher< disp_binding_activator_t, dispatcher_t >(
					env,
					m_disp_name,
					[this, agent]( dispatcher_t & disp ) -> disp_binding_activator_t
					{
						auto queue = disp.bind_agent( agent, m_params );

						return [queue, agent]() {
								agent->so_bind_to_dispatcher( *queue );
							};
					} );
			}

		virtual void
		unbind_agent(
			so_environment_t & env,
			agent_ref_t agent )
			{
				using namespace so_5::disp::reuse;

				do_with_dispatcher< void, dispatcher_t >( env, m_disp_name,
					[this, agent]( dispatcher_t & disp )
					{
						disp.unbind_agent( std::move( agent ) );
					} );
			}

	private :
		//! Name of dispatcher to bind agents to.
		const std::string m_disp_name;

		//! Binding parameters.
		const params_t m_params;
	};

} /* namespace anonymous */

//
// create_disp_binder
//
SO_5_EXPORT_FUNC_SPEC(so_5::rt::disp_binder_unique_ptr_t)
create_disp_binder(
	std::string disp_name,
	const params_t & params )
	{
		return so_5::rt::disp_binder_unique_ptr_t(
				new disp_binder_t( std::move( disp_name ), params ) );
	}

} /* namespace adv_thread_pool */

} /* namespace disp */

} /* namespace so_5 */


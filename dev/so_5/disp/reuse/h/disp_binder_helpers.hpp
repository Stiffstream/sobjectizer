/*
 * SObjectizer-5
 */

/*!
 * \file
 * \brief Helpers for disp_binder implementation.
 * \since
 * v.5.4.0
 */

#pragma once

#include <so_5/h/ret_code.hpp>
#include <so_5/h/exception.hpp>

#include <so_5/rt/h/environment.hpp>

#include <functional>
#include <type_traits>
#include <utility>
#include <string>

namespace so_5
{

namespace disp
{

namespace reuse
{

/*!
 * \brief A helper method for casing dispatcher to the specified type
 * and performing some action with it.
 * \since
 * v.5.5.4
 */
template< class Dispatcher, class Action > 
auto
do_with_dispatcher_of_type(
	dispatcher_t * disp_pointer,
	const std::string & disp_name,
	Action action )
	-> decltype(action(*static_cast<Dispatcher *>(nullptr)))
	{
		// It should be our dispatcher.
		Dispatcher * disp = dynamic_cast< Dispatcher * >(
				disp_pointer );

		if( nullptr == disp )
			SO_5_THROW_EXCEPTION(
					rc_disp_type_mismatch,
					"type of dispatcher with name '" + disp_name +
					"' is not '" + typeid(Dispatcher).name() + "'" );

		return action( *disp );
	}

/*!
 * \brief A helper method for extracting dispatcher by name,
 * checking its type and to some action.
 * \since
 * v.5.4.0
 */
template< class Dispatcher, class Action > 
auto
do_with_dispatcher(
	environment_t & env,
	const std::string & disp_name,
	Action action )
	-> decltype(action(*static_cast<Dispatcher *>(nullptr)))
	{
		dispatcher_ref_t disp_ref = env.query_named_dispatcher(
				disp_name );

		// If the dispatcher is found then the agent should be bound to it.
		if( !disp_ref.get() )
			SO_5_THROW_EXCEPTION(
					rc_named_disp_not_found,
					"dispatcher with name '" + disp_name + "' not found" );

		return do_with_dispatcher_of_type< Dispatcher >(
				disp_ref.get(),
				disp_name,
				action );
	}

//
// binder_for_public_disp_template_t
//
/*!
 * \since
 * v.5.5.8
 * 
 * \brief A template of binder for a named dispatcher.
 *
 * \tparam Dispatcher type of a dispatcher.
 * \tparam Binder_Mixin type of a mixin with implementation of
 * do_bind and do_unbind methods.
 */
template<
	typename Dispatcher,
	typename Binder_Mixin >
class binder_for_public_disp_template_t
	:	public disp_binder_t
	,	protected Binder_Mixin
	{
	public:
		/*!
		 * \tparam Binder_Mixin_Args arguments for constructor of Binder_Mixin class.
		 */
		template< typename... Binder_Mixin_Args >
		binder_for_public_disp_template_t(
			std::string disp_name,
			Binder_Mixin_Args &&... args )
			:	Binder_Mixin( std::forward<Binder_Mixin_Args>(args)... )
			,	m_disp_name( std::move( disp_name ) )
			{}

		virtual disp_binding_activator_t
		bind_agent(
			environment_t & env,
			agent_ref_t agent ) override
			{
				return do_with_dispatcher< Dispatcher >(
					env,
					m_disp_name,
					[this, agent]( Dispatcher & disp )
					{
						return this->do_bind( disp, std::move( agent ) );
					} );
			}

		virtual void
		unbind_agent(
			environment_t & env,
			agent_ref_t agent ) override
			{
				using namespace so_5::disp::reuse;

				do_with_dispatcher< Dispatcher >( env, m_disp_name,
					[this, agent]( Dispatcher & disp )
					{
						this->do_unbind( disp, std::move( agent ) );
					} );
			}

	private:
		//! Name of the dispatcher to be bound to.
		const std::string m_disp_name;
	};

//
// binder_for_private_disp_template_t
//

/*!
 * \since
 * v.5.5.8
 *
 * \brief A template of binder for a private dispatcher.
 *
 * \tparam Handle type of a smart pointer to private dispatcher.
 * \tparam Dispatcher type of an actual dispatcher.
 * \tparam Binder_Mixin type of a mixin with implementation of
 * do_bind and do_unbind methods.
 */
template<
	typename Handle,
	typename Dispatcher,
	typename Binder_Mixin >
class binder_for_private_disp_template_t
	:	public disp_binder_t
	,	protected Binder_Mixin
	{
	public:
		/*!
		 * \tparam Binder_Mixin_Args arguments for constructor of Binder_Mixin class.
		 */
		template< typename... Binder_Mixin_Args >
		binder_for_private_disp_template_t(
			//! A handle for private dispatcher.
			//! It is necessary to manage lifetime of the dispatcher instance.
			Handle handle,
			//! A dispatcher instance to work with.
			Dispatcher & instance,
			//! Binding parameters for the agent.
			Binder_Mixin_Args &&... params )
			:	Binder_Mixin( std::forward< Binder_Mixin_Args >(params)... )
			,	m_handle( std::move( handle ) )
			,	m_instance( instance )
			{}

		virtual disp_binding_activator_t
		bind_agent(
			environment_t & /* env */,
			agent_ref_t agent ) override
			{
				return this->do_bind( m_instance, std::move( agent ) );
			}

		virtual void
		unbind_agent(
			environment_t & /* env */,
			agent_ref_t agent ) override
			{
				this->do_unbind( m_instance, std::move( agent ) );
			}

	private:
		//! A handle for private dispatcher.
		/*!
		 * It is necessary to manage lifetime of the dispatcher instance.
		 */
		Handle m_handle;
		//! A dispatcher instance to work with.
		Dispatcher & m_instance;
	};

} /* namespace reuse */

} /* namespace disp */

} /* namespace so_5 */



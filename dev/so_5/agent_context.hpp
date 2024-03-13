/*
 * SObjectizer-5
 */

/*!
 * \file
 * \brief A context for agent construction and tuning.
 *
 * \since v.5.5.4
 */

#pragma once

#include <so_5/fwd.hpp>

#include <so_5/agent_tuning_options.hpp>

namespace so_5
{

//
// agent_context_t
//
/*!
 * \brief A context for agent construction and tuning.
 *
 * \since v.5.5.4
 */
class agent_context_t
	{
	public :
		//! Constructor for the case when only environment available.
		agent_context_t(
			//! SObjectizer Environment an agent will work in.
			environment_t & env )
			:	m_env( &env )
			{}

		//! Constructor for the case when environment and already created
		//! tuning option object are avaliable.
		agent_context_t(
			//! SObjectizer Environment an agent will work in.
			environment_t & env,
			//! Options to be used for agent tuning.
			agent_tuning_options_t options )
			:	m_env( &env )
			,	m_options( std::move( options ) )
			{}

		//! Swap operation.
		friend inline void
		swap(
			so_5::agent_context_t & a,
			so_5::agent_context_t & b ) noexcept
			{
				using std::swap;
				swap( a.m_env, b.m_env );
				swap( a.m_options, b.m_options );
			}

		//! Access to SObjectizer Environment.
		environment_t &
		env() const
			{
				return *m_env;
			}

		//! Access to SObjectizer Environment.
		/*!
		 * \since v.5.5.9
		 */
		environment_t &
		environment() const
			{
				return *m_env;
			}

		//! Read-Write access to agent options.
		agent_tuning_options_t &
		options()
			{
				return m_options;
			}

		//! Read-only access to agent options.
		const agent_tuning_options_t &
		options() const
			{
				return m_options;
			}

	private :
		//! SObjectizer Environment to work in.
		environment_t * m_env;
		//! Options for agent tuning.
		agent_tuning_options_t m_options;
	};

/*!
 * \brief A plus operator for creating agent_context object from
 * a reference to Environment and single agent tuning option.
 *
 * \since v.5.5.4
 */
template< typename Option >
agent_context_t
operator+( environment_t & env, Option arg )
	{
		agent_context_t result{ env };
		return std::move(result) + std::move(arg);
	}

/*!
 * \name Operators for tuning agent_context.
 * \{
 */
inline agent_context_t
operator+(
	agent_context_t ctx,
	subscription_storage_factory_t factory )
	{
		ctx.options().subscription_storage_factory( std::move( factory ) );
		return ctx;
	}

template< class M >
agent_context_t
operator+(
	agent_context_t ctx,
	message_limit::drop_indicator_t< M > limit )
	{
		ctx.options().message_limits( limit );
		return ctx;
	}

template< class M >
agent_context_t
operator+(
	agent_context_t ctx,
	message_limit::abort_app_indicator_t< M > limit )
	{
		ctx.options().message_limits( limit );
		return ctx;
	}

template< class M, class L >
agent_context_t
operator+(
	agent_context_t ctx,
	message_limit::log_then_abort_app_indicator_t< M, L > limit )
	{
		ctx.options().message_limits( limit );
		return ctx;
	}

template< class M, class L >
agent_context_t
operator+(
	agent_context_t ctx,
	message_limit::redirect_indicator_t< M, L > limit )
	{
		ctx.options().message_limits( std::move( limit ) );
		return ctx;
	}

template< class M >
agent_context_t
operator+(
	agent_context_t ctx,
	message_limit::transform_indicator_t< M > limit )
	{
		ctx.options().message_limits( std::move( limit ) );
		return ctx;
	}

inline agent_context_t
operator+(
	agent_context_t ctx,
	so_5::priority_t agent_priority )
	{
		ctx.options().priority( agent_priority );
		return ctx;
	}

inline agent_context_t
operator+(
	agent_context_t ctx,
	custom_direct_mbox_factory_t factory )
	{
		ctx.options().custom_direct_mbox_factory( std::move(factory) );
		return ctx;
	}
/*!
 * \}
 */

} /* namespace so_5 */


/*
 * SObjectizer-5
 */

/*!
 * \since
 * v.5.5.4
 *
 * \file
 * \brief A context for agent construction and tuning.
 */

#pragma once

#include <so_5/rt/h/fwd.hpp>

#include <so_5/rt/h/agent_tuning_options.hpp>

namespace so_5
{

/*
 * NOTE: copy and move constructors and copy operator is implemented
 * because Visual C++ 12.0 (MSVS2013) doesn't generate it by itself.
 */
//
// agent_context_t
//
/*!
 * \since
 * v.5.5.4
 *
 * \brief A context for agent construction and tuning.
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

		//! Copy constructor.
		agent_context_t(
			const agent_context_t & ctx )
			:	m_env( ctx.m_env )
			,	m_options( ctx.m_options )
			{}

		//! Move constructor.
		agent_context_t(
			agent_context_t && ctx )
			:	m_env( ctx.m_env )
			,	m_options( std::move( ctx.m_options ) )
			{}

		//! Swap operation.
		void
		swap( agent_context_t & other )
			{
				std::swap( m_env, other.m_env );
				std::swap( m_options, other.m_options );
			}

		//! Copy/move operator.
		agent_context_t &
		operator=( agent_context_t ctx )
			{
				swap( ctx );
				return *this;
			}

		//! Access to SObjectizer Environment.
		environment_t &
		env() const
			{
				return *m_env;
			}

		//! Access to SObjectizer Environment.
		/*!
		 * \since
		 * v.5.5.9
		 *
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
 * \since
 * v.5.5.4
 *
 * \brief A plus operator for creating agent_context object from
 * a reference to Environment and single agent tuning option.
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
/*!
 * \}
 */

namespace rt
{

/*!
 * \deprecated Will be removed in v.5.6.0. Use so_5::adhoc_agent_wrapper_t
 * instead.
 */
using agent_context_t = so_5::agent_context_t;

} /* namespace rt */

} /* namespace so_5 */

namespace std
{

template<>
inline void
swap(
	so_5::agent_context_t & a,
	so_5::agent_context_t & b )
	{
		a.swap( b );
	}

} /* namespace std */



/*
	SObjectizer 5.
*/

/*!
 * \file
 * \since v.5.3.0
 * \brief Special agent class which would be used as wrapper for ad-hoc agents.
 */

#include <so_5/rt/h/adhoc_agent_wrapper.hpp>

namespace so_5
{

adhoc_agent_wrapper_t::adhoc_agent_wrapper_t( agent_context_t ctx )
	:	agent_t( std::move( ctx ) )
	,	m_exception_reaction( inherit_exception_reaction )
	{}

adhoc_agent_wrapper_t::~adhoc_agent_wrapper_t()
	{}

void
adhoc_agent_wrapper_t::set_on_evt_start( std::function< void() > handler )
	{
		m_on_start = handler;
	}

void
adhoc_agent_wrapper_t::set_on_evt_finish( std::function< void() > handler )
	{
		m_on_finish = handler;
	}

void
adhoc_agent_wrapper_t::set_exception_reaction(
	exception_reaction_t reaction )
	{
		m_exception_reaction = reaction;
	}

void
adhoc_agent_wrapper_t::so_evt_start()
	{
		if( m_on_start )
			m_on_start();
	}

void
adhoc_agent_wrapper_t::so_evt_finish()
	{
		if( m_on_finish )
			m_on_finish();
	}

exception_reaction_t
adhoc_agent_wrapper_t::so_exception_reaction() const
	{
		if( exception_reaction_t::inherit_exception_reaction !=
				m_exception_reaction )
			return m_exception_reaction;
		else
			// Let's basic implementation handle this case.
			return agent_t::so_exception_reaction();
	}

} /* namespace so_5 */

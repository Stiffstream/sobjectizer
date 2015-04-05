/*
 * SObjectizer-5
 */

/*!
 * \file
 * \since v.5.5.4
 * \brief Interfaces of data source and data sources repository.
 */

#include <so_5/rt/stats/h/repository.hpp>

namespace so_5
{

namespace rt
{

namespace stats
{

//
// source_t
//
source_t::source_t()
	:	m_prev{ nullptr }
	,	m_next{ nullptr }
	{}

source_t::~source_t()
	{}

//
// repository_t
//
repository_t::~repository_t()
	{}

void
repository_t::source_list_add(
	source_t & what,
	source_t *& head,
	source_t *& tail )
	{
		if( !tail )
			{
				// Addition the the empty list.
				what.m_prev = what.m_next = nullptr;
				head = &what;
			}
		else
			{
				tail->m_next = &what;
				what.m_prev = tail;
				what.m_next = nullptr;
			}

		tail = &what;
	}

void
repository_t::source_list_remove(
	source_t & what,
	source_t *& head,
	source_t *& tail )
	{
		if( what.m_prev )
			what.m_prev->m_next = what.m_next;
		else
			head = what.m_next;

		if( what.m_next )
			what.m_next->m_prev = what.m_prev;
		else
			tail = what.m_prev;
	}

source_t *
repository_t::source_list_next(
	const source_t & what )
	{
		return what.m_next;
	}

//
// auto_registered_source_t
//
auto_registered_source_t::auto_registered_source_t( repository_t & repo )
	:	m_repo( repo )
	{
		m_repo.add( *this );
	}

auto_registered_source_t::~auto_registered_source_t()
	{
		m_repo.remove( *this );
	}

//
// manually_registered_source_t
//
manually_registered_source_t::manually_registered_source_t()
	:	m_repo{ nullptr }
	{}

manually_registered_source_t::~manually_registered_source_t()
	{
		if( m_repo )
			stop();
	}

void
manually_registered_source_t::start( repository_t & repo )
	{
		repo.add( *this );
		m_repo = &repo;
	}

void
manually_registered_source_t::stop()
	{
		m_repo->remove( *this );
		m_repo = nullptr;
	}

} /* namespace stats */

} /* namespace rt */

} /* namespace so_5 */


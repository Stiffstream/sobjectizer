/*
 * SObjectizer-5
 */

/*!
 * \file
 * \since
 * v.5.5.4
 *
 * \brief Interfaces of data source and data sources repository.
 */

#include <so_5/stats/repository.hpp>

namespace so_5
{

namespace stats
{

//
// repository_t
//
void
repository_t::source_list_add(
	source_t & what,
	source_t *& head,
	source_t *& tail )
	{
		if( !tail )
			{
				// Addition to the empty list.
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
auto_registered_source_t::auto_registered_source_t(
	outliving_reference_t< repository_t > repo )
	:	m_repo( repo )
	{
		m_repo.get().add( *this );
	}

auto_registered_source_t::~auto_registered_source_t() SO_5_NOEXCEPT
	{
		m_repo.get().remove( *this );
	}

//
// manually_registered_source_t
//
manually_registered_source_t::manually_registered_source_t()
	:	m_repo{ nullptr }
	{}

manually_registered_source_t::~manually_registered_source_t() SO_5_NOEXCEPT
	{
		if( m_repo )
			stop();
	}

void
manually_registered_source_t::start(
	outliving_reference_t< repository_t > repo )
	{
		repo.get().add( *this );
		m_repo = &(repo.get());
	}

void
manually_registered_source_t::stop()
	{
		m_repo->remove( *this );
		m_repo = nullptr;
	}

} /* namespace stats */

} /* namespace so_5 */


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
	source_t *& tail ) noexcept
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
	source_t *& tail ) noexcept
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
	const source_t & what ) noexcept
	{
		return what.m_next;
	}

} /* namespace stats */

} /* namespace so_5 */


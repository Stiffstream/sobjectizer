/*
	SObjectizer 5.
*/

#include <so_5/h/atomic_refcounted.hpp>

namespace so_5
{

atomic_refcounted_t::atomic_refcounted_t() SO_5_NOEXCEPT
{
	m_ref_counter = 0l;
}

atomic_refcounted_t::~atomic_refcounted_t() SO_5_NOEXCEPT
{
}

} /* namespace so_5 */

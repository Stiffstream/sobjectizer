/*
	SObjectizer 5.
*/

#include <so_5/rt/h/atomic_refcounted.hpp>

namespace so_5
{

namespace rt
{

atomic_refcounted_t::atomic_refcounted_t()
{
	m_ref_counter = 0l;
}

atomic_refcounted_t::~atomic_refcounted_t()
{
}

} /* namespace rt */

} /* namespace so_5 */

/*
 * SObjectizer-5
 */

/*!
 * \since
 * v.5.5.3
 *
 * \file
 * \brief An interface of subscription storage.
 */

#include <so_5/impl/subscription_storage_iface.hpp>

namespace so_5
{

namespace impl
{

subscription_storage_t::subscription_storage_t( message_sink_t * owner )
	:	m_owner( owner )
	{}

message_sink_t *
subscription_storage_t::owner() const noexcept
	{
		return m_owner;
	}

} /* namespace impl */

} /* namespace so_5 */


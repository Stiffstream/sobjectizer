/*
	SObjectizer 5.
*/

/*!
	\file
	\brief Information about just one subscription.
	\since v.5.7.4
*/

#pragma once

#include <so_5/types.hpp>

#include <so_5/mbox.hpp>
#include <so_5/agent.hpp>
#include <so_5/enveloped_msg.hpp>

#include <so_5/impl/message_limit_internals.hpp>

namespace so_5
{

namespace impl
{

namespace local_mbox_details
{

/*!
 * \since
 * v.5.7.4
 *
 * \brief An information block about one subscription to one message type.
 */
class basic_subscription_info_t
{
protected:
	/*!
	 * \brief Current status of the subscription.
	 */
	enum class state_t
	{
		nothing,
		only_subscriptions,
		only_filter,
		subscriptions_and_filter
	};

	//! Optional message limit for that subscription.
	const so_5::message_limit::control_block_t * m_limit;

	/*!
	 * \brief Delivery filter for that message for that subscription.
	 */
	const delivery_filter_t * m_filter;

	/*!
	 * \brief Current state of the subscription parameters.
	 */
	state_t m_state;

public :
	//! Default constructor. Creates an empty object.
	basic_subscription_info_t()
		:	m_limit( nullptr )
		,	m_filter( nullptr )
		,	m_state( state_t::nothing )
	{}

	//! Constructor for the case when info is being
	//! created during event subscription.
	basic_subscription_info_t(
		//! Optional limit info. Can be nullptr if limits have not to be used at all
		//! (for example, for limitless MPSC mboxes).
		const so_5::message_limit::control_block_t * limit )
		:	m_limit( limit )
		,	m_filter( nullptr )
		,	m_state( state_t::only_subscriptions )
	{}

	//! Constructor for the case when subscriber info is being
	//! created during setting a delivery filter.
	basic_subscription_info_t(
		const delivery_filter_t * filter )
		:	m_limit( nullptr )
		,	m_filter( filter )
		,	m_state( state_t::only_filter )
	{}

	bool
	empty() const
	{
		return state_t::nothing == m_state;
	}

	const message_limit::control_block_t *
	limit() const
	{
		return m_limit;
	}

	//! Set the message limit for the subscriber.
	/*!
	 * Setting the message limit means that there are subscriptions
	 * for the agent.
	 *
	 * \note The message limit can be nullptr.
	 */
	void
	set_limit( const message_limit::control_block_t * limit )
	{
		m_limit = limit;

		m_state = ( state_t::nothing == m_state ?
				state_t::only_subscriptions :
				state_t::subscriptions_and_filter );
	}

	//! Drop the message limit for the subscriber.
	/*!
	 * Dropping the message limit means that there is no more
	 * subscription for the agent.
	 */
	void
	drop_limit()
	{
		m_limit = nullptr;

		m_state = ( state_t::only_subscriptions == m_state ?
				state_t::nothing : state_t::only_filter );
	}

	//! Set the delivery filter for the subscriber.
	void
	set_filter( const delivery_filter_t & filter )
	{
		m_filter = &filter;

		m_state = ( state_t::nothing == m_state ?
				state_t::only_filter :
				state_t::subscriptions_and_filter );
	}

	//! Drop the delivery filter for the subscriber.
	void
	drop_filter()
	{
		m_filter = nullptr;

		m_state = ( state_t::only_filter == m_state ?
				state_t::nothing : state_t::only_subscriptions );
	}

	//! Must a message be delivered to the subscriber?
	template< typename Msg_Ref_Extractor >
	delivery_possibility_t
	must_be_delivered(
		message_sink_t & subscriber,
		const message_ref_t & msg,
		Msg_Ref_Extractor msg_extractor ) const
	{
		// For the case when there are actual subscriptions.
		// We assume that will be in 99.9% cases.
		auto need_deliver = delivery_possibility_t::must_be_delivered;

		if( state_t::only_filter == m_state )
			// Only filter, no actual subscriptions.
			// No message delivery for that case.
			need_deliver = delivery_possibility_t::no_subscription;
		else if( state_t::subscriptions_and_filter == m_state )
		{
			// Delivery must be checked by delivery filter.
			// But message must be extracted from an envelope first.
			auto opt_msg = so_5::enveloped_msg::message_to_be_inspected( msg );
			if( opt_msg )
			{
				message_t & actual_msg = msg_extractor( *opt_msg );
				need_deliver = m_filter->check( subscriber, actual_msg ) ?
						delivery_possibility_t::must_be_delivered :
						delivery_possibility_t::disabled_by_delivery_filter;
			}
			else
				need_deliver = delivery_possibility_t::hidden_by_envelope;
		}

		return need_deliver;
	}
};

} /* namespace local_mbox_details */

} /* namespace impl */

} /* namespace so_5 */


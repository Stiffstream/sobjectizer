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

namespace so_5
{

namespace impl
{

namespace local_mbox_details
{

//FIXME: document this!
/*!
 * \brief An information block about one subscription to one message type.
 *
 * \since v.5.7.4
 */
class subscription_info_without_sink_t
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

	/*!
	 * \brief Delivery filter for that message for that subscription.
	 */
	const delivery_filter_t * m_filter;

	/*!
	 * \brief Current state of the subscription parameters.
	 */
	state_t m_state;

public :
	//FIXME: document this!
	struct subscription_present_t {};

	//! Default constructor. Creates an empty object.
	subscription_info_without_sink_t()
		:	m_filter( nullptr )
		,	m_state( state_t::nothing )
	{}

	//! Constructor for the case when info is being
	//! created during event subscription.
	subscription_info_without_sink_t(
		subscription_present_t /*subscription_presence*/ )
		:	m_filter( nullptr )
		,	m_state( state_t::only_subscriptions )
	{}

	//! Constructor for the case when subscriber info is being
	//! created during setting a delivery filter.
	subscription_info_without_sink_t(
		const delivery_filter_t * filter )
		:	m_filter( filter )
		,	m_state( state_t::only_filter )
	{}

	[[nodiscard]]
	bool
	empty() const
	{
		return state_t::nothing == m_state;
	}

	/*!
	 * \brief Inform about addition of a subscription.
	 */
	void
	subscription_defined()
	{
		m_state = ( state_t::nothing == m_state ?
				state_t::only_subscriptions :
				state_t::subscriptions_and_filter );
	}

	/*!
	 * \brief Inform about removal of a subscription.
	 */
	void
	subscription_dropped()
	{
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
	[[nodiscard]]
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

//FIXME: document this!
/*!
 * \brief An information block about one subscription to one message type
 * with presence of message_sink.
 *
 * \since v.5.7.4
 */
class subscription_info_with_sink_t
{
protected:
	/*!
	 * \brief Message sink for a subscriber.
	 *
	 * nullptr means that there is no subscriber.
	 */
	message_sink_t * m_sink;

	/*!
	 * \brief Delivery filter for that message for that subscription.
	 *
	 * nullptr means that there is no delivery filter.
	 */
	const delivery_filter_t * m_filter;

public :
	//! Default constructor. Creates an empty object.
	subscription_info_with_sink_t()
		:	m_sink{ nullptr }
		,	m_filter{ nullptr }
	{}

	//! Constructor for the case when info is being
	//! created during event subscription.
	subscription_info_with_sink_t(
		message_sink_t & sink )
		:	m_sink{ std::addressof(sink) }
		,	m_filter{ nullptr }
	{}

	//! Constructor for the case when subscriber info is being
	//! created during setting a delivery filter.
	subscription_info_with_sink_t(
		const delivery_filter_t & filter )
		:	m_sink{ nullptr }
		,	m_filter{ std::addressof(filter) }
	{}

	//! Constructor for the case when info is being
	//! created when subscriber and delivery_filter are known.
	subscription_info_with_sink_t(
		message_sink_t & sink,
		const delivery_filter_t & filter )
		:	m_sink{ std::addressof(sink) }
		,	m_filter{ std::addressof(filter) }
	{}

	[[nodiscard]]
	bool
	empty() const noexcept
	{
		return !m_sink && !m_filter;
	}

	/*!
	 * \brief Inform about addition of a subscription.
	 */
	void
	set_sink( message_sink_t & sink )
	{
		m_sink = std::addressof(sink);
	}

	/*!
	 * \brief Inform about removal of a subscription.
	 */
	void
	drop_sink()
	{
		m_sink = nullptr;
	}

	//! Set the delivery filter for the subscriber.
	void
	set_filter( const delivery_filter_t & filter )
	{
		m_filter = std::addressof(filter);
	}

	//! Drop the delivery filter for the subscriber.
	void
	drop_filter()
	{
		m_filter = nullptr;
	}

	//! Must a message be delivered to the subscriber?
	template< typename Msg_Ref_Extractor >
	[[nodiscard]]
	delivery_possibility_t
	must_be_delivered(
		const message_ref_t & msg,
		Msg_Ref_Extractor msg_extractor ) const
	{
		// For the case when there are actual subscriptions.
		// We assume that will be in 99.9% cases.
		auto need_deliver = delivery_possibility_t::must_be_delivered;

		if( !m_sink )
			// Only filter, no actual subscriptions.
			// No message delivery for that case.
			need_deliver = delivery_possibility_t::no_subscription;
		else if( m_filter )
		{
			// Delivery must be checked by delivery filter.
			// But message must be extracted from an envelope first.
			auto opt_msg = so_5::enveloped_msg::message_to_be_inspected( msg );
			if( opt_msg )
			{
				message_t & actual_msg = msg_extractor( *opt_msg );
				need_deliver = m_filter->check( *m_sink, actual_msg ) ?
						delivery_possibility_t::must_be_delivered :
						delivery_possibility_t::disabled_by_delivery_filter;
			}
			else
				need_deliver = delivery_possibility_t::hidden_by_envelope;
		}

		return need_deliver;
	}

	/*!
	 * \brief Get a reference to the subscribed sink.
	 *
	 * \attention
	 * It's UB to try to access a sink if sink isn't set.
	 * There is no checking for performance reasons.
	 */
	[[nodiscard]]
	message_sink_t &
	sink_reference() const noexcept
	{
		return *m_sink;
	}

	/*!
	 * \brief Get a pointer to the subscribed sink.
	 */
	[[nodiscard]]
	message_sink_t *
	sink_pointer() const noexcept
	{
		return m_sink;
	}
};

} /* namespace local_mbox_details */

} /* namespace impl */

} /* namespace so_5 */


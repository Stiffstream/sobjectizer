/*
 * SObjectizer-5
 */

/*!
 * \file
 * \brief Stuff for multi_sink_binding implementation.
 *
 * \since v.5.8.0
 */

#pragma once

#include <so_5/single_sink_binding.hpp>

#include <so_5/exception.hpp>
#include <so_5/ret_code.hpp>

#include <so_5/details/sync_helpers.hpp>

#include <map>

namespace so_5
{

namespace multi_sink_binding_impl
{

//FIXME: document this!
using one_sink_bindings_t = std::map< std::type_index, single_sink_binding_t >;

//FIXME: document this!
using one_mbox_bindings_t = std::map<
		msink_t,
		one_sink_bindings_t,
		so_5::impl::msink_less_comparator_t >;

//FIXME: document this!
using bindings_map_t = std::map< mbox_id_t, one_mbox_bindings_t >;

//FIXME: document this!
class actual_binding_handler_t
	{
		bindings_map_t m_bindings;

	public:
		template< typename Msg >
		void
		do_make_subscription(
			const mbox_t & from,
			const msink_t & dest,
			delivery_filter_unique_ptr_t opt_delivery_filter )
			{
				//FIXME: this implementation provides only basic exception
				//safety. It needs to be rewritten to provide strong exception
				//safety.
				auto it_mbox = m_bindings.find( from->id() );
				if( it_mbox == m_bindings.end() )
					it_mbox = m_bindings.emplace( from, one_mbox_bindings_t{} ).first;

				auto & msinks = it_mbox->second;
				auto it_msink = msinks.find( dest );
				if( it_msink == msinks.end() )
					it_msink = msinks.emplace( dest, one_sink_bindings_t{} ).first;

				const auto & msg_type =
						message_payload_type< Msg >::subscription_type_index();

				auto & msgs = it_msink->second;
				auto it_msg = msgs.find( msg_type );
				if( it_msg == msgs.end() )
					it_msg = msgs.emplace( msg_type, single_sink_binding_t{} ).first;
				else
					{
						SO_5_THROW_EXCEPTION(
								rc_evt_handler_already_provided,
								std::string{ "msink already subscribed to a message" } +
								"(mbox:'" + from->query_name() +
								"', msg_type:'" + msg_type.name() + "'" );
					}

				it_msg->second.template bind< Msg >(
						from,
						dest,
						std::move(opt_delivery_filter) );
			}

		template< typename Msg >
		void
		do_drop_subscription(
			const mbox_t & from,
			const msink_t & dest ) noexcept
		{
			auto it_mbox = m_bindings.find( from->id() );
			if( it_mbox == m_bindings.end() )
				return;

			auto & msinks = it_mbox->second;
			auto it_msink = msinks.find( dest );
			if( it_msink == msinks.end() )
				return;

			const auto & msg_type =
					message_payload_type< Msg >::subscription_type_index();

			auto & msgs = it_msink->second;
			auto it_msg = msgs.find( msg_type );
			if( it_msg == msgs.end() )
				return;

			msgs.erase( it_msg );

			if( msgs.empty() )
				{
					msinks.erase( it_msg );
					if( msinks.empty() )
						{
							m_bindings.erase( it_mbox );
						}
				}
		}

		void
		do_drop_all_subscriptions(
			const mbox_t & from,
			const msink_t & dest ) noexcept
		{
			auto it_mbox = m_bindings.find( from->id() );
			if( it_mbox == m_bindings.end() )
				return;

			auto & msinks = it_mbox->second;
			msinks.erase( dest );
			if( msinks.empty() )
				m_bindings.erase( it_mbox );
		}
	};

} /* namespace multi_sink_binding_impl */

template< typename Lock_Type = std::mutex >
class multi_sink_binding_t
	:	protected so_5::details::lock_holder_detector< Lock_Type >::type
	,	protected multi_sink_binding_impl::actual_binding_handler_t
	{
	public:
		multi_sink_binding_t() = default;

		multi_sink_binding_t( const multi_sink_binding_t & ) = delete;
		multi_sink_binding_t &
		operator=( const multi_sink_binding_t & ) = delete;

		multi_sink_binding_t( multi_sink_binding_t && ) = delete;
		multi_sink_binding_t &
		operator=( multi_sink_binding_t && ) = delete;

		//FIXME: maybe it's better to name it as `subscribe`?
		template< typename Msg >
		void
		make_subscription(
			const mbox_t & from,
			const msink_t & dest )
			{
				this->lock_and_perform( [&]() {
						this->template do_make_subscription< Msg >(
								from,
								dest,
								delivery_filter_unique_ptr_t{} );
					} );
			}

		//FIXME: maybe it's better to name it as `subscribe`?
		template< typename Msg >
		void
		make_subscription(
			const mbox_t & from,
			const msink_t & dest,
			delivery_filter_unique_ptr_t opt_delivery_filter )
			{
				//FIXME: can delivery_filter be null here?
				this->lock_and_perform( [&]() {
						this->template do_make_subscription< Msg >(
								from,
								dest,
								std::move(opt_delivery_filter) );
					} );
			}

		template< typename Msg >
		void
		drop_subscription(
			const mbox_t & from,
			const msink_t & dest ) noexcept
			{
				this->lock_and_perform( [&]() {
						this->template do_drop_subscription< Msg >( from, dest );
					} );
			}

		void
		drop_all_subscriptions(
			const mbox_t & from,
			const msink_t & dest ) noexcept
			{
				this->lock_and_perform( [&]() {
						this->do_drop_all_subscriptions( from, dest );
					} );
			}
	};

} /* namespace so_5 */


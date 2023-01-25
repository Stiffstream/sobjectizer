/*
 * SObjectizer-5
 */

/*!
 * \file
 * \brief Various stuff dedicated for single-threaded environments.
 *
 * \since
 * v.5.5.19
 */

#pragma once

#include <so_5/mbox.hpp>

#include <so_5/ret_code.hpp>
#include <so_5/outliving.hpp>

namespace so_5 {

namespace stats {

namespace impl {

namespace st_env_stuff {

//
// next_turn_handler_t
//
/*!
 * \brief An interface for initiation of next turn in stats distribution.
 *
 * \since
 * v.5.5.19
 */
class next_turn_handler_t
	{
	public :
		next_turn_handler_t() {}
		virtual ~next_turn_handler_t() {}

		virtual void
		on_next_turn(
			//! ID of stats distribution.
			int run_id ) = 0;

		struct next_turn : public message_t
			{
				//! Who must do next turn.
				outliving_reference_t< next_turn_handler_t > m_handler;
				//! ID of stats distribution.
				int m_run_id;

				next_turn(
					outliving_reference_t< next_turn_handler_t > handler,
					int run_id )
					:	m_handler( std::move(handler) )
					,	m_run_id( run_id )
					{}
			};
	};

//
// next_turn_mbox_t
//
/*!
 * \since A special implementation of abstract_message_box for handling
 * stats distribution in single-threaded environments.
 *
 * A call to next_turn_handler_t::on_next_turn is performed directly
 * in do_deliver_message() method. This is done in assumption that
 * do_deliver_message() is called on the context on the main environment's
 * thread.
 *
 * \since
 * v.5.5.19
 */
class next_turn_mbox_t final : public abstract_message_box_t
	{
		//! Environment for which that mbox is created.
		/*!
		 * \note
		 * This attribute is necessary for correct implementation of
		 * inherited environment() method.
		 *
		 * \since
		 * v.5.6.0
		 */
		environment_t & m_env;

		next_turn_mbox_t( environment_t & env ) : m_env{ env } {}

	public:
		// NOTE: this method should never be used.
		mbox_id_t
		id() const override
			{
				return 0;
			}

		void
		subscribe_event_handler(
			const std::type_index & /*type_index*/,
			const message_limit::control_block_t * /*limit*/,
			message_sink_t & /*subscriber*/ ) override
			{
				SO_5_THROW_EXCEPTION( rc_not_implemented,
						"call to subscribe_event_handler() is illegal for "
						"next_turn_mbox_t" );
			}

		void
		unsubscribe_event_handlers(
			const std::type_index & /*type_index*/,
			message_sink_t & /*subscriber*/ ) override
			{
				SO_5_THROW_EXCEPTION( rc_not_implemented,
						"call to unsubscribe_event_handler() is illegal for "
						"next_turn_mbox_t" );
			}

		std::string
		query_name() const override
			{
				return "<next_turn_mbox>";
			}

		mbox_type_t
		type() const override
			{
				return mbox_type_t::multi_producer_single_consumer;
			}

		void
		do_deliver_message(
			delivery_mode_t /*delivery_mode*/,
			const std::type_index & msg_type,
			const message_ref_t & message,
			unsigned int /*overlimit_reaction_deep*/ ) override
			{
				static const auto & next_turn_msg_type =
						typeid(next_turn_handler_t::next_turn);

				if( msg_type != next_turn_msg_type )
					SO_5_THROW_EXCEPTION( rc_unexpected_error,
							"only next_turn_handler_t::next_turn expected in "
							"next_turn_mbox_t::do_deliver_message" );

				const auto & actual_message =
						dynamic_cast< const next_turn_handler_t::next_turn & >(
								*message.get() );

				actual_message.m_handler.get().on_next_turn(
						actual_message.m_run_id );
			}

		void
		set_delivery_filter(
			const std::type_index & /*msg_type*/,
			const delivery_filter_t & /*filter*/,
			message_sink_t & /*subscriber*/ ) override
			{
				SO_5_THROW_EXCEPTION( rc_not_implemented,
						"call to set_delivery_filter() is illegal for "
						"next_turn_mbox_t" );
			}

		void
		drop_delivery_filter(
			const std::type_index & /*msg_type*/,
			message_sink_t & /*subscriber*/ ) noexcept override
			{
				SO_5_THROW_EXCEPTION( rc_not_implemented,
						"call to drop_delivery_filter() is illegal for "
						"next_turn_mbox_t" );
			}

		/*!
		 * \note
		 * It seems that this method should never be called.
		 * But it is safer to provide an actual implementation for it
		 * than relying on wrong assumption.
		 */
		environment_t &
		environment() const noexcept override
			{
				return m_env;
			}

		//! Helper for simplify creation of that mboxes of that type.
		static mbox_t
		make( environment_t & env )
			{
				return { new next_turn_mbox_t{env} };
			}
	};

} /* namespace st_env_stuff */

} /* namespace impl */

} /* namespace stats */

} /* namespace so_5 */


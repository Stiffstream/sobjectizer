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

#include <so_5/rt/h/mbox.hpp>

#include <so_5/h/ret_code.hpp>
#include <so_5/h/outliving.hpp>

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
	public:
		// NOTE: this method should never be used.
		virtual mbox_id_t
		id() const override
			{
				return 0;
			}

		virtual void
		subscribe_event_handler(
			const std::type_index & /*type_index*/,
			const message_limit::control_block_t * /*limit*/,
			agent_t * /*subscriber*/ ) override
			{
				SO_5_THROW_EXCEPTION( rc_not_implemented,
						"call to subscribe_event_handler() is illegal for "
						"next_turn_mbox_t" );
			}

		virtual void
		unsubscribe_event_handlers(
			const std::type_index & /*type_index*/,
			agent_t * /*subscriber*/ ) override
			{
				SO_5_THROW_EXCEPTION( rc_not_implemented,
						"call to unsubscribe_event_handler() is illegal for "
						"next_turn_mbox_t" );
			}

		virtual std::string
		query_name() const override
			{
				return "<next_turn_mbox>";
			}

		virtual mbox_type_t
		type() const override
			{
				return mbox_type_t::multi_producer_single_consumer;
			}

		virtual void
		do_deliver_message(
			const std::type_index & msg_type,
			const message_ref_t & message,
			unsigned int /*overlimit_reaction_deep*/ ) const override
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

		virtual void
		do_deliver_service_request(
			const std::type_index & /*msg_type*/,
			const message_ref_t & /*message*/,
			unsigned int /*overlimit_reaction_deep*/ ) const override
			{
				SO_5_THROW_EXCEPTION( rc_not_implemented,
						"call to do_deliver_service_request() is illegal for "
						"next_turn_mbox_t" );
			}

		virtual void
		set_delivery_filter(
			const std::type_index & /*msg_type*/,
			const delivery_filter_t & /*filter*/,
			agent_t & /*subscriber*/ ) override
			{
				SO_5_THROW_EXCEPTION( rc_not_implemented,
						"call to set_delivery_filter() is illegal for "
						"next_turn_mbox_t" );
			}

		virtual void
		drop_delivery_filter(
			const std::type_index & /*msg_type*/,
			agent_t & /*subscriber*/ ) SO_5_NOEXCEPT override
			{
				SO_5_THROW_EXCEPTION( rc_not_implemented,
						"call to drop_delivery_filter() is illegal for "
						"next_turn_mbox_t" );
			}

		//! Helper for simplify creation of that mboxes of that type.
		static mbox_t
		make()
			{
				return new next_turn_mbox_t();
			}
	};

} /* namespace st_env_stuff */

} /* namespace impl */

} /* namespace stats */

} /* namespace so_5 */


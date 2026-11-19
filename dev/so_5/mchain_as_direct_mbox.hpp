/*
 * SObjectizer-5
 */

/*!
 * \file
 * \brief Stuff related to mchain_as_direct_mbox functionality.
 *
 * \since v.5.8.7
 */

#if !defined( SO_5_MCHAIN_AS_DIRECT_MBOX_HPP )
#define SO_5_MCHAIN_AS_DIRECT_MBOX_HPP

#include <so_5/mbox.hpp>

namespace so_5::mchain_as_direct_mbox
{

namespace impl
{

/*!
 * \brief Special wrapper to be used for redirection of messages
 * from direct mbox to the specified mchain.
 *
 * \since v.5.8.7
 */
class redirecting_wrapper_t final
	: public so_5::abstract_message_box_t
	{
		//! Mbox ID to be used.
		const mbox_id_t m_id;

		//! The destination for redirected messages.
		const mchain_t m_chain;

		mbox_id_t
		id() const override
			{
				return m_id;
			}

		void
		subscribe_event_handler(
			const std::type_index & /*type_index*/,
			abstract_message_sink_t & /*subscriber*/ ) override
			{
				SO_5_THROW_EXCEPTION( rc_not_implemented,
						"subscribe_event_handler doesn't supported for "
						"mchain_as_direct_mbox::redirecting_wrapper_t" );
			}

		void
		unsubscribe_event_handler(
			const std::type_index & /*type_index*/,
			abstract_message_sink_t & /*subscriber*/ ) noexcept override
			{
				// Nothing to do.
			}

		std::string
		query_name() const override
			{
				std::ostringstream s;
				s << "<mbox:type=mchain_as_direct_mbox:id=" << m_id << ">";
				return s.str();
			}

		mbox_type_t
		type() const override
			{
				return mbox_type_t::multi_producer_single_consumer;
			}

		void
		do_deliver_message(
			message_delivery_mode_t delivery_mode,
			const std::type_index & msg_type,
			const message_ref_t & message,
			unsigned int redirection_deep ) override
			{
				m_chain->do_deliver_message(
						delivery_mode,
						msg_type,
						message,
						redirection_deep );
			}

		void
		set_delivery_filter(
			const std::type_index & /*msg_type*/,
			const delivery_filter_t & /*filter*/,
			abstract_message_sink_t & /*subscriber*/ ) override
			{
				SO_5_THROW_EXCEPTION( rc_not_implemented,
						"set_delivery_filter doesn't supported for "
						"mchain_as_direct_mbox::redirecting_wrapper_t" );
			}

		void
		drop_delivery_filter(
			const std::type_index & /*msg_type*/,
			abstract_message_sink_t & /*subscriber*/ ) noexcept
			{
				// Nothing to do.
			}

		so_5::environment_t &
		environment() const noexcept
			{
				return m_chain->environment();
			}

	public:
		//! Initializing constructor.
		redirecting_wrapper_t(
			//! Mbox ID to be used.
			mbox_id_t id,
			//! The destination for redirected messages.
			//! It's expected to be non-null.
			mchain_t chain )
			: m_id{ id }
			, m_chain{ std::move(chain) }
			{}

		//! Getter to obtain the target mchain.
		[[nodiscard]]
		mchain_t
		as_chain() const noexcept
			{
				return m_chain;
			}
	};

} /* namespace impl */

//
// use_mchain
//
/*!
 * \brief Helper function that creates a factory for custom direct mbox.
 *
 * This factory makes an instance of mbox that will redirect all incoming
 * messages (even messages without a subscription) to the @a chain.
 *
 * Usage example:
 * \code
 * class my_agent : public so_5::agent_t
 * {
 * public:
 * 	my_agent(context_t ctx)
 * 		: so_5::agent_t{ ctx + so_5::mchain_as_direct_mbox::use_mchain(
 * 			so_5::create_mchain(ctx.environment()) }
 * 	{...}
 *
 * 	...
 * };
 * \endcode
 *
 * \since v.5.8.7
 */
[[nodiscard]] inline
custom_direct_mbox_factory_t
use_mchain( mchain_t chain )
	{
		return [chain]( partially_constructed_agent_ptr_t, mbox_t orig ) -> mbox_t
			{
				return { new impl::redirecting_wrapper_t{ orig->id(), chain } };
			};
	}

//
// as_mchain
//
/*!
 * \brief Helper function for accessing underlying mchain from a custom
 * direct mbox.
 *
 * Usage example:
 * \code
 * class my_agent : public so_5::agent_t
 * {
 * public:
 * 	my_agent(context_t ctx)
 * 		: so_5::agent_t{ ctx
 * 			+ so_5::enable_cpp_coro()
 * 			+ so_5::mchain_as_direct_mbox::use_mchain(so_5::create_mchain(
 * 				ctx.environment()) }
 * 	{...}
 *
 * 	...
 *
 * 	so_5::cpp_coro::task_t<void>
 * 	so_evt_async_start() override
 * 	{
 * 		// Receive and handle all messages from mchain used as the direct mbox.
 * 		so_5::cpp_coro::select(
 * 			so_5::from_all().handle_all(),
 * 			so_5::receive_case(
 * 				// The mbox can't be passed to receive_case, so the underlying
 * 				// mchain has to be extracted from it.
 * 				so_5::mchain_as_direct_mbox::as_chain( so_direct_mbox() ),
 * 				... ) );
 * 	}
 * };
 * \endcode
 *
 * \throw std::exception if @a direct_mbox has non-appropriate type.
 *
 * \since v.5.8.7
 */
[[nodiscard]] inline
mchain_t
as_mchain( const so_5::mbox_t & direct_mbox )
	{
		return dynamic_cast< impl::redirecting_wrapper_t & >( *direct_mbox )
				.as_chain();
	}

} /* namespace so_5::mchain_as_direct_mbox */

#endif


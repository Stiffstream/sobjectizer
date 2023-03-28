/*
	SObjectizer 5.
*/

#include <so_5/mbox.hpp>

#include <so_5/ret_code.hpp>

#include <so_5/environment.hpp>

namespace so_5
{

//
// abstract_message_box_t
//

//
// wrap_to_msink
//

namespace as_msink_impl
{

//
// mbox_as_sink_t
//
/*!
 * \brief Implementation of abstract_message_sink for a case when
 * the destination is a mbox.
 *
 * \note
 * Because mbox has no priority, but message_sink should have it, the
 * priority has to be specified in the constructor.
 */
class mbox_as_sink_t final : public abstract_message_sink_t
	{
		//! The destination for messages.
		const mbox_t m_mbox;

		//! The priority for the sink.
		const priority_t m_sink_priority;

	public:
		//! Initializing constructor.
		mbox_as_sink_t(
			mbox_t mbox,
			priority_t priority )
			:	m_mbox{ std::move(mbox) }
			,	m_sink_priority{ priority }
			{}

		environment_t &
		environment() const noexcept override
			{
				return m_mbox->environment();
			}

		priority_t
		sink_priority() const noexcept override
			{
				return m_sink_priority;
			}

		void
		push_event(
			mbox_id_t /*mbox_id*/,
			message_delivery_mode_t delivery_mode,
			const std::type_index & msg_type,
			const message_ref_t & message,
			unsigned int redirection_deep,
			const message_limit::impl::action_msg_tracer_t * /*tracer*/ ) override
			{
				if( redirection_deep >= max_redirection_deep )
					{
						// NOTE: this fragment can throw but it isn't a problem
						// because transform_reaction() is called during message
						// delivery process and exceptions are expected in that
						// process.
						SO_5_LOG_ERROR(
								this->environment().error_logger(),
								logger )
							logger << "maximum message redirection deep exceeded on "
									"mbox_as_sink::push_event; message will be ignored;"
								<< " msg_type: " << msg_type.name()
								<< ", target_mbox: " << m_mbox->query_name();
					}
				else
					{
						m_mbox->do_deliver_message(
								delivery_mode,
								msg_type,
								message,
								// redirection_deep has to be increased.
								redirection_deep + 1u );
					}
			}
	};

} /* namespace as_msink_impl */

msink_t
wrap_to_msink(
	const mbox_t & mbox,
	so_5::priority_t sink_priority )
	{
		return msink_t{
				std::make_unique<
						simple_sink_owner_t< as_msink_impl::mbox_as_sink_t >
					>( mbox, sink_priority )
			};
	}

} /* namespace so_5 */


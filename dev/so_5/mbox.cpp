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
// as_msink
//

namespace as_msink_impl
{

//
// mbox_as_sink_t
//
//FIXME: document this!
class mbox_as_sink_t final : public abstract_message_sink_t
	{
		const mbox_t m_mbox;

	public:
		mbox_as_sink_t( mbox_t mbox ) : m_mbox{ std::move(mbox) }
			{}

		environment_t &
		environment() const noexcept override
			{
				return m_mbox->environment();
			}

		priority_t
		sink_priority() const noexcept override
			{
				//FIXME: this value has to be specified by a user.
				return prio::p0;
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
as_msink( const mbox_t & mbox )
	{
		return msink_t{
				std::make_unique<
						simple_sink_owner_t< as_msink_impl::mbox_as_sink_t >
					>( mbox )
			};
	}

} /* namespace so_5 */


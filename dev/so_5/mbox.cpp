/*
	SObjectizer 5.
*/

#include <so_5/mbox.hpp>

#include <so_5/ret_code.hpp>

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
			const std::type_index & msg_type,
			const message_ref_t & message,
			unsigned int overlimit_reaction_deep,
			const message_limit::impl::action_msg_tracer_t * /*tracer*/ ) override
			{
				//FIXME: should the value of overlimit_reaction_deep be checked here?

				m_mbox->do_deliver_message(
						//FIXME: where can we get the delivery_mode?
						message_delivery_mode_t::ordinary,
						msg_type,
						message,
						//FIXME: should we increment overlimit_reaction_deep here?
						overlimit_reaction_deep );
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


/*
 * Example of usage of mutable_msg with resending it as an immutable_msg.
 */

#include <iostream>

#include <so_5/all.hpp>

// A message which will be modified.
struct envelope final : public so_5::message_t
{
	// Payload to be modified.
	// Please note: it is not a const object.
	std::string m_payload;

	envelope(std::string payload) : m_payload(std::move(payload)) {}
};

// An agent which will mutate message.
class modificator_agent final : public so_5::agent_t
{
public :
	modificator_agent(context_t ctx,
		std::string prefix, std::string suffix,
		so_5::mbox_t receiver)
		:	so_5::agent_t(std::move(ctx))
		,	m_prefix(std::move(prefix))
		,	m_suffix(std::move(suffix))
		,	m_receiver(std::move(receiver))
	{
		so_subscribe_self().event( &modificator_agent::on_envelope );
	}

private :
	const std::string m_prefix;
	const std::string m_suffix;
	const so_5::mbox_t m_receiver;

	void on_envelope(mutable_mhood_t<envelope> cmd)
	{
		// Show message address and its old content.
		std::cout << "modificator, msg_addr=" << cmd.get()
				<< ", old_content=" << cmd->m_payload;

		// Modify message's payload.
		cmd->m_payload.insert(begin(cmd->m_payload), begin(m_prefix), end(m_prefix));
		cmd->m_payload.insert(end(cmd->m_payload), begin(m_suffix), end(m_suffix));
		std::cout << ", new_content=" << cmd->m_payload << std::endl;

		// Resend the message to receiver as an immutable message.
		so_5::send(m_receiver, to_immutable(std::move(cmd)));
	}
};

// Type of agent which will receive modified message.
class receiver_agent final : public so_5::agent_t
{
public :
	receiver_agent(context_t ctx) : so_5::agent_t(std::move(ctx))
	{
		so_subscribe_self().event( &receiver_agent::on_envelope );
	}

private :
	void on_envelope(mhood_t<envelope> cmd)
	{
		// Show message address and its resulting content.
		std::cout << "receiver, msg_addr=" << cmd.get()
				<< ", content=" << cmd->m_payload;

		so_deregister_agent_coop_normally();
	}
};

int main()
{
	try
	{
		so_5::launch( [](so_5::environment_t & env) {
			so_5::mbox_t modificator_mbox;
			// Create a coop with agents
			env.introduce_coop( [&](so_5::coop_t & coop) {
				auto receiver = coop.make_agent<receiver_agent>();
				auto modificator = coop.make_agent<modificator_agent>(
						"<{(", ")}>", receiver->so_direct_mbox() );
				modificator_mbox = modificator->so_direct_mbox();
			} );

			// Send mutable message with initial content.
			so_5::send<so_5::mutable_msg<envelope>>(modificator_mbox, "hello");
		} );
	}
	catch(const std::exception & x)
	{
		std::cerr << "Oops! Exception: " << x.what() << std::endl;
	}

	return 0;
}


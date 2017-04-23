/*
 * Example of usage of mutable_msg in agents chain.
 */

#include <iostream>

#include <so_5/all.hpp>

// A message which will be sent from one agent to another.
struct envelope final : public so_5::message_t
{
	// Payload to be modified by every agent in the chain.
	// Please note: it is not a const object.
	std::string m_payload;

	envelope(std::string payload) : m_payload(std::move(payload)) {}
};

// An agent which will be part of a chain.
// This agent will mutate a data from received message.
class chain_agent final : public so_5::agent_t
{
public :
	chain_agent(context_t ctx,
		std::string name,
		std::string prefix, std::string suffix,
		so_5::mbox_t next)
		:	so_5::agent_t(std::move(ctx))
		,	m_name(std::move(name))
		,	m_prefix(std::move(prefix))
		,	m_suffix(std::move(suffix))
		,	m_next(std::move(next))
	{
		so_subscribe_self().event( &chain_agent::on_envelope );
	}

private :
	const std::string m_name;
	const std::string m_prefix;
	const std::string m_suffix;
	const so_5::mbox_t m_next;

	void on_envelope(mutable_mhood_t<envelope> cmd)
	{
		// Show message address and its old content.
		std::cout << m_name << ", msg_addr=" << cmd.get()
				<< ", old_content=" << cmd->m_payload;

		// Modify message's payload.
		cmd->m_payload.insert(begin(cmd->m_payload), begin(m_prefix), end(m_prefix));
		cmd->m_payload.insert(end(cmd->m_payload), begin(m_suffix), end(m_suffix));
		std::cout << ", new_content=" << cmd->m_payload << std::endl;

		// Resend the message to next agent in chain.
		so_5::send(m_next, std::move(cmd));
	}
};

// Type of agent which will be the last agent in chain.
class last_agent final : public so_5::agent_t
{
public :
	last_agent(context_t ctx) : so_5::agent_t(std::move(ctx))
	{
		so_subscribe_self().event( &last_agent::on_envelope );
	}

private :
	void on_envelope(mutable_mhood_t<envelope> cmd)
	{
		// Show message address and its resulting content.
		std::cout << "last, msg_addr=" << cmd.get()
				<< ", content=" << cmd->m_payload;

		so_deregister_agent_coop_normally();
	}
};

int main()
{
	try
	{
		so_5::launch( [](so_5::environment_t & env) {
			so_5::mbox_t chain_head;
			// Create a coop with agents chain.
			env.introduce_coop( [&](so_5::coop_t & coop) {
				// Agents will be created from the last to the first.
				auto last = coop.make_agent<last_agent>();
				auto third = coop.make_agent<chain_agent>(
						"third", " [", "] ", last->so_direct_mbox() );
				auto second = coop.make_agent<chain_agent>(
						"second", " {", "} ", third->so_direct_mbox() );
				auto first = coop.make_agent<chain_agent>(
						"first", " (", ") ", second->so_direct_mbox() );
				chain_head = first->so_direct_mbox();
			} );

			// Send mutable message with initial content.
			so_5::send<so_5::mutable_msg<envelope>>(chain_head, "hello");
		} );
	}
	catch(const std::exception & x)
	{
		std::cerr << "Oops! Exception: " << x.what() << std::endl;
	}

	return 0;
}


/*
 * A unit-test for testing new create_child_coop and introduce_child_coop
 * functions added in v.5.5.8.
 */

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>

struct msg_started : public so_5::signal_t {};

class child_agent_t final : public so_5::agent_t
{
	const so_5::mbox_t m_owner;

public:
	child_agent_t( context_t ctx, so_5::mbox_t owner )
		:	so_5::agent_t{ std::move(ctx) }
		,	m_owner{ std::move(owner) }
	{}

	void so_evt_start() override
	{
		so_5::send< msg_started >( m_owner );
	}
};

class create_child_coop_agent_t final : public so_5::agent_t
{
	so_5::coop_t & m_parent;

public:
	create_child_coop_agent_t( context_t ctx, so_5::coop_t & parent )
		:	so_5::agent_t{ std::move(ctx) }
		,	m_parent{ parent }
	{
		so_subscribe_self().event( [this](mhood_t<msg_started>) {
				m_parent.deregister_normally();
			} );
	}

	void so_evt_start() override
	{
		auto child = so_5::create_child_coop( m_parent, so_5::autoname );
		child->make_agent< child_agent_t >( so_direct_mbox() );
		so_environment().register_coop( std::move(child) );
	}
};

class introduce_child_coop_agent_t final : public so_5::agent_t
{
	so_5::coop_t & m_parent;

public:
	introduce_child_coop_agent_t( context_t ctx, so_5::coop_t & parent )
		:	so_5::agent_t{ std::move(ctx) }
		,	m_parent{ parent }
	{
		so_subscribe_self().event( [this](mhood_t<msg_started>) {
				m_parent.deregister_normally();
			} );
	}

	void so_evt_start() override
	{
		so_5::introduce_child_coop( m_parent,
			[this]( so_5::coop_t & coop ) {
				coop.make_agent< child_agent_t >( so_direct_mbox() );
			} );
	}
};
void
init( so_5::environment_t & env )
{
	// First cooperation.
	env.introduce_coop( []( so_5::coop_t & parent ) {
			parent.make_agent< create_child_coop_agent_t >( std::ref(parent) );
		} );

	// Second cooperation.
	env.introduce_coop( []( so_5::coop_t & parent ) {
			parent.make_agent< introduce_child_coop_agent_t >( std::ref(parent) );
		} );
}

int
main()
{
	try
	{
		so_5::launch( init );

		return 0;
	}
	catch( const std::exception & x )
	{
		std::cerr << "Exception: " << x.what() << std::endl;
	}

	return 2;
}


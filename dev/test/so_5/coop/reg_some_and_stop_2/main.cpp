/*
 * Another test of registering and deregistering cooperations.
 *
 * Several cooperations are registered.
 * A message is sent to agents.
 * Part of cooperations are deregistered.
 * Another message is sent to agents.
 */

#include <iostream>
#include <map>
#include <exception>
#include <stdexcept>

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>

const char * g_test_mbox_name = "test_mbox";

struct test_message : public so_5::signal_t {};

class test_agent_t
	:
		public so_5::agent_t
{
		typedef so_5::agent_t base_type_t;

	public:

		test_agent_t(
			so_5::environment_t & env,
			const so_5::mbox_t & test_mbox )
			:
				base_type_t( env ),
				m_test_mbox( test_mbox )
		{
			++m_agent_count;
		}

		~test_agent_t() override
		{
			--m_agent_count;
		}

		void
		so_define_agent() override;

		void
		evt_test(
			mhood_t< test_message > msg );

		// Count of life agents.
		static so_5::atomic_counter_t m_agent_count;

		// Count of evt_test calls.
		static so_5::atomic_counter_t m_message_rec_cnt;

	private:
		so_5::mbox_t m_test_mbox;

};

so_5::atomic_counter_t test_agent_t::m_agent_count;
so_5::atomic_counter_t test_agent_t::m_message_rec_cnt;

void
test_agent_t::so_define_agent()
{
	so_subscribe( m_test_mbox )
		.in( so_default_state() )
			.event( &test_agent_t::evt_test );
}

void
test_agent_t::evt_test(
	mhood_t< test_message > )
{
	++m_message_rec_cnt;
}

auto
reg_coop(
	const so_5::mbox_t & test_mbox,
	so_5::environment_t & env )
{
	auto coop = env.make_coop();

	coop->make_agent< test_agent_t >( test_mbox );
	coop->make_agent< test_agent_t >( test_mbox );

	return env.register_coop( std::move( coop ) );
}

void
init( so_5::environment_t & env )
{
	so_5::mbox_t test_mbox =
		env.create_mbox( g_test_mbox_name );

	auto coop_1 = reg_coop( test_mbox, env );
	reg_coop( test_mbox, env );
	auto coop_3 = reg_coop( test_mbox, env );
	reg_coop( test_mbox, env );
	reg_coop( test_mbox, env );
	auto coop_6 = reg_coop( test_mbox, env );

	// Give time to subscription.
	std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );

	// Initiate message.
	so_5::send< test_message >( env.create_mbox( g_test_mbox_name ) );

	// Give time to process message.
	std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );

	auto x = test_agent_t::m_agent_count.load();
	x -= test_agent_t::m_message_rec_cnt;
	if( 0 != x )
		throw std::runtime_error(
			"check 1: test_agent_t::m_agent_count != "
			"test_agent_t::m_message_rec_cnt" );

	// Deregister some cooperations.
	env.deregister_coop( coop_1, so_5::dereg_reason::normal );

	env.deregister_coop( coop_6, so_5::dereg_reason::normal );

	env.deregister_coop( coop_3, so_5::dereg_reason::normal );

	test_agent_t::m_message_rec_cnt = 0;

	// Give time to finish deregistration.
	std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );

	// Send another message.
	so_5::send< test_message >( env.create_mbox( g_test_mbox_name ) );

	// Give time to process message.
	std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );

	x = test_agent_t::m_agent_count;
	x -= test_agent_t::m_message_rec_cnt;
	if( 0 != x )
		throw std::runtime_error(
			"check 2: test_agent_t::m_agent_count != "
			"test_agent_t::m_message_rec_cnt" );

	env.stop();

}

int
main()
{
	run_with_time_limit( [] {
			so_5::launch( &init );
		},
		10 );

	return 0;
}

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>

namespace test
{

class a_demo_t final : public so_5::agent_t
{
	struct msg_handle_this final : public so_5::message_t
	{
		int m_ordinal;

		msg_handle_this( int ordinal ) : m_ordinal{ ordinal } {}
	};

	std::atomic< int > m_handlers_at_same_time{ 0 };

public:
	a_demo_t( context_t ctx )
		: so_5::agent_t{ ctx + so_5::enable_cpp_coro() }
	{}

	void
	so_define_agent() override
	{
		so_subscribe_self()
			.event( &a_demo_t::evt_handle_this, so_5::thread_safe )
			;
	}

	void
	so_evt_start() override
	{
		so_5::send< msg_handle_this >( *this, 0 );
		so_5::send< msg_handle_this >( *this, 1 );
		so_5::send< msg_handle_this >( *this, 2 );
	}

private:
	void
	evt_handle_this( mhood_t< msg_handle_this > cmd )
	{
		m_handlers_at_same_time += 1;

		if( 1 < m_handlers_at_same_time )
			throw std::runtime_error{ "Several handlers at the same time!" };

		std::this_thread::sleep_for( std::chrono::milliseconds{ 50 } );

		m_handlers_at_same_time -= 1;

		if( 2 == cmd->m_ordinal )
			so_deregister_agent_coop_normally();
	}
};

void
run_demo()
{
	so_5::launch( []( so_5::environment_t & env ) {
			env.register_agent_as_coop(
					env.make_agent< a_demo_t >(),
					so_5::disp::adv_thread_pool::make_dispatcher(
							env,
							4u ).binder() );
		} );
}

} /* namespace test */

using namespace test;

int
main()
{
	try
	{
		run_with_time_limit(
			[]() {
				run_demo();
			},
			5 );
	}
	catch(const std::exception & ex)
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}


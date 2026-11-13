#include <so_5/all.hpp>

#include "../../dummy_yield.hpp"

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>
#include <test/3rd_party/various_helpers/ensure.hpp>

using namespace std::chrono_literals;

namespace test
{

struct basic_data : public so_5::message_t
	{
		std::string m_data;

		basic_data( std::string data ) : m_data{ std::move(data) }
			{}
	};

struct msg_001 final : public basic_data
	{
		using basic_data::basic_data;
	};

struct msg_002 final : public basic_data
	{
		using basic_data::basic_data;
	};

struct msg_003 final : public basic_data
	{
		using basic_data::basic_data;
	};

struct msg_004 final : public basic_data
	{
		using basic_data::basic_data;
	};

struct msg_005 final : public basic_data
	{
		using basic_data::basic_data;
	};

struct msg_006 final : public basic_data
	{
		using basic_data::basic_data;
	};

struct msg_007 final : public basic_data
	{
		using basic_data::basic_data;
	};

struct msg_008 final : public basic_data
	{
		using basic_data::basic_data;
	};

class a_demo_t final : public so_5::agent_t
{
	struct msg_done final : public so_5::signal_t {};

public:
	a_demo_t( context_t ctx )
		: so_5::agent_t{ ctx + so_5::enable_cpp_coro() }
	{}

	void
	so_define_agent() override
	{
		so_subscribe_self()
			.event( &a_demo_t::evt_done )
			.event( &a_demo_t::evt_001 )
			.event( &a_demo_t::evt_002 )
			.event( &a_demo_t::evt_003 )
			.event( &a_demo_t::evt_004 )
			.event( &a_demo_t::evt_005 )
			.event( &a_demo_t::evt_006 )
			.event( &a_demo_t::evt_007 )
			.event( &a_demo_t::evt_008 )
			;
	}

	void
	so_evt_start() override
	{
		so_5::send< msg_001 >( *this,
				"rather-long-string-to-overflow-SSO-buffer-001" );
	}

	void
	so_evt_finish() override
	{
		std::cout << "evt_finish" << std::endl;
	}

private:
	void
	evt_done( mhood_t< msg_done > )
	{
		so_deregister_agent_coop_normally();
	}

	so_5::cpp_coro::task_t< void >
	evt_001( mhood_t< msg_001 > cmd )
	{
		const std::string copy = cmd->m_data;
		std::cout << cmd->m_data << std::endl;

		co_await dummy_yield( so_environment(), 20ms );

		ensure_or_die( copy == cmd->m_data, "copy should match source data" );

		so_5::send< msg_002 >( *this, cmd->m_data + "-002" );
	}

	so_5::cpp_coro::task_t< void >
	evt_002( const mhood_t< msg_002 > & cmd )
	{
		const std::string copy = cmd->m_data;
		std::cout << cmd->m_data << std::endl;

		co_await dummy_yield( so_environment(), 20ms );

		ensure_or_die( copy == cmd->m_data, "copy should match source data" );

		so_5::send< msg_003 >( *this, cmd->m_data + "-003" );
	}

	so_5::cpp_coro::task_t< void >
	evt_003( const msg_003 & cmd )
	{
		const std::string copy = cmd.m_data;
		std::cout << cmd.m_data << std::endl;

		co_await dummy_yield( so_environment(), 20ms );

		ensure_or_die( copy == cmd.m_data, "copy should match source data" );

		so_5::send< msg_004 >( *this, cmd.m_data + "-004" );
	}

	so_5::cpp_coro::task_t< void >
	evt_004( msg_004 cmd )
	{
		const std::string copy = cmd.m_data;
		std::cout << cmd.m_data << std::endl;

		co_await dummy_yield( so_environment(), 20ms );

		ensure_or_die( copy == cmd.m_data, "copy should match source data" );

		so_5::send< msg_005 >( *this, cmd.m_data + "-005" );
	}

	so_5::cpp_coro::task_t< void >
	evt_005( mhood_t< msg_005 > cmd ) const
	{
		const std::string copy = cmd->m_data;
		std::cout << cmd->m_data << std::endl;

		co_await dummy_yield( so_environment(), 20ms );

		ensure_or_die( copy == cmd->m_data, "copy should match source data" );

		so_5::send< msg_006 >( *this, cmd->m_data + "-006" );
	}

	so_5::cpp_coro::task_t< void >
	evt_006( const mhood_t< msg_006 > & cmd ) const
	{
		const std::string copy = cmd->m_data;
		std::cout << cmd->m_data << std::endl;

		co_await dummy_yield( so_environment(), 20ms );

		ensure_or_die( copy == cmd->m_data, "copy should match source data" );

		so_5::send< msg_007 >( *this, cmd->m_data + "-007" );
	}

	so_5::cpp_coro::task_t< void >
	evt_007( const msg_007 & cmd ) const
	{
		const std::string copy = cmd.m_data;
		std::cout << cmd.m_data << std::endl;

		co_await dummy_yield( so_environment(), 20ms );

		ensure_or_die( copy == cmd.m_data, "copy should match source data" );

		so_5::send< msg_008 >( *this, cmd.m_data + "-008" );
	}

	so_5::cpp_coro::task_t< void >
	evt_008( msg_008 cmd ) const
	{
		const std::string copy = cmd.m_data;
		std::cout << cmd.m_data << std::endl;

		co_await dummy_yield( so_environment(), 20ms );

		ensure_or_die( copy == cmd.m_data, "copy should match source data" );

		so_5::send< msg_done >( *this );
	}
};

void
run_demo()
{
	so_5::launch( []( so_5::environment_t & env ) {
			env.register_agent_as_coop(
					env.make_agent< a_demo_t >() );
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


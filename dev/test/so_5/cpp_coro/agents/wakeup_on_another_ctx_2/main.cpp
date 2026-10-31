#include <so_5/all.hpp>

#include <so_5/cpp_coro/mchain_select.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>
#include <test/3rd_party/various_helpers/ensure.hpp>

#include <syncstream>

namespace test
{

struct msg_take_ctx final : public so_5::message_t
{
	so_5::cpp_coro::scheduler_t & m_scheduler;

	msg_take_ctx( so_5::cpp_coro::scheduler_t & scheduler )
		: m_scheduler{ scheduler }
	{}
};

class a_ctx_provider_t final : public so_5::agent_t
{
	const so_5::mbox_t m_dest;

public:
	a_ctx_provider_t( context_t ctx, so_5::mbox_t dest )
		: so_5::agent_t{ ctx + so_5::enable_cpp_coro() }
		, m_dest{ std::move(dest) }
	{}

	void
	so_evt_start() override
	{
		so_5::send< msg_take_ctx >( m_dest, so_this_agent_scheduler() );
	}
};

class a_consumer_t final : public so_5::agent_t
{
	struct msg_handle_it final : public so_5::signal_t {};

public:
	a_consumer_t( context_t ctx )
		: so_5::agent_t{ ctx + so_5::enable_cpp_coro() }
	{}

	void
	so_define_agent() override
	{
		so_subscribe_self().event( &a_consumer_t::evt_take_ctx );
	}

private:
	so_5::cpp_coro::task_t< void >
	evt_take_ctx( mhood_t<msg_take_ctx> cmd )
	{
		const auto this_thread_id = std::this_thread::get_id();
		std::osyncstream(std::cout) << this_thread_id << " | start" << std::endl;

		auto ch = so_5::create_mchain( so_environment() );

		so_5::send_delayed< msg_handle_it >( ch, std::chrono::milliseconds{ 50 } );

		co_await so_5::cpp_coro::select(
				so_5::from_all().handle_n(1),
				so_5::receive_case( ch,
					[]( mhood_t<msg_handle_it> ) {
						std::osyncstream(std::cout) << std::this_thread::get_id()
								<< " | msg_handle_it received" << std::endl;
					} ) );

		const auto this_thread_id_2 = std::this_thread::get_id();
		std::osyncstream(std::cout) << this_thread_id_2 << " | resume after select"
				<< std::endl;

		if( this_thread_id != this_thread_id_2 )
		{
			std::osyncstream(std::cout) << this_thread_id_2 << " | throwing"
					<< std::endl;
			throw std::runtime_error{ "work resumed on a different thread" };
		}
	}
};

void
run_demo()
{
	so_5::launch( []( so_5::environment_t & env ) {
			env.introduce_coop(
					so_5::disp::active_obj::make_dispatcher( env ).binder(),
					[]( so_5::coop_t & coop ) {
						auto consumer = coop.make_agent< a_consumer_t >();
						coop.make_agent< a_ctx_provider_t >(
								consumer->so_direct_mbox() );
					} );
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


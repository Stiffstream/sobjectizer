/*
 * A test for parallel shutdown of SOEnv in presence of stop guards.
 */

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>
#include <test/3rd_party/various_helpers/ensure.hpp>

#include <test/3rd_party/utest_helper/helper.hpp>

using namespace std;

namespace test
{

// Test stop_guard.
// Its goal is to suspend the current thread in stop() for some time.
class my_stop_guard_t final
	: public so_5::stop_guard_t
	, public std::enable_shared_from_this<my_stop_guard_t>
{
	std::atomic<int> m_counter;

public :
	my_stop_guard_t() = default;

	void
	stop() noexcept override
	{
		++m_counter;

		// Suspend the current thread.
		std::cout << "my_stop_guard_t::stop suspending the current thread..." << std::endl;
		std::this_thread::sleep_for( std::chrono::milliseconds{ 500 } );
		std::cout << "my_stop_guard_t::stop resuming the current thread..." << std::endl;
	}

	[[nodiscard]] int
	counter() const noexcept
	{
		return m_counter.load( std::memory_order_acquire );
	}
};

class worker_t final : public so_5::agent_t
{
public :
	struct worker_started final : public so_5::signal_t {};
	struct start_shutdown final : public so_5::signal_t {};
	struct stop_called final : public so_5::signal_t {};

	worker_t(
		context_t ctx,
		so_5::mbox_t notify_mbox )
		:	so_5::agent_t( std::move(ctx) )
		,	m_notify_mbox( std::move(notify_mbox) )
	{}

	void
	so_define_agent() override
	{
		so_subscribe( m_notify_mbox ).event( &worker_t::evt_start_shutdown );
	}

	void
	so_evt_start() override
	{
		so_5::send< worker_started >( m_notify_mbox );
	}

private :
	const so_5::mbox_t m_notify_mbox;

	void
	evt_start_shutdown( mhood_t< start_shutdown > )
	{
		so_environment().stop();
		so_5::send< stop_called >( m_notify_mbox );
	}
};

class coordinator_t final : public so_5::agent_t
{
public:
	coordinator_t(
		context_t ctx,
		std::size_t total_workers,
		so_5::mbox_t notify_mbox,
		std::shared_ptr< my_stop_guard_t > stop_guard )
		:	so_5::agent_t{ std::move(ctx) }
		,	m_total_workers{ total_workers }
		,	m_notify_mbox{ std::move(notify_mbox) }
		,	m_stop_guard{ std::move(stop_guard) }
	{
	}

	void
	so_define_agent() override
	{
		so_subscribe( m_notify_mbox )
			.event( &coordinator_t::evt_worker_started )
			.event( &coordinator_t::evt_stop_called )
			;
	}

	void
	so_evt_start()
	{
		so_environment().setup_stop_guard( m_stop_guard );
	}

private:
	const std::size_t m_total_workers;
	const so_5::mbox_t m_notify_mbox;

	const std::shared_ptr< my_stop_guard_t > m_stop_guard;

	std::size_t m_workers_started{};
	std::size_t m_workers_stopped{};

	void
	evt_worker_started( mhood_t< worker_t::worker_started > )
	{
		++m_workers_started;
		if( m_total_workers == m_workers_started )
			so_5::send< worker_t::start_shutdown >( m_notify_mbox );
	}

	void
	evt_stop_called( mhood_t< worker_t::stop_called > cmd )
	{
		++m_workers_stopped;
		if( m_total_workers == m_workers_stopped )
			so_environment().remove_stop_guard( m_stop_guard );
	}
};

void do_test()
{
	auto sg_1 = std::make_shared< my_stop_guard_t >();

	so_5::launch( [&]( so_5::environment_t & env ) {
			auto notify_mbox = env.create_mbox();

			env.introduce_coop(
				so_5::disp::active_obj::make_dispatcher( env ).binder(),
				[&]( so_5::coop_t & coop ) {
					coop.make_agent< coordinator_t >( 3u, notify_mbox, sg_1 );

					coop.make_agent< worker_t >( notify_mbox );
					coop.make_agent< worker_t >( notify_mbox );
					coop.make_agent< worker_t >( notify_mbox );
				} );
		},
		[]( so_5::environment_params_t & /*params*/ )
		{
//			params.message_delivery_tracer( so_5::msg_tracing::std_cout_tracer() );
		} );

	ensure_or_die( 1 == sg_1->counter(), "invalid value for sg_1" );
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
				do_test();
			},
			5 );
	}
	catch(const exception & ex)
	{
		cerr << "Error: " << ex.what() << endl;
		return 1;
	}

	return 0;
}


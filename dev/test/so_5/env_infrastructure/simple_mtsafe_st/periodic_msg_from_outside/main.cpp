/*
 * A test for simple_mtsafe_st_env_infastructure with one simple agent
 * and periodic message.
 */

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>

#include <test/3rd_party/utest_helper/helper.hpp>

using namespace std;

namespace test
{

class timer_holder_t
{
	std::mutex m_lock;
	so_5::timer_id_t m_timer;

public:
	void
	store( so_5::timer_id_t timer )
	{
		std::lock_guard< std::mutex > lock{ m_lock };
		m_timer = std::move(timer);
	}

	void
	release()
	{
		std::lock_guard< std::mutex > lock{ m_lock };
		m_timer.release();
	}
};

class a_test_t final : public so_5::agent_t
{
	timer_holder_t & m_timer_to_drop;
	int m_ticks{ 0 };

public :
	struct tick : public so_5::signal_t {};

	a_test_t(
		context_t ctx,
		timer_holder_t & timer_to_drop )
		: so_5::agent_t( std::move(ctx) )
		, m_timer_to_drop{ timer_to_drop }
	{
		so_subscribe_self().event( [this](mhood_t< tick >) {
				++m_ticks;
				if( 3 == m_ticks )
					so_deregister_agent_coop_normally();
			} );
	}

	~a_test_t() override
	{
		m_timer_to_drop.release();
	}
};

} /* namespace test */

using namespace test;

int
main()
{
	try
	{
		run_with_time_limit(
			[]() {
				thread outside_thread;
				timer_holder_t timer_id;

				so_5::launch(
					[&]( so_5::environment_t & env ) {
						so_5::mbox_t test_mbox = env.introduce_coop(
							[&timer_id]( so_5::coop_t & coop ) {
								return coop.make_agent< a_test_t >( timer_id )
										->so_direct_mbox();
							} );

						outside_thread = thread( [test_mbox, &timer_id] {
							this_thread::sleep_for( chrono::milliseconds( 350 ) );
							timer_id.store( so_5::send_periodic< a_test_t::tick >(
									test_mbox,
									chrono::milliseconds(100),
									chrono::milliseconds(100) ) );
							this_thread::sleep_for( chrono::seconds(1) );
						} );
					},
					[]( so_5::environment_params_t & params ) {
						params.infrastructure_factory(
								so_5::env_infrastructures::simple_mtsafe::factory() );
					} );

				outside_thread.join();
			},
			5,
			"simple agent with periodic message from outside" );
	}
	catch( const exception & ex )
	{
		cerr << "Error: " << ex.what() << endl;
		return 1;
	}

	return 0;
}


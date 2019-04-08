/*
 * A test for simple_mtsafe_st_env_infastructure with unknown
 * exception from init_fn function.
 */

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>
#include <test/3rd_party/various_helpers/ensure.hpp>

using namespace std;

class coop_resource_t
{
	so_5::atomic_counter_t & m_counter;

public :
	coop_resource_t( so_5::atomic_counter_t & counter )
		:	m_counter{ counter }
	{
		++m_counter;
	}

	~coop_resource_t() noexcept
	{
		--m_counter;
	}
};

class dummy_actor_t final : public so_5::agent_t
{
	struct next_turn final : public so_5::signal_t {};

	so_5::atomic_counter_t & m_finish_counter;

public :
	dummy_actor_t( context_t ctx, so_5::atomic_counter_t & finish_counter )
		:	so_5::agent_t::agent_t{ std::move(ctx) }
		,	m_finish_counter{ finish_counter }
	{}

	void
	so_evt_start() override
	{
		so_subscribe_self().event( &dummy_actor_t::on_next_turn );

		so_5::send< next_turn >( *this );
	}

	void
	so_evt_finish() override
	{
		++m_finish_counter;
	}

private :
	void
	on_next_turn( mhood_t<next_turn> )
	{
		so_5::send_delayed< next_turn >( *this, std::chrono::milliseconds(10) );
	}
};

int
main()
{
	try
	{
		run_with_time_limit(
			[]() {
				so_5::atomic_counter_t counter{ 0 };
				so_5::atomic_counter_t finish_counter{ 0 };

				try
				{
					so_5::launch(
						[&]( so_5::environment_t & env ) {
							// Create a bunch of coops with dummy_actors inside.
							for( int i = 0; i != 10000; ++i )
							{
								auto coop = env.make_coop();
								coop->take_under_control(
										std::make_unique< coop_resource_t >(
												std::ref(counter) ) );
								coop->make_agent< dummy_actor_t >(
										std::ref(finish_counter) );

								auto id = env.register_coop( std::move(coop) );

								// Some of them will be deregistered.
								if( 0 == (i % 3) )
									env.deregister_coop( id,
											so_5::dereg_reason::normal );
							}

							throw "boom!";
						},
						[]( so_5::environment_params_t & params ) {
							params.infrastructure_factory(
									so_5::env_infrastructures::simple_not_mtsafe::factory() );
						} );

					// An exception should be thrown from so_5::launch.
					std::cout << "We expect an exception from launch";
					std::abort();
				}
				catch( const so_5::exception_t & x )
				{
					std::cout << "Exception is caught: " << x.what() << std::endl;
				}

				const auto actual_counter = counter.load();
				ensure_or_die( 0u == actual_counter,
						"expected value of counter is 0, actual=" +
						std::to_string(actual_counter) );

				const auto actual_finish_counter = finish_counter.load();
				ensure_or_die( 10000u == actual_finish_counter,
						"unexpected finish_counter=" +
						std::to_string(actual_finish_counter) );
			},
			300 );
	}
	catch( const exception & ex )
	{
		cerr << "Error: " << ex.what() << endl;
		return 1;
	}

	return 0;
}


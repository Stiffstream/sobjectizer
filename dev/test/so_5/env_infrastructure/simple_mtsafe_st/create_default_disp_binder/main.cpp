/*
 * A test for create_default_disp_binder.
 */

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>

class dummy_agent : public so_5::agent_t {
public :
	dummy_agent(context_t ctx) : so_5::agent_t(std::move(ctx)) {}
};

class coop_shutdowner : public so_5::agent_t {
	struct stop final : public so_5::signal_t {};
public :
	coop_shutdowner(context_t ctx) : so_5::agent_t(std::move(ctx)) {
		so_subscribe_self().event( [this](mhood_t<stop>) {
			so_deregister_agent_coop_normally();
		} );
	}

	virtual void so_evt_start() override {
		so_5::send<stop>(*this);
	}
};

void
make_coop( so_5::environment_t & env )
{
	env.introduce_coop(
		so_5::make_default_disp_binder( env ),
		[]( so_5::coop_t & coop ) {
			for(int i = 0; i < 100; ++i)
				coop.make_agent<dummy_agent>();

			coop.make_agent<coop_shutdowner>();
		} );
}

int
main()
{
	try
	{
		run_with_time_limit(
			[]() {
				so_5::launch(
					[]( so_5::environment_t & env ) {
						for(int i = 0; i < 10; ++i)
							make_coop( env );
					},
					[]( so_5::environment_params_t & params ) {
						params.infrastructure_factory(
								so_5::env_infrastructures::simple_mtsafe::factory() );
					} );
			},
			20 );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}


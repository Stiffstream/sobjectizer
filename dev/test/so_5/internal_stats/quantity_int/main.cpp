#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>

namespace test
{

class a_stats_listener_t final : public so_5::agent_t
{
public :
	a_stats_listener_t( context_t ctx )
		:	so_5::agent_t( std::move(ctx) )
	{}

	void so_define_agent() override
	{
		using namespace so_5::stats;

		auto & controller = so_environment().stats_controller();

		so_default_state()
			.event( controller.mbox(), &a_stats_listener_t::evt_quantity_int )
			;
	}

	void so_evt_start() override
	{
		so_environment().stats_controller().set_distribution_period(
				std::chrono::milliseconds( 100 ) );
		so_environment().stats_controller().turn_on();
	}

private :
	void evt_quantity_int(
		const so_5::stats::messages::quantity< int > & evt )
	{
		std::cout << "I: '" << evt.m_prefix << evt.m_suffix << "': " << evt.m_value << std::endl;
		so_deregister_agent_coop_normally();
	}
};

class my_data_source_t final : public so_5::stats::source_t
{
public:
	void
	distribute(const so_5::mbox_t & to) override
	{
		so_5::send< so_5::stats::messages::quantity<int> >(
				to,
				"my_data_source",
				"/dummy",
				0 );
	};
};

class a_custom_ds_holder_t final : public so_5::agent_t
{
	so_5::stats::manually_registered_source_holder_t< my_data_source_t > m_ds_holder;

public:
	a_custom_ds_holder_t( context_t ctx ) : so_5::agent_t{ std::move(ctx) }
	{}

	void
	so_evt_start() override
	{
		m_ds_holder.start(
				so_5::outliving_mutable( so_environment().stats_repository() ) );
	}

	void
	so_evt_finish() override
	{
		m_ds_holder.stop();
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
			[]()
			{
				so_5::launch( [](so_5::environment_t & env) {
						env.introduce_coop( [](so_5::coop_t & coop) {
								coop.make_agent< a_stats_listener_t >();
								coop.make_agent< a_custom_ds_holder_t >();
							} );
					} );
			},
			5 );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}


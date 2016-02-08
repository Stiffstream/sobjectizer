/*
 * A test of simple sequence of messages for
 * prio_one_thread::strictly_ordered dispatcher.
 */

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>

struct msg_hello : public so_5::signal_t {};

void
define_receiver_agent(
	so_5::coop_t & coop,
	so_5::disp::prio_one_thread::strictly_ordered::private_dispatcher_t & disp,
	so_5::priority_t priority,
	const so_5::mbox_t & common_mbox,
	std::string & sequence )
	{
		coop.define_agent( coop.make_agent_context() + priority, disp.binder() )
			.event< msg_hello >(
				common_mbox,
				[priority, &sequence] {
					sequence += std::to_string(
						static_cast< std::size_t >( priority ) );
				} );
	}

std::string &
define_main_agent(
	so_5::coop_t & coop,
	so_5::disp::prio_one_thread::strictly_ordered::private_dispatcher_t & disp,
	const so_5::mbox_t & common_mbox )
	{
		auto sequence = std::make_shared< std::string >();

		coop.define_agent( coop.make_agent_context() + so_5::prio::p0, disp.binder() )
			.event< msg_hello >(
				common_mbox,
				[&coop, sequence] {
					*sequence += "0";
					if( "76543210" != *sequence )
						throw std::runtime_error( "Unexpected value of sequence: " +
								*sequence );
					else
						coop.environment().stop();
				} );

		return *sequence;
	}

void
define_starter_agent(
	so_5::coop_t & coop,
	so_5::disp::prio_one_thread::strictly_ordered::private_dispatcher_t & disp )
	{
		coop.define_agent( coop.make_agent_context() + so_5::prio::p0, disp.binder() )
			.on_start( [&coop, &disp] {
				auto common_mbox = coop.environment().create_mbox();

				coop.environment().introduce_coop(
					[&]( so_5::coop_t & child )
					{
						using namespace so_5::prio;

						std::string & sequence = define_main_agent(
								child, disp, common_mbox );
						define_receiver_agent(
								child, disp, p1, common_mbox, sequence );
						define_receiver_agent(
								child, disp, p2, common_mbox, sequence );
						define_receiver_agent(
								child, disp, p3, common_mbox, sequence );
						define_receiver_agent(
								child, disp, p4, common_mbox, sequence );
						define_receiver_agent(
								child, disp, p5, common_mbox, sequence );
						define_receiver_agent(
								child, disp, p6, common_mbox, sequence );
						define_receiver_agent(
								child, disp, p7, common_mbox, sequence );
					} );

				so_5::send< msg_hello >( common_mbox );
			} );
	}

void
fill_coop(
	so_5::coop_t & coop )
	{
		using namespace so_5::disp::prio_one_thread::strictly_ordered;

		define_starter_agent( coop,
			*(create_private_disp(coop.environment())) );
	}

int
main()
{
	try
	{
		// Do several iterations to increase probability of errors detection.
		std::cout << "runing iterations" << std::flush;
		for( int i = 0; i != 100; ++i )
		{
			run_with_time_limit(
				[]()
				{
					so_5::launch(
						[]( so_5::environment_t & env )
						{
							env.introduce_coop( fill_coop );
						} );
				},
				20,
				"simple sequence prio_one_thread::strictly_ordered dispatcher test" );
			std::cout << "." << std::flush;
		}
		std::cout << "done" << std::endl;
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}


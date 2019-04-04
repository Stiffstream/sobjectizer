/*
 * A test for agent_t::so_make_new_direct_mbox().
 */

#include <iostream>
#include <sstream>

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>

class a_test_t final : public so_5::agent_t
	{
		struct first final : public so_5::signal_t {};
		struct second final : public so_5::signal_t {};

	public :
		using so_5::agent_t::agent_t;

		void
		so_evt_start() override
			{
				so_subscribe_self()
						.event( [](mhood_t<first>) {} )
						.event( [](mhood_t<second>) {
								throw std::runtime_error( "second shouldn't be "
										"received from so_direct_mbox()" );
							} );

				const auto another = so_make_new_direct_mbox();

				so_subscribe( another )
						.event( [](mhood_t<first>) {
								throw std::runtime_error( "second shouldn't be "
										"received from another direct mbox" );
							} )
						.event( [this](mhood_t<second>) {
								so_deregister_agent_coop_normally();
							} );

				so_5::send< first >( *this );
				so_5::send< second >( another );
			}
	};

int
main()
{
	run_with_time_limit( [] {
		so_5::launch(
			[]( so_5::environment_t & env )
			{
				env.register_agent_as_coop( env.make_agent< a_test_t >() );
			} );
	},
	10 );

	return 0;
}


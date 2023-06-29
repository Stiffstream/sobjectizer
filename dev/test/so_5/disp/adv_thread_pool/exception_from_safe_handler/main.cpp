/*
 * Check for an exception from thread_safe event handler.
 * Test test should fail.
 */

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>
#include <test/3rd_party/various_helpers/ensure.hpp>

class a_test_t final : public so_5::agent_t
{
	struct check_it final : public so_5::signal_t {};

public:
	using so_5::agent_t::agent_t;

	void
	so_define_agent() override
	{
		so_subscribe_self().event( &a_test_t::evt_check_it, so_5::thread_safe );
	}

	void
	so_evt_start() override
	{
		so_5::send< check_it >( *this );
	}

	so_5::exception_reaction_t
	so_exception_reaction() const noexcept override
	{
		return so_5::deregister_coop_on_exception;
	}

private:
	void
	evt_check_it( mhood_t<check_it> )
	{
		throw std::runtime_error{ "Oops!" };
	}
};

void
run_test()
{
	so_5::launch( [&]( so_5::environment_t & env ) {
			env.introduce_coop( [&]( so_5::coop_t & coop ) {
					auto disp = so_5::disp::adv_thread_pool::make_dispatcher( env );

					coop.make_agent_with_binder< a_test_t >( disp.binder() );
				} );
		},
		[]( so_5::environment_params_t & /*params*/ ) {
#if 0
			params.message_delivery_tracer(
					so_5::msg_tracing::std_cout_tracer() );
#endif
		} );
}

int
main()
{
	try
	{
		run_with_time_limit(
			[]() {
				run_test();
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


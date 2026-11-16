#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>
#include <test/3rd_party/various_helpers/ensure.hpp>

namespace test
{

class a_demo_t final : public so_5::agent_t
{
public:
	a_demo_t( context_t ctx )
		: so_5::agent_t{ ctx + so_5::enable_cpp_coro() }
	{}

	void
	so_evt_start() override
	{
		throw std::runtime_error{ "this method should not be called!" };
	}
};

void
run_demo()
{
	so_5::wrapped_env_t sobj;

	try
	{
		auto & env = sobj.environment();
		env.register_agent_as_coop(
				env.make_agent< a_demo_t >(),
				so_5::disp::prio_one_thread::strictly_ordered::make_dispatcher(
						env ).binder() );

		throw std::runtime_error{ "this code should not run!" };
	}
	catch( const so_5::exception_t & x )
	{
		std::cout << "exception caught: " << x.what() << std::endl;

		ensure_or_die(
				x.error_code() == so_5::rc_cpp_coro_not_supported_by_dispatcher,
				"rc_cpp_coro_not_supported_by_dispatcher expected" );
	}
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


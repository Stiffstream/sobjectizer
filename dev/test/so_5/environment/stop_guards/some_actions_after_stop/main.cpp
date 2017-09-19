/*
 * A test for simple stop_guard.
 */

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>
#include <various_helpers_1/ensure.hpp>

#include <utest_helper_1/h/helper.hpp>

using namespace std;

class empty_stop_guard_t
	: public so_5::stop_guard_t
	, public std::enable_shared_from_this<empty_stop_guard_t>
{
public :
	empty_stop_guard_t()
	{}

	virtual void
	stop() SO_5_NOEXCEPT override
	{}
};

void make_stuff(
	so_5::environment_t & env,
	so_5::outliving_reference_t< bool > step_3_completed )
{
	auto guard = std::make_shared< empty_stop_guard_t >();
	env.setup_stop_guard( guard );

	env.introduce_coop( [&]( so_5::coop_t & coop ) {
		struct step_1 final : public so_5::signal_t {};
		struct step_2 final : public so_5::signal_t {};
		struct step_3 final : public so_5::signal_t {};

		auto a = coop.define_agent();
		a.on_start( [a, &env] {
			env.stop();
			so_5::send_delayed< step_1 >( a, std::chrono::milliseconds(50) );
		} );
		a.event( a, [a]( so_5::mhood_t<step_1> ) {
			so_5::send_delayed< step_2 >( a, std::chrono::milliseconds(50) );
		} );
		a.event( a, [a]( so_5::mhood_t<step_2> ) {
			so_5::send_delayed< step_3 >( a, std::chrono::milliseconds(50) );
		} );
		a.event( a, [step_3_completed, &env, guard]( so_5::mhood_t<step_3> ) {
			step_3_completed.get() = true;
			env.remove_stop_guard( guard );
		} );
	} );
}

int
main()
{
	try
	{
		run_with_time_limit(
			[]() {
				bool step_3_completed = false;
				so_5::launch(
					[&](so_5::environment_t & env) {
						make_stuff( env, so_5::outliving_mutable( step_3_completed ) );
					},
					[](so_5::environment_params_t & params) {
						(void)params;
					} );

				if( !step_3_completed )
					throw std::runtime_error( "step 3 is not completed" );
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


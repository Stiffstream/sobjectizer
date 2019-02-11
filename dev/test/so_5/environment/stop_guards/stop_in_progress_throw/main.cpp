/*
 * A test for adding stop_guard when stop in progress.
 * An exception is expected by default.
 */

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>
#include <test/3rd_party/various_helpers/ensure.hpp>

#include <test/3rd_party/utest_helper/helper.hpp>

using namespace std;

class empty_stop_guard_t
	: public so_5::stop_guard_t
	, public std::enable_shared_from_this<empty_stop_guard_t>
{
public :
	empty_stop_guard_t()
	{}

	virtual void
	stop() noexcept override
	{}
};

void make_stuff( so_5::environment_t & env )
{
	class actor_t final : public so_5::agent_t
	{
	public:
		using so_5::agent_t::agent_t;

		void so_evt_start() override
		{
			so_environment().stop();
			try {
				so_environment().setup_stop_guard(
						std::make_shared< empty_stop_guard_t >() );
				throw std::runtime_error(
					"an exception is expected when "
					"a stop_guard is added when stop is in progress" );
			}
			catch( const so_5::exception_t & ex ) {
				if( so_5::rc_cannot_set_stop_guard_when_stop_is_started !=
						ex.error_code() )
					throw;
			}
		}
	};

	env.introduce_coop( [&]( so_5::coop_t & coop ) {
		coop.make_agent< actor_t >();
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
					[&](so_5::environment_t & env) {
						make_stuff( env );
					},
					[](so_5::environment_params_t & params) {
						(void)params;
					} );
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


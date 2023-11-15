/*
 * A test for exception from sync-start.
 */

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/ensure.hpp>
#include <test/3rd_party/various_helpers/time_limited_execution.hpp>

using namespace std::chrono_literals;

class test_exception final : public std::runtime_error
	{
		int m_code;

	public:
		test_exception(
			const char * description,
			int code )
			:	std::runtime_error{ description }
			,	m_code{ code }
		{}

		[[nodiscard]]
		int
		code() const noexcept { return m_code; }
	};

int
main()
{
	try
	{
		run_with_time_limit(
			[]()
			{
				bool exception_thrown = false;

				try
					{
						so_5::wrapped_env_t env{
								so_5::wrapped_env_t::wait_init_completion,
								[&]( so_5::environment_t & /*env*/ ) {
									std::this_thread::sleep_for(
											std::chrono::milliseconds{ 150 } );

									throw test_exception{ "just a test", 42 };
								}
							};
					}
				catch( const test_exception & x )
					{
						ensure_or_die( 42 == x.code(),
								"unexpected x.code() value" );
						exception_thrown = true;
					}

				ensure_or_die( exception_thrown,
						"exception has to be throw from wrapped_env_t constructor" );
			},
			10 );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}


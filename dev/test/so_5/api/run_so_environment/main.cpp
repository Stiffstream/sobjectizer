/*
	Test of so_5::launch() routines.

	Tests both: compilation of template routines specialization
	and params passin correctnes.
*/

#include <utest_helper_1/h/helper.hpp>

#include <functional>

#include <so_5/all.hpp>

void
init( so_5::environment_t & env )
{
	env.stop();
}

UT_UNIT_TEST( launch_with_free_function_pointer )
{
	so_5::launch( &init );
}

const std::string test_str_param = "Hello!";

void
init_with_string_param(
	so_5::environment_t & env,
	const std::string & param )
{
	if( param != test_str_param )
		throw std::runtime_error( "paramaters error" );

	env.stop();
}

const int test_int_param = 42;

void
init_with_int_param(
	so_5::environment_t & env,
	int param )
{
	if( param != test_int_param )
		throw std::runtime_error( "paramaters error" );

	env.stop();
}

UT_UNIT_TEST( launch_with_parameter )
{
	using namespace std;
	using namespace std::placeholders;

	{
		const std::string param = test_str_param;
		UT_PUSH_CONTEXT( "string parameter");
		so_5::launch(
			bind( &init_with_string_param, _1, param ) );
		UT_POP_CONTEXT();
	}

	{
		const int param = test_int_param;
		UT_PUSH_CONTEXT( "int paramater");
		so_5::launch(
			bind( &init_with_int_param, _1, param ) );
		UT_POP_CONTEXT();
	}
}

struct so_init_tester_t
{
	so_init_tester_t()
	{}

	void
	init(
		so_5::environment_t & env )
	{
		env.stop();
	}

	private:
		so_init_tester_t( const so_init_tester_t & );
		void
		operator = ( const so_init_tester_t & );
};

UT_UNIT_TEST( launch_on_object )
{
	using namespace std;
	using namespace std::placeholders;

	so_init_tester_t so_init_tester;
	so_5::launch(
		bind( &so_init_tester_t::init, &so_init_tester, _1 ) );
}

int
main()
{
	UT_RUN_UNIT_TEST( launch_with_free_function_pointer )
	UT_RUN_UNIT_TEST( launch_with_parameter )
	UT_RUN_UNIT_TEST( launch_on_object )

	return 0;
}

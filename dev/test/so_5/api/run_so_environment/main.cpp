/*
	Test of so_5::api::run_so_environment() routines.

	Tests both: compilation of template routines specialization
	and params passin correctnes.
*/

#include <utest_helper_1/h/helper.hpp>

#include <so_5/rt/h/rt.hpp>
#include <so_5/api/h/api.hpp>

void
init( so_5::rt::so_environment_t & env )
{
	env.stop();
}

UT_UNIT_TEST( run_so_environment )
{
	so_5::api::run_so_environment(
		&init );
}

const std::string test_str_param = "Hello!";

void
init_with_string_param(
	so_5::rt::so_environment_t & env,
	const std::string & param )
{
	if( param != test_str_param )
		throw std::runtime_error( "paramaters error" );

	env.stop();
}

const int test_int_param = 42;

void
init_with_int_param(
	so_5::rt::so_environment_t & env,
	int param )
{
	if( param != test_int_param )
		throw std::runtime_error( "paramaters error" );

	env.stop();
}

UT_UNIT_TEST( run_so_environment_with_parameter )
{
	{
		const std::string param = test_str_param;
		UT_PUSH_CONTEXT( "string paramater");
		so_5::api::run_so_environment_with_parameter(
			&init_with_string_param,
			param );
		UT_POP_CONTEXT();
	}

	{
		const int param = test_int_param;
		UT_PUSH_CONTEXT( "int paramater");
		so_5::api::run_so_environment_with_parameter(
			&init_with_int_param,
			param );
		UT_POP_CONTEXT();
	}
}

struct so_init_tester_t
{
	so_init_tester_t()
	{}

	void
	init(
		so_5::rt::so_environment_t & env )
	{
		env.stop();
	}

	private:
		so_init_tester_t( const so_init_tester_t & );
		void
		operator = ( const so_init_tester_t & );
};

UT_UNIT_TEST( run_so_environment_on_object )
{
	so_init_tester_t so_init_tester;
	so_5::api::run_so_environment_on_object(
		so_init_tester,
		&so_init_tester_t::init );
}

int
main( int argc, char * argv[] )
{
	UT_RUN_UNIT_TEST( run_so_environment )
	UT_RUN_UNIT_TEST( run_so_environment_with_parameter )
	UT_RUN_UNIT_TEST( run_so_environment_on_object )

	return 0;
}

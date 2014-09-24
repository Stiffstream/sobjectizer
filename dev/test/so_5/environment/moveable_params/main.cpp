/*
	Test of moveabillity of environment_params_t.
*/

// NOTE: this test is temporary empty because of deletion of
// some obsolete functions.

#include <utest_helper_1/h/helper.hpp>

#include <so_5/all.hpp>

so_5::rt::environment_params_t
make_param()
{
	so_5::rt::environment_params_t params;

	return params;
}

UT_UNIT_TEST( environment_params )
{
	so_5::rt::environment_params_t param = make_param();
}

int
main( int argc, char * argv[] )
{
	UT_RUN_UNIT_TEST( environment_params )

	return 0;
}


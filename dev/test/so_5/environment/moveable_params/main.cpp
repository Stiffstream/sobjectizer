/*
	Test of moveabillity of so_environment_params_t.
*/

#include <utest_helper_1/h/helper.hpp>

#include <so_5/rt/h/rt.hpp>
#include <so_5/api/h/api.hpp>

so_5::rt::so_environment_params_t
make_param()
{
	so_5::rt::so_environment_params_t params;
	params.mbox_mutex_pool_size( 1024 );

	return params;
}

UT_UNIT_TEST( so_environment_params )
{
	so_5::rt::so_environment_params_t param = make_param();

	UT_CHECK_EQ( 1024, param.mbox_mutex_pool_size() );
}

int
main( int argc, char * argv[] )
{
	UT_RUN_UNIT_TEST( so_environment_params )

	return 0;
}


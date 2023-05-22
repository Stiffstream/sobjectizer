/*
	Test of moveabillity of environment_params_t.
*/

#include <test/3rd_party/utest_helper/helper.hpp>

#include <so_5/all.hpp>

so_5::environment_params_t
make_param()
{
	so_5::environment_params_t params;

	return params;
}

UT_UNIT_TEST( environment_params )
{
	so_5::environment_params_t param = make_param();
}

UT_UNIT_TEST( default_disp_params_on_move_ctor )
{
	so_5::environment_params_t params;

	{
		UT_CHECK_EQ( true,
				so_5::work_thread_activity_tracking_t::unspecified ==
						params.default_disp_params().work_thread_activity_tracking() );

		params.default_disp_params(
				so_5::disp::one_thread::disp_params_t{}
						.turn_work_thread_activity_tracking_on() );

		UT_CHECK_EQ( true,
				so_5::work_thread_activity_tracking_t::on ==
						params.default_disp_params().work_thread_activity_tracking() );
	}

	so_5::environment_params_t p2{ std::move(params) };
	UT_CHECK_EQ( true,
			so_5::work_thread_activity_tracking_t::on ==
					p2.default_disp_params().work_thread_activity_tracking() );
}

UT_UNIT_TEST( default_disp_params_on_move_op )
{
	so_5::environment_params_t params;

	{
		UT_CHECK_EQ( true,
				so_5::work_thread_activity_tracking_t::unspecified ==
						params.default_disp_params().work_thread_activity_tracking() );

		params.default_disp_params(
				so_5::disp::one_thread::disp_params_t{}
						.turn_work_thread_activity_tracking_on() );

		UT_CHECK_EQ( true,
				so_5::work_thread_activity_tracking_t::on ==
						params.default_disp_params().work_thread_activity_tracking() );
	}

	so_5::environment_params_t p2;
	p2 = std::move(params);
	UT_CHECK_EQ( true,
			so_5::work_thread_activity_tracking_t::on ==
					p2.default_disp_params().work_thread_activity_tracking() );
}

int
main()
{
	UT_RUN_UNIT_TEST( environment_params )
	UT_RUN_UNIT_TEST( default_disp_params_on_move_ctor )
	UT_RUN_UNIT_TEST( default_disp_params_on_move_op )

	return 0;
}


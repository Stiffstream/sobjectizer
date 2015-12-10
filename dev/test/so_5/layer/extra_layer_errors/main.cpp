/*
 * A tests for layers.
 */

#include <array>
#include <iostream>
#include <map>
#include <memory>
#include <exception>

#include <so_5/all.hpp>

#include <utest_helper_1/h/helper.hpp>

#include "../separate_so_thread_inl.cpp"

template < int N >
class test_layer_t
	:
		public so_5::layer_t
{
	public:
		test_layer_t()
		{}

		virtual ~test_layer_t()
		{}

		virtual void
		start()
		{}

		virtual void
		shutdown()
		{}

		virtual void
		wait()
		{}
};

class test_layer_bad_start_t
	:
		public so_5::layer_t
{
	public:
		test_layer_bad_start_t()
		{}

		virtual ~test_layer_bad_start_t()
		{}

		virtual void
		start()
		{
			throw std::runtime_error( "failure" );
		}

		virtual void
		shutdown()
		{}

		virtual void
		wait()
		{}
};

class so_environment_error_checker_t
	:	public so_5::environment_t
	,	public separate_so_thread::init_finish_signal_mixin_t
{
		typedef so_5::environment_t base_type_t;
	public:
		so_environment_error_checker_t()
			:
				base_type_t(
					std::move(
						so_5::environment_params_t()
							.add_layer( new test_layer_t< 0 >() )
							.disable_autoshutdown() ) )
		{}

		virtual ~so_environment_error_checker_t(){}

		virtual void
		init()
		{
			init_finished();
		}
};

UT_UNIT_TEST( check_errors )
{
	so_environment_error_checker_t so_env;

	separate_so_thread::run_on( so_env, [&]() {
			const char * const null_str = nullptr;
			// Try to set up layer which is already set.
			try {
				so_env.add_extra_layer( new test_layer_t< 0 > );
				UT_CHECK_EQ( null_str, "exception must be thrown" );
			}
			catch( const so_5::exception_t & x )
			{
				UT_CHECK_EQ(
					so_5::rc_trying_to_add_extra_layer_that_already_exists_in_default_list,
					x.error_code() );
			}

			// Try to set up layer by zero pointer.
			try {
				so_env.add_extra_layer(
					std::unique_ptr< test_layer_t< 1 > >( nullptr ) );
				UT_CHECK_EQ( null_str, "exception must be thrown" );
			}
			catch( const so_5::exception_t & x )
			{
				UT_CHECK_EQ(
					so_5::rc_trying_to_add_nullptr_extra_layer,
					x.error_code() );
			}

			// Try to add new layer. No errors expected.
			so_env.add_extra_layer( new test_layer_t< 1 > );

			// Try to add layer which is already set.
			try {
				so_env.add_extra_layer( new test_layer_t< 1 > );
				UT_CHECK_EQ( null_str, "exception must be thrown" );
			}
			catch( const so_5::exception_t & x )
			{
				UT_CHECK_EQ(
					so_5::rc_trying_to_add_extra_layer_that_already_exists_in_extra_list,
					x.error_code() );
			}

			// Try to add layer which is failed to start.
			try {
				so_env.add_extra_layer(	new test_layer_bad_start_t() );
				UT_CHECK_EQ( null_str, "exception must be thrown" );
			}
			catch( const so_5::exception_t & x )
			{
				UT_CHECK_EQ(
					so_5::rc_unable_to_start_extra_layer,
					x.error_code() );
			}
		} );
}

UT_UNIT_TEST( check_exceptions )
{
	so_environment_error_checker_t so_env;

	separate_so_thread::run_on( so_env, [&]() {
			// Try to set up layer which is already set.
			UT_CHECK_THROW(
				so_5::exception_t,
				so_env.add_extra_layer(
					std::unique_ptr< test_layer_t< 0 > >( new test_layer_t< 0 > ) ) );

			// Try to set up layer by zero pointer.
			UT_CHECK_THROW(
				so_5::exception_t,
				so_env.add_extra_layer(
					std::unique_ptr< test_layer_t< 1 > >( nullptr ) ) );

			// Try to add new layer. No errors expected.
			so_env.add_extra_layer(
				std::unique_ptr< test_layer_t< 1 > >( new test_layer_t< 1 > ) );

			// Try to add layer which is already set.
			UT_CHECK_THROW(
				so_5::exception_t,
				so_env.add_extra_layer(
					std::unique_ptr< test_layer_t< 1 > >( new test_layer_t< 1 > ) ) );

			// Try to add layer which is failed to start.
			UT_CHECK_THROW(
				so_5::exception_t,
				so_env.add_extra_layer(
					std::unique_ptr< test_layer_bad_start_t >( new test_layer_bad_start_t ) ) );
		} );
}

int
main()
{
	UT_RUN_UNIT_TEST( check_errors );
	UT_RUN_UNIT_TEST( check_exceptions );

	return 0;
}

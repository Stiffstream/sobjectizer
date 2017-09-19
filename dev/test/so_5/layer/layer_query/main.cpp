/*
 * A test for layers set and get.
 */

#include <iostream>
#include <map>
#include <exception>

#include <so_5/all.hpp>

#include <utest_helper_1/h/helper.hpp>

#include "../separate_so_thread_inl.cpp"

void * last_created_objects[ 64 ];

template < int N >
class test_layer_t
	:
		public so_5::layer_t
{
	public:
		test_layer_t()
		{
			last_created_objects[ N ] = this;
		}

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

class test_environment_t
	:	public so_5::environment_t
	,	public separate_so_thread::init_finish_signal_mixin_t
{
		typedef so_5::environment_t base_type_t;
	public:
		test_environment_t(
			test_layer_t< 1 > * tl1,
			test_layer_t< 2 > * tl2,
			test_layer_t< 3 > * tl3 )
			:
				base_type_t(
					std::move(
						so_5::environment_params_t()
							.add_layer(
								std::unique_ptr< test_layer_t< 1 > >( tl1 ) )
							.add_layer(
								std::unique_ptr< test_layer_t< 2 > >( tl2 ) )
							.add_layer(
								std::unique_ptr< test_layer_t< 3 > >( tl3 ) )
							.disable_autoshutdown() ) )
		{}

		virtual ~test_environment_t(){}

		virtual void
		init()
		{
			stop();
			init_finished();
		}
};

void
check_layers_match(
	test_layer_t< 1 > * tl1,
	test_layer_t< 2 > * tl2,
	test_layer_t< 3 > * tl3,
	const test_environment_t & so_env )
{
	UT_CHECK_EQ( so_env.query_layer_noexcept< test_layer_t< 1 > >(), tl1 ); 
	UT_CHECK_EQ( so_env.query_layer_noexcept< test_layer_t< 2 > >(), tl2 );
	UT_CHECK_EQ( so_env.query_layer_noexcept< test_layer_t< 3 > >(), tl3 );
}

UT_UNIT_TEST( check_all_exist )
{
	test_layer_t< 1 > * tl1 = new test_layer_t< 1 >;
	test_layer_t< 2 > * tl2 = new test_layer_t< 2 >;
	test_layer_t< 3 > * tl3 = new test_layer_t< 3 >;

	test_environment_t so_env( tl1, tl2, tl3 );

	separate_so_thread::run_on( so_env, [&]() {
			check_layers_match( tl1, tl2, tl3, so_env );
		} );
}

UT_UNIT_TEST( check_1_2_exist )
{
	test_layer_t< 1 > * tl1 = new test_layer_t< 1 >;
	test_layer_t< 2 > * tl2 = new test_layer_t< 2 >;

	test_environment_t so_env( tl1, tl2, nullptr );

	separate_so_thread::run_on( so_env, [&]() {
			check_layers_match( tl1, tl2, nullptr, so_env );
		} );
}

UT_UNIT_TEST( check_1_3_exist )
{
	test_layer_t< 1 > * tl1 = new test_layer_t< 1 >;
	test_layer_t< 3 > * tl3 = new test_layer_t< 3 >;

	test_environment_t so_env( tl1, nullptr, tl3 );

	separate_so_thread::run_on( so_env, [&]() {
			check_layers_match( tl1, nullptr, tl3, so_env );
		} );
}

UT_UNIT_TEST( check_2_3_exist )
{
	test_layer_t< 2 > * tl2 = new test_layer_t< 2 >;
	test_layer_t< 3 > * tl3 = new test_layer_t< 3 >;

	test_environment_t so_env( nullptr, tl2, tl3 );

	separate_so_thread::run_on( so_env, [&]() {
			check_layers_match( nullptr, tl2, tl3, so_env );
		} );
}

#define CHECK_LAYER_EXISTANCE( so_env, N ) \
		UT_CHECK_EQ( \
			so_env.query_layer< test_layer_t< N > >(), \
			static_cast< test_layer_t< N >* >(last_created_objects[ N ]) )

void
init( so_5::environment_t & env )
{
	CHECK_LAYER_EXISTANCE( env, 1 );
	CHECK_LAYER_EXISTANCE( env, 2 );
	CHECK_LAYER_EXISTANCE( env, 3 );
	CHECK_LAYER_EXISTANCE( env, 4 );
	CHECK_LAYER_EXISTANCE( env, 5 );
	CHECK_LAYER_EXISTANCE( env, 6 );
	CHECK_LAYER_EXISTANCE( env, 7 );
	CHECK_LAYER_EXISTANCE( env, 8 );
	CHECK_LAYER_EXISTANCE( env, 9 );
	CHECK_LAYER_EXISTANCE( env, 10 );
	CHECK_LAYER_EXISTANCE( env, 11 );
	CHECK_LAYER_EXISTANCE( env, 12 );
	CHECK_LAYER_EXISTANCE( env, 13 );
	CHECK_LAYER_EXISTANCE( env, 14 );
	CHECK_LAYER_EXISTANCE( env, 15 );
	CHECK_LAYER_EXISTANCE( env, 16 );
	CHECK_LAYER_EXISTANCE( env, 17 );
	CHECK_LAYER_EXISTANCE( env, 18 );
	CHECK_LAYER_EXISTANCE( env, 19 );
	CHECK_LAYER_EXISTANCE( env, 20 );
	CHECK_LAYER_EXISTANCE( env, 21 );
	CHECK_LAYER_EXISTANCE( env, 22 );
	CHECK_LAYER_EXISTANCE( env, 23 );
	CHECK_LAYER_EXISTANCE( env, 24 );
	CHECK_LAYER_EXISTANCE( env, 25 );
	CHECK_LAYER_EXISTANCE( env, 26 );
	CHECK_LAYER_EXISTANCE( env, 27 );
	CHECK_LAYER_EXISTANCE( env, 28 );
	CHECK_LAYER_EXISTANCE( env, 29 );
	CHECK_LAYER_EXISTANCE( env, 30 );
	CHECK_LAYER_EXISTANCE( env, 31 );
	CHECK_LAYER_EXISTANCE( env, 32 );

	env.stop();
}


#define ADD_LAYER( N ) \
	.add_layer( new test_layer_t< N > )

UT_UNIT_TEST( check_many_layers )
{
	so_5::launch(
		&init,
		[]( so_5::environment_params_t & params )
		{
			params
			ADD_LAYER( 1 )
			ADD_LAYER( 2 )
			ADD_LAYER( 3 )
			ADD_LAYER( 4 )
			ADD_LAYER( 5 )
			ADD_LAYER( 6 )
			ADD_LAYER( 7 )
			ADD_LAYER( 8 )
			ADD_LAYER( 9 )
			ADD_LAYER( 10 )
			ADD_LAYER( 11 )
			ADD_LAYER( 12 )
			ADD_LAYER( 13 )
			ADD_LAYER( 14 )
			ADD_LAYER( 15 )
			ADD_LAYER( 16 )
			ADD_LAYER( 17 )
			ADD_LAYER( 18 )
			ADD_LAYER( 19 )
			ADD_LAYER( 20 )
			ADD_LAYER( 21 )
			ADD_LAYER( 22 )
			ADD_LAYER( 23 )
			ADD_LAYER( 24 )
			ADD_LAYER( 25 )
			ADD_LAYER( 26 )
			ADD_LAYER( 27 )
			ADD_LAYER( 28 )
			ADD_LAYER( 29 )
			ADD_LAYER( 30 )
			ADD_LAYER( 31 )
			ADD_LAYER( 32 );
		} );
}


int
main()
{
	UT_RUN_UNIT_TEST( check_all_exist );
	UT_RUN_UNIT_TEST( check_1_2_exist );
	UT_RUN_UNIT_TEST( check_1_3_exist );
	UT_RUN_UNIT_TEST( check_2_3_exist );
	UT_RUN_UNIT_TEST( check_many_layers );

	return 0;
}

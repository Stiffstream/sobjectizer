/*
 * A test for layers.
 */

#include <array>
#include <iostream>
#include <map>
#include <memory>
#include <exception>

#include <so_5/all.hpp>

#include <utest_helper_1/h/helper.hpp>

#include "../separate_so_thread_inl.cpp"

std::array< so_5::layer_t *, 64 > last_created_objects;

template < int N >
class test_layer_t
	:
		public so_5::layer_t
{
	public:
		test_layer_t()
		{
			last_created_objects[ N ] = &*this;
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
				base_type_t( get_params() ),
				m_tl1( tl1 ),
				m_tl2( tl2 ),
				m_tl3( tl3 )
		{}

		virtual ~test_environment_t(){}

		virtual void
		init()
		{
			if( nullptr != m_tl1.get() )
				add_extra_layer( std::move( m_tl1 ) );
			if( nullptr != m_tl2.get() )
				add_extra_layer( std::move( m_tl2 ) );
			if( nullptr != m_tl3.get() )
				add_extra_layer( std::move( m_tl3 ) );

			init_finished();
		}

	private:
		std::unique_ptr< test_layer_t< 1 > > m_tl1;
		std::unique_ptr< test_layer_t< 2 > > m_tl2;
		std::unique_ptr< test_layer_t< 3 > > m_tl3;

		static so_5::environment_params_t
		get_params()
		{
			so_5::environment_params_t params;
			params.disable_autoshutdown();
			return params;
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
	test_layer_t< 3 > * tl3 = nullptr;

	test_environment_t so_env( tl1, tl2, tl3 );

	separate_so_thread::run_on( so_env, [&]() {
			check_layers_match( tl1, tl2, tl3, so_env );
		} );
}

UT_UNIT_TEST( check_1_3_exist )
{
	test_layer_t< 1 > * tl1 = new test_layer_t< 1 >;
	test_layer_t< 2 > * tl2 = nullptr;
	test_layer_t< 3 > * tl3 = new test_layer_t< 3 >;

	test_environment_t so_env( tl1, tl2, tl3 );

	separate_so_thread::run_on( so_env, [&]() {
			check_layers_match( tl1, tl2, tl3, so_env );
		} );
}

UT_UNIT_TEST( check_2_3_exist )
{
	test_layer_t< 1 > * tl1 = nullptr;
	test_layer_t< 2 > * tl2 = new test_layer_t< 2 >;
	test_layer_t< 3 > * tl3 = new test_layer_t< 3 >;

	test_environment_t so_env( tl1, tl2, tl3 );

	separate_so_thread::run_on( so_env, [&]() {
			check_layers_match( tl1, tl2, tl3, so_env );
		} );
}


#define CHECK_LAYER_DONT_EXISTS( so_env, N ) \
		UT_CHECK_EQ( \
			so_env.query_layer_noexcept< test_layer_t< N > >(), \
			static_cast< so_5::layer_t* >( nullptr) )

#define ADD_LAYER( so_env, N ) \
	so_env.add_extra_layer( \
		std::unique_ptr< test_layer_t< N > >( \
			new test_layer_t< N > ) )

#define CHECK_LAYER_EXISTS( so_env, N ) \
		UT_CHECK_EQ( \
			so_env.query_layer_noexcept< test_layer_t< N > >(), \
			last_created_objects[ N ] )


#define CHECK_LAYER( so_env, N ) \
	CHECK_LAYER_DONT_EXISTS( so_env, N ); \
	ADD_LAYER( so_env, N ); \
	CHECK_LAYER_EXISTS( so_env, N );

void
init( so_5::environment_t & env )
{
	CHECK_LAYER( env, 1 )
	CHECK_LAYER( env, 2 )
	CHECK_LAYER( env, 3 )
	CHECK_LAYER( env, 4 )
	CHECK_LAYER( env, 5 )
	CHECK_LAYER( env, 6 )
	CHECK_LAYER( env, 7 )
	CHECK_LAYER( env, 8 )
	CHECK_LAYER( env, 9 )
	CHECK_LAYER( env, 10 )
	CHECK_LAYER( env, 11 )
	CHECK_LAYER( env, 12 )
	CHECK_LAYER( env, 13 )
	CHECK_LAYER( env, 14 )
	CHECK_LAYER( env, 15 )
	CHECK_LAYER( env, 16 )
	CHECK_LAYER( env, 17 )
	CHECK_LAYER( env, 18 )
	CHECK_LAYER( env, 19 )
	CHECK_LAYER( env, 20 )
	CHECK_LAYER( env, 21 )
	CHECK_LAYER( env, 22 )
	CHECK_LAYER( env, 23 )
	CHECK_LAYER( env, 24 )
	CHECK_LAYER( env, 25 )
	CHECK_LAYER( env, 26 )
	CHECK_LAYER( env, 27 )
	CHECK_LAYER( env, 28 )
	CHECK_LAYER( env, 29 )
	CHECK_LAYER( env, 30 )
	CHECK_LAYER( env, 31 )
	CHECK_LAYER( env, 32 )
	CHECK_LAYER( env, 33 )
	CHECK_LAYER( env, 34 )
	CHECK_LAYER( env, 35 )
	CHECK_LAYER( env, 36 )
	CHECK_LAYER( env, 37 )
	CHECK_LAYER( env, 38 )
	CHECK_LAYER( env, 39 )
	CHECK_LAYER( env, 40 )
	CHECK_LAYER( env, 41 )
	CHECK_LAYER( env, 42 )
	CHECK_LAYER( env, 43 )
	CHECK_LAYER( env, 44 )
	CHECK_LAYER( env, 45 )
	CHECK_LAYER( env, 46 )
	CHECK_LAYER( env, 47 )
	CHECK_LAYER( env, 48 )
	CHECK_LAYER( env, 49 )
	CHECK_LAYER( env, 50 )
	CHECK_LAYER( env, 51 )
	CHECK_LAYER( env, 52 )
	CHECK_LAYER( env, 53 )
	CHECK_LAYER( env, 54 )
	CHECK_LAYER( env, 55 )
	CHECK_LAYER( env, 56 )
	CHECK_LAYER( env, 57 )
	CHECK_LAYER( env, 58 )
	CHECK_LAYER( env, 59 )
	CHECK_LAYER( env, 60 )
	CHECK_LAYER( env, 61 )
	CHECK_LAYER( env, 62 )
	CHECK_LAYER( env, 63 )

	CHECK_LAYER_EXISTS( env, 1 );
	CHECK_LAYER_EXISTS( env, 2 );
	CHECK_LAYER_EXISTS( env, 3 );
	CHECK_LAYER_EXISTS( env, 4 );
	CHECK_LAYER_EXISTS( env, 5 );
	CHECK_LAYER_EXISTS( env, 6 );
	CHECK_LAYER_EXISTS( env, 7 );
	CHECK_LAYER_EXISTS( env, 8 );
	CHECK_LAYER_EXISTS( env, 9 );
	CHECK_LAYER_EXISTS( env, 10 );
	CHECK_LAYER_EXISTS( env, 11 );
	CHECK_LAYER_EXISTS( env, 12 );
	CHECK_LAYER_EXISTS( env, 13 );
	CHECK_LAYER_EXISTS( env, 14 );
	CHECK_LAYER_EXISTS( env, 15 );
	CHECK_LAYER_EXISTS( env, 16 );
	CHECK_LAYER_EXISTS( env, 17 );
	CHECK_LAYER_EXISTS( env, 18 );
	CHECK_LAYER_EXISTS( env, 19 );
	CHECK_LAYER_EXISTS( env, 20 );
	CHECK_LAYER_EXISTS( env, 21 );
	CHECK_LAYER_EXISTS( env, 22 );
	CHECK_LAYER_EXISTS( env, 23 );
	CHECK_LAYER_EXISTS( env, 24 );
	CHECK_LAYER_EXISTS( env, 25 );
	CHECK_LAYER_EXISTS( env, 26 );
	CHECK_LAYER_EXISTS( env, 27 );
	CHECK_LAYER_EXISTS( env, 28 );
	CHECK_LAYER_EXISTS( env, 29 );
	CHECK_LAYER_EXISTS( env, 30 );
	CHECK_LAYER_EXISTS( env, 31 );
	CHECK_LAYER_EXISTS( env, 32 );
	CHECK_LAYER_EXISTS( env, 33 );
	CHECK_LAYER_EXISTS( env, 34 );
	CHECK_LAYER_EXISTS( env, 35 );
	CHECK_LAYER_EXISTS( env, 36 );
	CHECK_LAYER_EXISTS( env, 37 );
	CHECK_LAYER_EXISTS( env, 38 );
	CHECK_LAYER_EXISTS( env, 39 );
	CHECK_LAYER_EXISTS( env, 40 );
	CHECK_LAYER_EXISTS( env, 41 );
	CHECK_LAYER_EXISTS( env, 42 );
	CHECK_LAYER_EXISTS( env, 43 );
	CHECK_LAYER_EXISTS( env, 44 );
	CHECK_LAYER_EXISTS( env, 45 );
	CHECK_LAYER_EXISTS( env, 46 );
	CHECK_LAYER_EXISTS( env, 47 );
	CHECK_LAYER_EXISTS( env, 48 );
	CHECK_LAYER_EXISTS( env, 49 );
	CHECK_LAYER_EXISTS( env, 50 );
	CHECK_LAYER_EXISTS( env, 51 );
	CHECK_LAYER_EXISTS( env, 52 );
	CHECK_LAYER_EXISTS( env, 53 );
	CHECK_LAYER_EXISTS( env, 54 );
	CHECK_LAYER_EXISTS( env, 55 );
	CHECK_LAYER_EXISTS( env, 56 );
	CHECK_LAYER_EXISTS( env, 57 );
	CHECK_LAYER_EXISTS( env, 58 );
	CHECK_LAYER_EXISTS( env, 59 );
	CHECK_LAYER_EXISTS( env, 60 );
	CHECK_LAYER_EXISTS( env, 61 );
	CHECK_LAYER_EXISTS( env, 62 );
	CHECK_LAYER_EXISTS( env, 63 );

	env.stop();
}



UT_UNIT_TEST( check_many_layers )
{
	so_5::launch( &init );
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

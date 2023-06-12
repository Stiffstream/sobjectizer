#include <so_5/unique_subscribers_mbox.hpp>
#include <so_5/all.hpp>

#include <test/3rd_party/utest_helper/helper.hpp>
#include <test/3rd_party/various_helpers/time_limited_execution.hpp>

UT_UNIT_TEST( mbox_namespace_name )
{
	UT_CHECK_THROW( so_5::exception_t, so_5::mbox_namespace_name_t{ "" } );

	std::string_view name{ "a" };
	so_5::mbox_namespace_name_t mbox_namespace{ name };

	UT_CHECK_EQ( name, mbox_namespace.query_name() );
}

UT_UNIT_TEST( all_different_names )
{
	so_5::mbox_id_t first_id{}, second_id{}, third_id{};

	run_with_time_limit( [&] {
			so_5::launch( [&](so_5::environment_t & env) {
						auto first = env.create_mbox( "demo" );
						first_id = first->id();

						auto second = env.introduce_named_mbox(
								so_5::mbox_namespace_name_t{ "global" },
								"demo",
								[&env]() { return env.create_mbox(); } );
						second_id = second->id();

						auto third = env.introduce_named_mbox(
								so_5::mbox_namespace_name_t{ "local" },
								"demo",
								[&env]() { return env.create_mbox(); } );
						third_id = third->id();
					} );
		},
		5 );

	UT_CHECK_NE( first_id, second_id );
	UT_CHECK_NE( first_id, third_id );
	UT_CHECK_NE( second_id, third_id );
}

UT_UNIT_TEST( duplicate_names )
{
	so_5::mbox_id_t first_0{}, first_1{},
			second_0{}, second_1{},
			third_0{}, third_1{};

	run_with_time_limit( [&] {
			so_5::launch( [&](so_5::environment_t & env) {
						{
							auto m1 = env.create_mbox( "demo" );
							first_0 = m1->id();

							auto m2 = env.create_mbox( "demo" );
							first_1 = m2->id();
						}

						{
							auto m1 = env.introduce_named_mbox(
									so_5::mbox_namespace_name_t{ "global" },
									"demo",
									[&env]() { return env.create_mbox(); } );
							second_0 = m1->id();

							auto m2 = env.introduce_named_mbox(
									so_5::mbox_namespace_name_t{ "global" },
									"demo",
									[&env]() { return env.create_mbox(); } );
							second_1 = m2->id();
						}

						{
							auto m1 = env.introduce_named_mbox(
									so_5::mbox_namespace_name_t{ "local" },
									"demo",
									[&env]() { return env.create_mbox(); } );
							third_0 = m1->id();

							auto m2 = env.introduce_named_mbox(
									so_5::mbox_namespace_name_t{ "local" },
									"demo",
									[&env]() { return env.create_mbox(); } );
							third_1 = m2->id();
						}
					} );
		},
		5 );

	UT_CHECK_EQ( first_0, first_1 );
	UT_CHECK_EQ( second_0, second_1 );
	UT_CHECK_EQ( third_0, third_1 );

	UT_CHECK_NE( first_0, second_0 );
	UT_CHECK_NE( first_0, third_0 );
	UT_CHECK_NE( second_0, third_0 );
}

UT_UNIT_TEST( nested_factory_call )
{
	so_5::mbox_id_t first_id{}, second_id{}, third_id{};

	run_with_time_limit( [&] {
			so_5::launch( [&](so_5::environment_t & env) {
						auto m1 = env.introduce_named_mbox(
								so_5::mbox_namespace_name_t{ "global" },
								"demo",
								[&]() {
									auto m2 = env.introduce_named_mbox(
											so_5::mbox_namespace_name_t{ "local" },
											"demo",
											[&]() {
												auto m3 = env.create_mbox( "demo" );
												third_id = m3->id();
												return m3;
											} );

									second_id = m2->id();
									return m2;
								} );
						first_id = m1->id();
					} );
		},
		5 );

	UT_CHECK_EQ( first_id, second_id );
	UT_CHECK_EQ( first_id, third_id );
}

UT_UNIT_TEST( exception_from_factory )
{
	run_with_time_limit( [&] {
			so_5::launch( [&](so_5::environment_t & env) {
						try
						{
							auto m1 = env.introduce_named_mbox(
									so_5::mbox_namespace_name_t{ "global" },
									"demo",
									[]() -> so_5::mbox_t {
										throw std::runtime_error{ "Oops!" };
									} );

							// We can't be here!
							std::cerr << "An exception has to be thrown by "
									"introduce_named_mbox call, aborting..."
									<< std::endl;

							std::abort();
						}
						catch( const std::runtime_error & x )
						{
							if( std::string_view{ "Oops!" } != x.what() )
							{
								std::cerr << "Unexpected exception caught: "
										<< x.what() << std::endl
										<< "Aborting..."
										<< std::endl;

								std::abort();
							}
						}

						std::ignore = env.introduce_named_mbox(
								so_5::mbox_namespace_name_t{ "global" },
								"demo",
								[&env]() { return env.create_mbox(); } );
					} );
		},
		5 );
}

int main()
{
	UT_RUN_UNIT_TEST( mbox_namespace_name )
	UT_RUN_UNIT_TEST( all_different_names )
	UT_RUN_UNIT_TEST( duplicate_names )
	UT_RUN_UNIT_TEST( nested_factory_call )
	UT_RUN_UNIT_TEST( exception_from_factory )
}


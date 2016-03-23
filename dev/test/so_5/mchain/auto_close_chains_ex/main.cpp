/*
 * Test for auto_close_mchains() function with exceptions.
 */

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>

using namespace std;

class test_ex_t : public runtime_error
{
public :
	test_ex_t() : runtime_error{ "test exception" } {}
};

void
worker_thread( so_5::mchain_t command_ch, so_5::mchain_t reply_ch )
{
	receive( from( command_ch ),
		[reply_ch]( int i ) { so_5::send< string >( reply_ch, to_string(i) ); } );

	close_retain_content( reply_ch );
}

class auto_joiner
{
	thread & m_what;
public :
	auto_joiner( thread & what ) : m_what{ what } {}
	~auto_joiner() { m_what.join(); }
};

void
do_test_case(
	so_5::environment_t & env,
	bool throw_exception )
{
	auto first_cmd_ch = create_mchain( env );
	auto first_reply_ch = create_mchain( env );
	thread first_worker{ worker_thread, first_cmd_ch, first_reply_ch };
	auto_joiner first_joiner{ first_worker };
	auto first_ch_closer = so_5::auto_close_mchains(
			so_5::mchain_props::close_mode_t::drop_content,
			first_cmd_ch,
			first_reply_ch );

	auto second_cmd_ch = create_mchain( env );
	auto second_reply_ch = create_mchain( env );
	thread second_worker{ worker_thread, second_cmd_ch, second_reply_ch };
	auto_joiner second_joiner{ second_worker };
	auto second_ch_closer = so_5::auto_close_mchains(
			so_5::mchain_props::close_mode_t::drop_content,
			second_cmd_ch,
			second_reply_ch );

	if( throw_exception )
		throw test_ex_t{};

	for( int i = 0; i != 3; ++i )
	{
		so_5::send< int >( first_cmd_ch, i );
		so_5::send< int >( second_cmd_ch, -i );
	}

	so_5::select( so_5::from_all().empty_timeout( chrono::milliseconds(500) ),
		case_( first_reply_ch, []( string s ) {
				cout << "first: " << s << endl;
			} ),
		case_( second_reply_ch, []( string s ) {
				cout << "second: " << s << endl;
			} ) );

	close_drop_content( first_cmd_ch );
	close_drop_content( second_cmd_ch );
}

void
do_check( bool throw_exception )
{
	run_with_time_limit(
		[=]()
		{
			so_5::wrapped_env_t sobj;

			cout << "=== "
					<< (throw_exception ? "throw" : "no throw")
					<< endl;

			if( throw_exception )
			{
				try
				{
					do_test_case( sobj.environment(), throw_exception );
					throw runtime_error( "test_ex_t is not thrown" );
				}
				catch( const test_ex_t & ) {}
			}
			else
				do_test_case( sobj.environment(), throw_exception );
		},
		20 );
}

int
main()
{
	try
	{
		do_check( false );
		do_check( true );
	}
	catch( const exception & ex )
	{
		cerr << "Error: " << ex.what() << endl;
		return 1;
	}

	return 0;
}


/*
 * Usage of select() for listening several mchains.
 */

#include <algorithm>
#include <iostream>
#include <string>

// Main SObjectizer header file.
#include <so_5/all.hpp>

using namespace std;

void reverse_worker( so_5::mchain_t command_ch, so_5::mchain_t reply_ch )
{
	receive( from( command_ch ), [&]( string str ) {
			this_thread::sleep_for( chrono::milliseconds( 10 * str.size() ) );
			reverse( str.begin(), str.end() );
			so_5::send< string >( reply_ch, str );
		} );

	close_retain_content( reply_ch );
}

void doubler_worker( so_5::mchain_t command_ch, so_5::mchain_t reply_ch )
{
	receive( from( command_ch ), [&]( string str ) {
			this_thread::sleep_for( chrono::milliseconds( 5 * str.size() * 2 ) );
			so_5::send< string >( reply_ch, str + str );
		} );

	close_retain_content( reply_ch );
}

void demo()
{
	// A SObjectizer instance.
	so_5::wrapped_env_t sobj;

	// Strings to be sent to workers.
	vector< string > strings{ "one", "two", "three", "four" };

	// Thread objects for workers.
	thread reverse_thread;
	thread doubler_thread;
	// Threads must be automatically joined (either normal return from
	// demo() or in case of an exception).
	auto thread_joiner = so_5::auto_join( reverse_thread, doubler_thread );

	// Create workers.
	
	// The first worker and its stuff.
	// Chains for the first worker.
	auto reverse_cmd_ch = create_mchain( sobj );
	auto reverse_reply_ch = create_mchain( sobj );
	// Command and reply chains must be closed automatically in case
	// of unexpected exit (as result of exception for example).
	// Content of mchain must be dropped in case of unxpected exit.
	auto reverse_chain_closer = so_5::auto_close_drop_content(
			reverse_cmd_ch, reverse_reply_ch );
	// Launching thread for the first worker.
	reverse_thread = thread{ reverse_worker, reverse_cmd_ch, reverse_reply_ch };
	// Interator for reverse_worker tasks.
	auto reverse_it = strings.begin();

	// The second worker and its stuff.
	auto doubler_cmd_ch = create_mchain( sobj );
	auto doubler_reply_ch = create_mchain( sobj );
	// Command and reply chains must be closed automatically in case
	// of unexpected exit (as result of exception for example).
	// Content of mchain must be dropped in case of unxpected exit.
	auto doubler_chain_closer = so_5::auto_close_drop_content(
			doubler_cmd_ch, doubler_reply_ch );
	// Launching thread for the second worker.
	doubler_thread = thread{ doubler_worker, doubler_cmd_ch, doubler_reply_ch };
	// Interator for doubler_worker tasks.
	auto doubler_it = strings.begin();

	// Send initial messages.
	so_5::send< string >( reverse_cmd_ch, *(reverse_it++) );
	so_5::send< string >( doubler_cmd_ch, *(doubler_it++) );

	// Handle results and initiate new tasks.
	select( so_5::from_all(),
		case_( reverse_reply_ch, [&]( const string & v ) {
				cout << "reverse_result: " << v << endl;
				if( reverse_it != strings.end() )
					so_5::send< string >( reverse_cmd_ch, *(reverse_it++) );
				else
					// No more work for reverse worker. Close command chain and
					// worker will finish its work.
					close_drop_content( reverse_cmd_ch );
			} ),
		case_( doubler_reply_ch, [&]( const string & v ) {
				cout << "doubler_result: " << v << endl;
				if( doubler_it != strings.end() )
					so_5::send< string >( doubler_cmd_ch, *(doubler_it++) );
				else
					// No more work for doubler worker. Close command chain and
					// worker will finish its work.
					close_drop_content( doubler_cmd_ch );
			} ) );

	// There is no need to call join() for reverse_worker and doubler_worker
	// because of auto_joiner objects.

	// SObjectizer will be stopped automatically.
}

int main()
{
	try
	{
		demo();
	}
	catch( const exception & ex )
	{
		cerr << "Error: " << ex.what() << endl;
	}

	return 0;
}


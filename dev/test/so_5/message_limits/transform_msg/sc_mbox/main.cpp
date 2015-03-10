/*
 * A simple test for message limits (transforming message).
 */

#include "../test_logic.inl"

int
main()
{
	do_test( "mpsc_mbox test case",
		[]( a_test_t & a ) { a.set_working_mbox( a.so_direct_mbox() ); } );

	return 0;
}

